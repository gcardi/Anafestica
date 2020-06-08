//---------------------------------------------------------------------------

#ifndef CfgJSONH
#define CfgJSONH

#include <System.JSON.hpp>
#include <System.IOUtils.hpp>
#include <System.JSON.Writers.hpp>
#include <System.DateUtils.hpp>

#include <memory>
#include <string_view>
#include <vector>

#include <anafestica/Cfg.h>

template<typename T>
class TD;


//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSON {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_( FileName )
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

    void CreateJSONObject() {
        document_.reset( TJSONObject::ParseJSONValue( TFile::ReadAllText( fileName_ ) ) );
        if ( !document_ ) {
            throw Exception(
                _T( "Invalid file format for \'%s\'" ),
                ARRAYOFCONST(( fileName_ ))
            );
        }
    }

    void DestroyAndCloseJSONObject() {
        document_.reset();
    }

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

    TJSONObject& OpenKey( String Path, bool Create ) {
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
                    throw Exception(
                        _T( "\'%s' not exists in path \'%s\'" ),
                        ARRAYOFCONST(( NodeName, Path ))
                    );
                }
            }
            return *Node;
        }
        else {
            throw Exception(
                _T( "Can't open node \'%s\'" ),
                ARRAYOFCONST(( Path ))
            );
        }
    }

/*
        }
        else if ( Create ) {
            auto SL = std::make_unique<TStringList>
            OpenValue(
        }
        else {
            throw Exception(
                _T( "Node \'%s' doesn't exists" ),
                ARRAYOFCONST(( Path ))
            );
        }
*/

//rimuovere i valori top, left,right e bottom pria di riscriverli?

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

        // REG_BINARY    - Binary data in any form.
        // REG_DWORD     - 32-bit number.
        // REG_QWORD     - 64-bit number.
        // REG_EXPAND_SZ - Null-terminated string that contains unexpanded references to environment variables
        // REG_MULTI_SZ  - Array of null-terminated strings that are terminated by two null characters.
        // REG_SZ        - Null-terminated string.
                                                                  // CLASS  TAG
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
            WriteNumber<__int64>( _T( "ull" ), name_, Val );   // <- da testare
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
            Write(
                _T( "cur" ), name_,
                std::make_unique<TJSONString>( CurrToStr( Val ) )
            );
        }

        /*
        void operator()( std::shared_ptr<TStrings> Val ) const {  // sl
            auto Array = std::make_unique<TJSONArray>();
            for ( auto Item : Val.get() ) {
                Array->Add( Item );
            }
            auto ValueObj = std::make_unique<TJSONObject>();
            ValueObj->AddPair( _T( "sl" ), Array.get() );
            Array.release();
            if ( auto Pair = obj_.Get( name_ ) ) {
                Pair->JsonValue = ValueObj.get();
            }
            else {
                obj_.AddPair( name_, ValueObj.get() );
            }
            ValueObj.release();
        }
        */

        void operator()( std::vector<String> const &Val ) const { // sv     (sv)
        }

        void operator()( TBytes Val ) const {                     // dab
        }

        void operator()( std::vector<Byte> const & Val ) const {  // vb     (vb)
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
    virtual TConfigNode::ValueContType DoCreateValueList( String KeyName ) override {
        TConfigNode::ValueContType Values;

        return Values;
    }

    virtual TConfigNode::NodeContType DoCreateNodeList( String KeyName ) override {
        TConfigNode::NodeContType Nodes;
        return Nodes;
    }

    virtual void DoSaveValueList( String KeyName, TConfigNode::ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            auto& Key = OpenKey( KeyName, true );
            for ( auto& v : Values ) {
                auto const ValueState = v.second.second;
                if ( GetAlwaysFlushNodeFlag() || ValueState == TConfigNode::Operation::Write ) {
                    SaveValue( Key, v );
                }
                else if ( v.second.second == TConfigNode::Operation::Erase ) {
                    DeleteValue( Key, v );
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
                        _T( "%s\\%s" ), ARRAYOFCONST(( KeyName, n.first ))
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
        TFile::WriteAllText( fileName_, document_->Format( 2 ) );
        //TFile::WriteAllText( fileName_, document_->ToJSON() );
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
