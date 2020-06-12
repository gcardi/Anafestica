//---------------------------------------------------------------------------

#ifndef CfgJSONH
#define CfgJSONH

#include <System.JSON.hpp>
#include <System.IOUtils.hpp>
#include <System.JSON.Writers.hpp>
#include <System.DateUtils.hpp>
#include <System.NetEncoding.hpp>
#include <System.SysUtils.hpp>

#include <memory>
#include <string_view>
#include <vector>
#include <array>
#include <functional>
#include <iterator>
#include <string>

#include <anafestica/Cfg.h>

template<typename T>
class TD;

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSON {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
private:
    static constexpr LPTSTR ValuesNodeName = _T( "values" );
    static constexpr LPTSTR NodesNodeName = _T( "nodes" );
public:
    TConfig( String FileName, bool Compact = true, bool ReadOnly = false,
             bool FlushAllItems = false )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_{ FileName }, compact_{ Compact }
    {
        if ( TFile::Exists( fileName_ ) ) {
            JSONObjRAII JSON{ *this };
            GetRootNode().Read( *this, String() );
        }
    }

    ~TConfig() {
        try {
            if ( !GetReadOnlyFlag() ) {
                DoFlush();
            }
        }
        catch ( ... ) {
        }
    }

    TConfig( TConfig const & ) = delete;
    TConfig& operator=( TConfig const & ) = delete;
private:
    class JSONObjRAII {
    public:
        JSONObjRAII( TConfig& Cfg ) : cfg_{ Cfg } { Cfg.CreateJSONObject(); }
        ~JSONObjRAII() noexcept {
            try { cfg_.DestroyAndCloseJSONObject(); } catch ( ... ) {}
        }
        JSONObjRAII( JSONObjRAII const & ) = delete;
        JSONObjRAII& operator=( JSONObjRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    std::unique_ptr<TJSONValue> document_;
    String fileName_;
    bool compact_;

    void CreateJSONObject() {
        if ( FileExists( fileName_ ) ) {
            document_.reset( TJSONObject::ParseJSONValue( TFile::ReadAllText( fileName_ ) ) );
            if ( document_ ) {
                return;
            }
        }
        document_.reset( new TJSONObject{} );
    }

    void DestroyAndCloseJSONObject() {
        document_.reset();
    }

    // https://www.bfilipek.com/2018/07/string-view-perf-followup.html
    #if defined( _UNICODE )
    auto SplitStringView( std::wstring_view StrView, std::wstring_view Delimiters )
    #else
    auto SplitStringView( std::string_view StrView, std::string_view Delimiters )
    #endif
    {
        using SVType = decltype( StrView );
        std::vector<SVType> Result;

        for ( size_t First {} ; First < StrView.size() ; ) {
            const auto Second = StrView.find_first_of( Delimiters, First );

            if ( First != Second ) {
                Result.emplace_back( StrView.substr( First, Second - First ) );
            }

            if ( Second == SVType::npos ) {
                break;
            }

            First = Second + 1;
        }
        return Result;
    }

    auto SplitString( String Text, String Delimiters ) {
    #if defined( _UNICODE )
        return SplitStringView(
            std::wstring_view( Text.c_str(), Text.Length() ),
            std::wstring_view( Delimiters.c_str(), Delimiters.Length() )
        );
    #else
        return SplitStringView(
            std::string_view( Text.c_str(), Text.Length() ),
            std::string_view( Delimiters.c_str(), Delimiters.Length() )
        );
    #endif
    }

    TJSONObject* OpenPath( String Path, bool Create = false ) {
        auto Node = dynamic_cast<TJSONObject*>( document_.get() );
        if ( Node ) {
            for ( auto const & Item : SplitString( Path, _T( "\\" ) ) ) {
                auto const NodeName = String( Item.data(), Item.size() );
                if ( auto Inner = Node->FindValue( NodeName ) ) {
                    if ( auto ANode = dynamic_cast<TJSONObject*>( Inner ) ) {
                        Node = ANode;
                    }
                    else {
                        throw Exception(
                            _T( "\'%s' is not a JSON object for path \'%s\'" ),
                            ARRAYOFCONST(( NodeName, Path ))
                        );
                    }
                }
                else if ( Create ) {
                    auto InnerObj = std::make_unique<TJSONObject>();
                    Node->AddPair( NodeName, InnerObj.get() );
                    Node = InnerObj.release();
                }
                else {
/*
                    throw Exception(
                        _T( "\'%s' doesn't exists in path \'%s\'" ),
                        ARRAYOFCONST(( NodeName, Path ))
                    );
*/
                    return nullptr;
                }
            }
            return Node;
        }
        else {
            throw Exception(
                _T( "Can't open JSON node \'%s\'" ),
                ARRAYOFCONST(( Path ))
            );
        }
    }

//rimuovere i valori top, left, right e bottom prima di riscriverli?

    class SaveVisitor : public boost::static_visitor<void> {
    public:
        SaveVisitor( TJSONObject& Obj, String Name )
            : obj_{ Obj }, name_{ Name } {}
        SaveVisitor( SaveVisitor const & ) = delete;
        SaveVisitor& operator=( SaveVisitor const & ) = delete;

        template<typename T>
        void Write( String Type, String Name, std::unique_ptr<T> Val ) const {
            auto ValueObj = std::make_unique<TJSONObject>();
            ValueObj->AddPair( Type, Val.get() );
            Val.release();
            if ( auto Pair = obj_.Get( Name ) ) {
                Pair->JsonValue = ValueObj.get();
            }
            else {
                obj_.AddPair( Name, ValueObj.get() );
            }
            ValueObj.release();
        }

        template<typename C, typename T>
        void WriteNumber( String Type, String Name, T Val ) const {
            Write(
                Type, Name,
                std::make_unique<TJSONNumber>( static_cast<C>( Val ) )
            );
        }
                                                                  // -----  -----
        void operator()( int Val ) const {                        // i32    (i)
            WriteNumber<int>( _T( "i" ), name_, Val );
        }

        void operator()( unsigned int Val ) const {               // u32    (u)
            WriteNumber<__int64>( _T( "u" ), name_, Val );
        }

        void operator()( long Val ) const {                       // i32    (l)
            WriteNumber<__int64>( _T( "l" ), name_, Val );
        }

        void operator()( unsigned long Val ) const {              // u32    (ul)
            WriteNumber<__int64>( _T( "ul" ), name_, Val );
        }

        void operator()( char Val ) const {                       // i8     (c)
            WriteNumber<int>( _T( "c" ), name_, Val );
        }

        void operator()( unsigned char Val ) const {              // u8     (uc)
            WriteNumber<int>( _T( "uc" ), name_, Val );
        }

        void operator()( short Val ) const {                      // i16    (s)
            WriteNumber<int>( _T( "s" ), name_, Val );
        }

        void operator()( unsigned short Val ) const {             // u16    (us)
            WriteNumber<int>( _T( "u" ), name_, Val );
        }

        void operator()( long long Val ) const {                  // i64    (ll)
            WriteNumber<__int64>( _T( "ll" ), name_, Val );
        }

        void operator()( unsigned long long Val ) const {         // u64    (ull)
            Write(
                _T( "ull" ), name_,
                std::make_unique<TJSONString>(
#if defined( _UNICODE )
                    std::to_wstring( Val ).c_str()
#else
                    std::to_string( Val ).c_str()
#endif
                )
            );
        }

        void operator()( bool Val ) const {                       // u8     (b)
            Write( _T( "b" ), name_, std::make_unique<TJSONBool>( Val ) );
        }

        void operator()( System::UnicodeString Val ) const {      // sz     (sz)
            Write( _T( "sz" ), name_, std::make_unique<TJSONString>( Val ) );
        }

        void operator()( System::TDateTime Val ) const {          // b8     (dt)
            Write(
                _T( "dt" ), name_,
                std::make_unique<TJSONString>( DateToISO8601( Val, false ) )
            );
        }

        void operator()( float Val ) const {                      // b8     (flt)
            WriteNumber<double>( _T( "flt" ), name_, Val );
        }

        void operator()( double Val ) const {                     // b8     (dbl)
            WriteNumber<double>( _T( "dbl" ), name_, Val );
        }

        void operator()( System::Currency Val ) const {           // b8     (cur)
            TFormatSettings FS;
            FS.DecimalSeparator = _T( '.' );
            Write(
                _T( "cur" ), name_,
                std::make_unique<TJSONString>(
                    CurrToStr( Val, FS )
                )
            );
        }

        void operator()( std::vector<String> const &Val ) const { // sv     (sv)
            auto Array = std::make_unique<TJSONArray>();
            for ( auto Item : Val ) {
                Array->Add( Item );
            }
            auto ValueObj = std::make_unique<TJSONObject>();
            ValueObj->AddPair( _T( "sv" ), Array.get() );
            Array.release();
            if ( auto Pair = obj_.Get( name_ ) ) {
                Pair->JsonValue = ValueObj.get();
            }
            else {
                obj_.AddPair( name_, ValueObj.get() );
            }
            ValueObj.release();
        }

        void operator()( TBytes Val ) const {                     // dab
            Write(
                _T( "dab" ), name_,
                std::make_unique<TJSONString>(
                    TNetEncoding::Base64->EncodeBytesToString(
                        &Val[0], Val.High
                    )
                )
            );
        }

        void operator()( std::vector<Byte> const & Val ) const {  // vb     (vb)
            Write(
                _T( "vb" ), name_,
                std::make_unique<TJSONString>(
                    Val.empty() ?
                      String()
                    :
                      TNetEncoding::Base64->EncodeBytesToString(
                          Val.data(), Val.size() - 1
                      )
                )
            );
        }

    private:
        TJSONObject& obj_;
        String name_;
    };

    void SaveValue( TJSONObject& Obj, TConfigNode::ValueContType::value_type const & v ) {
        boost::apply_visitor(
            SaveVisitor{ Obj, v.first }, v.second.first
        );
    }

    void DeleteValue( TJSONObject& Obj, TConfigNode::ValueContType::value_type const & v ) {
        Obj.RemovePair( v.first );
    }

protected:
    virtual String DoGetNodePathSeparator() const override {
        return Format( _T( "\\%s\\" ), String( NodesNodeName ) );
    }

    virtual TConfigNode::ValueContType DoCreateValueList( String KeyName ) override {
        using ValueBuilderType =
            std::function<
                TConfigNodeValueType (
                    String        // Key Name
                  , TJSONValue &
                )
            >;

        static constexpr std::array<LPCTSTR,TConfigNodeValueType::types::size::value> TypeIds {
            _T( "b" ),
            _T( "c" ),
            _T( "cur" ),
            _T( "dab" ),
            _T( "dbl" ),
            _T( "dt" ),
            _T( "flt" ),
            _T( "i" ),
            _T( "l" ),
            _T( "ll" ),
            _T( "s" ),
            _T( "sv" ),
            _T( "sz" ),
            _T( "u" ),
            _T( "uc" ),
            _T( "ul" ),
            _T( "ull" ),
            _T( "us" ),
            _T( "vb" ),
        };

        using Fn = std::function<TConfigNodeValueType(TJSONValue&)>;

        static std::array<Fn,TConfigNodeValueType::types::size::value> Builders {
            // "b"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<bool>();
            },

            // "c"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return static_cast<char>( Value.GetValue<int>() );
            },

            // "cur"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                TFormatSettings FS;
                FS.DecimalSeparator = _T( '.' );
                return StrToCurr( Value.GetValue<String>(), FS );
            },

            // "dab"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return
                    TNetEncoding::Base64->DecodeStringToBytes(
                        Value.GetValue<String>()
                    );
            },

            // "dbl"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<double>();
            },

            // "dt"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return ISO8601ToDate( Value.GetValue<String>(), false );
            },

            // "flt"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<float>();
            },

            // "i"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<int>();
            },

            // "l"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return static_cast<long>( Value.GetValue<__int64>() );
            },

            // "ll"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<__int64>();
            },

            // "s"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<short>();
            },

            // "sv"
            []( TJSONValue& Value ) -> TConfigNodeValueType { return {}; },

            // "sz"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<String>();
            },

            // "u"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<unsigned>();
            },

            // "uc"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<unsigned char>();
            },

            // "ul"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return static_cast<unsigned long>( Value.GetValue<__int64>() );
            },

            // "ull"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return std::stoull( Value.GetValue<String>().c_str() );
            },

            // "us"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                return Value.GetValue<unsigned short>();
            },

            // "vb"
            []( TJSONValue& Value ) -> TConfigNodeValueType {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes(
                    Value.GetValue<String>()
                );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
                    System::begin( &Bytes ), System::end( &Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
        };

        TConfigNode::ValueContType Values;

        if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, ValuesNodeName ) ) ) {
            for ( int Idx = 0 ; Idx < Key->Count ; ++Idx ) {
                auto const & Pair = Key->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    auto Name = Pair->JsonString->Value();

                    if ( auto InnerObj = dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                        if ( InnerObj->Count == 1 ) {
                            auto const & InnerPair = InnerObj->Pairs[0];
                            auto TypeName = InnerPair->JsonString->Value();
                            auto TypeIdIt =
                                std::lower_bound(
                                    std::begin( TypeIds ),
                                    std::end( TypeIds ),
                                    TypeName,
                                    []( auto Lhs, auto Rhs ) {
                                        return String( Lhs ) < Rhs;
                                    }
                                );
                            if ( String( *TypeIdIt ) == TypeName ) {
                                auto const BuilderIdx =
                                    std::distance( std::begin( TypeIds ), TypeIdIt );
                                    auto Val = Builders[BuilderIdx]( *InnerPair->JsonValue );
                                    TConfigNode::PutItemTo(
                                        Values, Name,
                                        TConfigNode::ValuePairType{
                                            Val, TConfigNode::Operation::None
                                        }
                                    );
                            }
                            else {
                            }
                        }
                    }
                }
            }
        }
        return Values;
    }

    virtual TConfigNode::NodeContType DoCreateNodeList( String KeyName ) override {
        TConfigNode::NodeContType Nodes;

        if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, NodesNodeName ) ) ) {
            for ( int Idx = 0 ; Idx < Key->Count ; ++Idx ) {
                auto const & Pair = Key->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    auto NodeName = Pair->JsonString->Value();
                    Nodes[NodeName] = std::move( std::make_unique<TConfigNode>() );
                }
            }
        }
        return Nodes;
    }

    virtual void DoSaveValueList( String KeyName, TConfigNode::ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, ValuesNodeName ), true ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == TConfigNode::Operation::Write ) {
                        SaveValue( *Key, v );
                    }
                    else if ( v.second.second == TConfigNode::Operation::Erase ) {
                        DeleteValue( *Key, v );
                    }
                }
            }
        }
    }

    virtual void DoSaveNodeList( String KeyName, TConfigNode::NodeContType const & Nodes ) override {
        for ( auto const & n : Nodes ) {
            if ( GetAlwaysFlushNodeFlag() || n.second->IsModified() ) {
                n.second->Write(
                    *this,
                    Format(
                        _T( "%s\\%s\\%s" ), KeyName, NodesNodeName, n.first
                    )
                );
            }
        }
    }

    virtual void DoDeleteNode( String KeyName ) override {
    }

    virtual void DoFlush() override {
        JSONObjRAII JSON{ *this };
        GetRootNode().Write( *this, String() );

        if ( !TFile::Exists( fileName_ ) ) {
            auto Path = TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( Path ) ) {
                TDirectory::CreateDirectory( Path );
            }
        }
        TFile::WriteAllText(
            fileName_,
            compact_ ? document_->ToJSON() : document_->Format( 2 )
        );
    }

    virtual bool DoGetForcedWritesFlag() const {
        return false;
    }
};

//---------------------------------------------------------------------------
} // End namespace JSON
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
