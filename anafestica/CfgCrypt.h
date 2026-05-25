//---------------------------------------------------------------------------

#ifndef CfgCryptH
#define CfgCryptH

#include <System.Classes.hpp>
#include <System.IOUtils.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>

#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <vector>

#include <anafestica/FileVersionInfo.h>

#pragma comment( lib, "Bcrypt" )

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace Crypt {
//---------------------------------------------------------------------------

using Bytes = std::vector<BYTE>;

struct TOptions {
    bool Enabled {};
    String Secret;
    String ApplicationId;

    TOptions() = default;
    TOptions( String SecretKey, String AppId )
        : Enabled{ true }, Secret{ SecretKey }, ApplicationId{ AppId } {}

    static TOptions Default();
};

namespace Detail {

static constexpr std::array<BYTE, 12> Magic {
    'A', 'N', 'A', 'F', 'C', 'R', 'Y', 'P', 'T', '0', '1', 0
};
static constexpr BYTE Version = 1;
static constexpr ULONG NonceSize = 12;
static constexpr ULONG TagSize = 16;

inline void Check( NTSTATUS Status ) {
    if ( Status < 0 ) {
        throw Exception(
            _D( "Anafestica crypt error 0x%08x" ),
            ARRAYOFCONST(( Status ))
        );
    }
}

class AlgHandle {
public:
    explicit AlgHandle( LPCWSTR Algo ) {
        Check( ::BCryptOpenAlgorithmProvider( &handle_, Algo, nullptr, 0 ) );
    }
    ~AlgHandle() {
        if ( handle_ ) {
            ::BCryptCloseAlgorithmProvider( handle_, 0 );
        }
    }
    AlgHandle( AlgHandle const & ) = delete;
    AlgHandle& operator=( AlgHandle const & ) = delete;

    [[nodiscard]] BCRYPT_ALG_HANDLE Get() const noexcept { return handle_; }

private:
    BCRYPT_ALG_HANDLE handle_ {};
};

class KeyHandle {
public:
    KeyHandle( AlgHandle& Alg, Bytes const & Key ) {
        DWORD Dummy {};
        DWORD ObjectLen {};
        Check(
            ::BCryptGetProperty(
                Alg.Get(), BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PBYTE>( &ObjectLen ),
                sizeof ObjectLen, &Dummy, 0
            )
        );
        keyObject_.resize( ObjectLen );
        Check(
            ::BCryptGenerateSymmetricKey(
                Alg.Get(), &handle_, keyObject_.data(),
                static_cast<ULONG>( keyObject_.size() ),
                const_cast<PUCHAR>( Key.data() ),
                static_cast<ULONG>( Key.size() ), 0
            )
        );
    }
    ~KeyHandle() {
        if ( handle_ ) {
            ::BCryptDestroyKey( handle_ );
        }
        std::fill( keyObject_.begin(), keyObject_.end(), 0 );
    }
    KeyHandle( KeyHandle const & ) = delete;
    KeyHandle& operator=( KeyHandle const & ) = delete;

    [[nodiscard]] BCRYPT_KEY_HANDLE Get() const noexcept { return handle_; }

private:
    BCRYPT_KEY_HANDLE handle_ {};
    Bytes keyObject_;
};

inline Bytes ToUTF8Bytes( String const & Text ) {
    auto Enc = TEncoding::UTF8->GetBytes( Text );
    return Bytes( std::begin( Enc ), std::end( Enc ) );
}

inline Bytes SHA256( Bytes const & Data ) {
    AlgHandle Alg{ BCRYPT_SHA256_ALGORITHM };
    DWORD Dummy {};
    DWORD ObjectLen {};
    DWORD HashLen {};
    Check(
        ::BCryptGetProperty(
            Alg.Get(), BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PBYTE>( &ObjectLen ), sizeof ObjectLen,
            &Dummy, 0
        )
    );
    Check(
        ::BCryptGetProperty(
            Alg.Get(), BCRYPT_HASH_LENGTH,
            reinterpret_cast<PBYTE>( &HashLen ), sizeof HashLen,
            &Dummy, 0
        )
    );
    Bytes Object( ObjectLen );
    BCRYPT_HASH_HANDLE Hash {};
    Check(
        ::BCryptCreateHash(
            Alg.Get(), &Hash, Object.data(),
            static_cast<ULONG>( Object.size() ), nullptr, 0, 0
        )
    );
    try {
        Check(
            ::BCryptHashData(
                Hash, const_cast<PUCHAR>( Data.data() ),
                static_cast<ULONG>( Data.size() ), 0
            )
        );
        Bytes Result( HashLen );
        Check(
            ::BCryptFinishHash(
                Hash, Result.data(),
                static_cast<ULONG>( Result.size() ), 0
            )
        );
        ::BCryptDestroyHash( Hash );
        return Result;
    }
    catch ( ... ) {
        ::BCryptDestroyHash( Hash );
        throw;
    }
}

inline Bytes DeriveKey( TOptions const & Options ) {
    auto Material =
        ToUTF8Bytes(
            Options.Secret + _D( "\nAnafestica\n" ) + Options.ApplicationId
        );
    return SHA256( Material );
}

inline Bytes ReadAllBytes( String const & FileName ) {
    if ( !TFile::Exists( FileName ) ) {
        return {};
    }
    auto BytesArray = TFile::ReadAllBytes( FileName );
    return Bytes( std::begin( BytesArray ), std::end( BytesArray ) );
}

inline void WriteAllBytes( String const & FileName, Bytes const & Data ) {
    TBytes BytesArray;
    BytesArray.Length = static_cast<int>( Data.size() );
    if ( !Data.empty() ) {
        std::copy( Data.begin(), Data.end(), std::begin( BytesArray ) );
    }
    TFile::WriteAllBytes( FileName, BytesArray );
}

inline bool HasHeader( Bytes const & Data ) {
    return
        Data.size() > Magic.size() + 1 + NonceSize + TagSize &&
        std::equal( Magic.begin(), Magic.end(), Data.begin() ) &&
        Data[Magic.size()] == Version;
}

} // End namespace Detail

inline String GetDefaultApplicationId() {
    try {
        return TFileVersionInfo{
            ::GetModuleName( reinterpret_cast<NativeUInt>( HInstance ) )
        }.ProductName;
    }
    catch ( ... ) {
        return ::GetModuleName( reinterpret_cast<NativeUInt>( HInstance ) );
    }
}

inline String GetMachineSecret() {
    try {
        auto Reg = std::make_unique<TRegistry>( KEY_READ );
        Reg->RootKey = HKEY_LOCAL_MACHINE;
        if ( Reg->OpenKeyReadOnly( _D( "SOFTWARE\\Microsoft\\Cryptography" ) ) ) {
            auto const MachineGuid = Reg->GetDataAsString( _D( "MachineGuid" ) );
            if ( !MachineGuid.IsEmpty() ) {
                return MachineGuid;
            }
        }
    }
    catch ( ... ) {
    }

    wchar_t WindowsPath[MAX_PATH] {};
    if ( ::SHGetFolderPathW( nullptr, CSIDL_WINDOWS, nullptr, 0, WindowsPath ) == S_OK ) {
        wchar_t RootPath[MAX_PATH] {};
        if ( ::GetVolumePathNameW( WindowsPath, RootPath, MAX_PATH ) ) {
            DWORD Serial {};
            if ( ::GetVolumeInformationW(
                    RootPath, nullptr, 0, &Serial, nullptr, nullptr, nullptr, 0
                 ) )
            {
                return IntToHex( static_cast<int>( Serial ), 8 );
            }
        }
    }

    return {};
}

inline TOptions TOptions::Default() {
    auto Secret = GetMachineSecret();
    auto AppId = GetDefaultApplicationId();
    return Secret.IsEmpty() || AppId.IsEmpty() ? TOptions{} : TOptions{ Secret, AppId };
}

inline Bytes Encrypt( TOptions const & Options, Bytes const & PlainText ) {
    if ( !Options.Enabled ) {
        return PlainText;
    }

    Detail::AlgHandle Alg{ BCRYPT_AES_ALGORITHM };
    Detail::Check(
        ::BCryptSetProperty(
            Alg.Get(), BCRYPT_CHAINING_MODE,
            const_cast<PUCHAR>(
                reinterpret_cast<const UCHAR*>( BCRYPT_CHAIN_MODE_GCM )
            ),
            sizeof( BCRYPT_CHAIN_MODE_GCM ), 0
        )
    );

    auto KeyBytes = Detail::DeriveKey( Options );
    Detail::KeyHandle Key{ Alg, KeyBytes };

    Bytes Nonce( Detail::NonceSize );
    Detail::Check(
        ::BCryptGenRandom(
            nullptr, Nonce.data(), static_cast<ULONG>( Nonce.size() ),
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        )
    );

    Bytes Tag( Detail::TagSize );
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AuthInfo;
    BCRYPT_INIT_AUTH_MODE_INFO( AuthInfo );
    AuthInfo.pbNonce = Nonce.data();
    AuthInfo.cbNonce = static_cast<ULONG>( Nonce.size() );
    AuthInfo.pbTag = Tag.data();
    AuthInfo.cbTag = static_cast<ULONG>( Tag.size() );

    ULONG CipherLen {};
    Detail::Check(
        ::BCryptEncrypt(
            Key.Get(), const_cast<PUCHAR>( PlainText.data() ),
            static_cast<ULONG>( PlainText.size() ), &AuthInfo,
            nullptr, 0, nullptr, 0, &CipherLen, 0
        )
    );

    Bytes CipherText( CipherLen );
    Detail::Check(
        ::BCryptEncrypt(
            Key.Get(), const_cast<PUCHAR>( PlainText.data() ),
            static_cast<ULONG>( PlainText.size() ), &AuthInfo,
            nullptr, 0, CipherText.data(),
            static_cast<ULONG>( CipherText.size() ), &CipherLen, 0
        )
    );
    CipherText.resize( CipherLen );

    Bytes Result;
    Result.reserve( Detail::Magic.size() + 1 + Nonce.size() + Tag.size() + CipherText.size() );
    Result.insert( Result.end(), Detail::Magic.begin(), Detail::Magic.end() );
    Result.push_back( Detail::Version );
    Result.insert( Result.end(), Nonce.begin(), Nonce.end() );
    Result.insert( Result.end(), Tag.begin(), Tag.end() );
    Result.insert( Result.end(), CipherText.begin(), CipherText.end() );
    return Result;
}

inline Bytes Decrypt( TOptions const & Options, Bytes const & FileBytes ) {
    if ( !Options.Enabled ) {
        return FileBytes;
    }
    if ( !Detail::HasHeader( FileBytes ) ) {
        throw Exception( _D( "Invalid Anafestica encrypted configuration file" ) );
    }

    auto Pos = FileBytes.begin() + Detail::Magic.size() + 1;
    Bytes Nonce( Pos, Pos + Detail::NonceSize );
    Pos += Detail::NonceSize;
    Bytes Tag( Pos, Pos + Detail::TagSize );
    Pos += Detail::TagSize;
    Bytes CipherText( Pos, FileBytes.end() );

    Detail::AlgHandle Alg{ BCRYPT_AES_ALGORITHM };
    Detail::Check(
        ::BCryptSetProperty(
            Alg.Get(), BCRYPT_CHAINING_MODE,
            const_cast<PUCHAR>(
                reinterpret_cast<const UCHAR*>( BCRYPT_CHAIN_MODE_GCM )
            ),
            sizeof( BCRYPT_CHAIN_MODE_GCM ), 0
        )
    );

    auto KeyBytes = Detail::DeriveKey( Options );
    Detail::KeyHandle Key{ Alg, KeyBytes };

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AuthInfo;
    BCRYPT_INIT_AUTH_MODE_INFO( AuthInfo );
    AuthInfo.pbNonce = Nonce.data();
    AuthInfo.cbNonce = static_cast<ULONG>( Nonce.size() );
    AuthInfo.pbTag = Tag.data();
    AuthInfo.cbTag = static_cast<ULONG>( Tag.size() );

    ULONG PlainLen {};
    Detail::Check(
        ::BCryptDecrypt(
            Key.Get(), CipherText.data(), static_cast<ULONG>( CipherText.size() ),
            &AuthInfo, nullptr, 0, nullptr, 0, &PlainLen, 0
        )
    );

    Bytes PlainText( PlainLen );
    Detail::Check(
        ::BCryptDecrypt(
            Key.Get(), CipherText.data(), static_cast<ULONG>( CipherText.size() ),
            &AuthInfo, nullptr, 0, PlainText.data(),
            static_cast<ULONG>( PlainText.size() ), &PlainLen, 0
        )
    );
    PlainText.resize( PlainLen );
    return PlainText;
}

inline Bytes LoadBytes( String const & FileName, TOptions const & Options ) {
    return Decrypt( Options, Detail::ReadAllBytes( FileName ) );
}

inline void SaveBytes( String const & FileName, Bytes const & PlainText,
                       TOptions const & Options )
{
    Detail::WriteAllBytes( FileName, Encrypt( Options, PlainText ) );
}

inline String LoadText( String const & FileName, TEncoding* Encoding,
                        TOptions const & Options )
{
    auto PlainText = LoadBytes( FileName, Options );
    TBytes BytesArray;
    BytesArray.Length = static_cast<int>( PlainText.size() );
    if ( !PlainText.empty() ) {
        std::copy( PlainText.begin(), PlainText.end(), std::begin( BytesArray ) );
    }
    return Encoding->GetString( BytesArray );
}

inline void SaveText( String const & FileName, String const & Text,
                      TEncoding* Encoding, TOptions const & Options )
{
    auto BytesArray = Encoding->GetBytes( Text );
    SaveBytes(
        FileName, Bytes( std::begin( BytesArray ), std::end( BytesArray ) ),
        Options
    );
}

//---------------------------------------------------------------------------
} // End namespace Crypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
