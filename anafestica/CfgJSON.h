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

    template<typename J, typename T>
    static void Write( J& Obj, String Type, String Name, std::unique_ptr<T> Val ) {
        auto ValueObj = std::make_unique<TJSONObject>();
        ValueObj->AddPair( Type, Val.get() );
        Val.release();
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = ValueObj.get();
        }
        else {
            Obj.AddPair( Name, ValueObj.get() );
        }
        ValueObj.release();
    }

    template<typename C, typename J, typename T>
    static void WriteNumber( J& Obj, String Type, String Name, T Val ) {
        Write(
            Obj, Type, Name,
            std::make_unique<TJSONNumber>( static_cast<C>( Val ) )
        );
    }

    template<typename J, typename T>
    static void WriteStrings( J& Obj, String Type, String Name, T&& Val ) {
        auto Array = std::make_unique<TJSONArray>();
        for ( auto Item : Val ) {
            Array->Add( Item );
        }
        auto ValueObj = std::make_unique<TJSONObject>();
        ValueObj->AddPair( Type, Array.get() );
        Array.release();
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = ValueObj.get();
        }
        else {
            Obj.AddPair( Name, ValueObj.get() );
        }
        ValueObj.release();
    }

    // https://www.bfilipek.com/2019/02/2lines3featuresoverload.html
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload( Ts... ) -> overload<Ts...>;

    void SaveValue( TJSONObject& Obj, ValueContType::value_type const & v ) {
        boost::apply_visitor(
            overload {
                [&Obj, &v]( int Val ) {
                    WriteNumber<int>( Obj, _T( "i" ), v.first, Val );
                },

                [&Obj, &v]( unsigned int Val ) {
                    WriteNumber<__int64>( Obj, _T( "u" ), v.first, Val );
                },

                [&Obj, &v]( long Val ) {
                    WriteNumber<__int64>( Obj, _T( "l" ), v.first, Val );
                },

                [&Obj, &v]( unsigned long Val ) {
                    WriteNumber<__int64>( Obj, _T( "ul" ), v.first, Val );
                },

                [&Obj, &v]( char Val ) {
                    WriteNumber<int>( Obj, _T( "c" ), v.first, Val );
                },

                [&Obj, &v]( unsigned char Val ) {
                    WriteNumber<int>( Obj, _T( "uc" ), v.first, Val );
                },

                [&Obj, &v]( short Val ) {
                    WriteNumber<int>( Obj, _T( "s" ), v.first, Val );
                },

                [&Obj, &v]( unsigned short Val ) {
                    WriteNumber<int>( Obj, _T( "us" ), v.first, Val );
                },

                [&Obj, &v]( long long Val ) {
                    WriteNumber<__int64>( Obj, _T( "ll" ), v.first, Val );
                },

                [&Obj, &v]( unsigned long long Val ) {
                    Write(
                        Obj, _T( "ull" ), v.first,
                        std::make_unique<TJSONString>(
#if defined( _UNICODE )
                            std::to_wstring( Val ).c_str()
#else
                            std::to_string( Val ).c_str()
#endif
                        )
                    );
                },

                [&Obj, &v]( bool Val ) {
                    Write(
                        Obj, _T( "b" ), v.first,
                        std::make_unique<TJSONBool>( Val )
                    );
                },

                [&Obj, &v]( System::UnicodeString Val ) {
                    Write(
                        Obj, _T( "sz" ), v.first,
                        std::make_unique<TJSONString>( Val )
                    );
                },

                [&Obj, &v]( System::TDateTime Val ) {
                    Write(
                        Obj, _T( "dt" ), v.first,
                        std::make_unique<TJSONString>(
                            DateToISO8601( Val, false )
                        )
                    );
                },

                [&Obj, &v]( float Val ) {
                    WriteNumber<double>( Obj, _T( "flt" ), v.first, Val );
                },

                [&Obj, &v]( double Val ) {
                    WriteNumber<double>( Obj, _T( "dbl" ), v.first, Val );
                },

                [&Obj, &v]( System::Currency Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _T( '.' );
                    Write(
                        Obj, _T( "cur" ), v.first,
                        std::make_unique<TJSONString>( CurrToStr( Val, FS ) )
                    );
                },

                [&Obj, &v]( StringCont const & Val ) {
                    WriteStrings( Obj, _T( "sv" ), v.first, Val );
                },

                [&Obj, &v]( TBytes Val ) {
                    Write(
                        Obj, _T( "dab" ), v.first,
                        std::make_unique<TJSONString>(
                            TNetEncoding::Base64->EncodeBytesToString(
                                &Val[0], Val.High
                            )
                        )
                    );
                },

                [&Obj, &v]( std::vector<Byte> const & Val ) {
                    Write(
                        Obj, _T( "vb" ), v.first,
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
            },
            v.second.first
        );
    }

    void DeleteValue( TJSONObject& Obj, ValueContType::value_type const & v ) {
        Obj.RemovePair( v.first );
    }

protected:
    virtual String DoGetNodePathSeparator() const override {
        return Format( _T( "\\%s\\" ), String( NodesNodeName ) );
    }

    virtual ValueContType DoCreateValueList( String KeyName ) override {
        using ValueBuilderType =
            std::function<
                TConfigNodeValueType (
                    String        // Key Name
                  , TJSONValue &
                )
            >;

        // Must be sorted because will be used with std::lower_bound
        static constexpr std::array<LPCTSTR,TConfigNodeValueType::types::size::value> TypeIds {
            _T( "b" ),   _T( "c" ),  _T( "cur" ), _T( "dab" ),
            _T( "dbl" ), _T( "dt" ), _T( "flt" ), _T( "i" ),
            _T( "l" ),   _T( "ll" ), _T( "s" ),   _T( "sv" ),
            _T( "sz" ),  _T( "u" ),  _T( "uc" ),  _T( "ul" ),
            _T( "ull" ), _T( "us" ), _T( "vb" ),
        };

        using Fn = std::function<TConfigNodeValueType(TJSONValue&)>;

        // positions must be in sync with 'TypeIds'
        static std::array<Fn,TConfigNodeValueType::types::size::value> Builders {
            // "b"
            []( TJSONValue& Value ) {
                return Value.GetValue<bool>();
            },

            // "c"
            []( TJSONValue& Value ) {
                return static_cast<char>( Value.GetValue<int>() );
            },

            // "cur"
            []( TJSONValue& Value ) {
                TFormatSettings FS;
                FS.DecimalSeparator = _T( '.' );
                return StrToCurr( Value.GetValue<String>(), FS );
            },

            // "dab"
            []( TJSONValue& Value ) {
                return
                    TNetEncoding::Base64->DecodeStringToBytes(
                        Value.GetValue<String>()
                    );
            },

            // "dbl"
            []( TJSONValue& Value ) {
                return Value.GetValue<double>();
            },

            // "dt"
            []( TJSONValue& Value ) {
                return ISO8601ToDate( Value.GetValue<String>(), false );
            },

            // "flt"
            []( TJSONValue& Value ) {
                return Value.GetValue<float>();
            },

            // "i"
            []( TJSONValue& Value ) {
                return Value.GetValue<int>();
            },

            // "l"
            []( TJSONValue& Value ) {
                return static_cast<long>( Value.GetValue<__int64>() );
            },

            // "ll"
            []( TJSONValue& Value ) {
                return Value.GetValue<__int64>();
            },

            // "s"
            []( TJSONValue& Value ) {
                return Value.GetValue<short>();
            },

            // "sv"
            []( TJSONValue& Value ) {
                StringCont Strings;
                if ( auto JSONArr = dynamic_cast<TJSONArray*>( &Value ) ) {
                    Strings.reserve( JSONArr->Count );
                    std::transform(
                        System::begin( JSONArr ), System::end( JSONArr ),
                        std::back_inserter( Strings ),
                        []( auto Val ){ return Val->template GetValue<String>(); }
                    );
                }
                return Strings;
            },

            // "sz"
            []( TJSONValue& Value ) {
                return Value.GetValue<String>();
            },

            // "u"
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned>();
            },

            // "uc"
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned char>();
            },

            // "ul"
            []( TJSONValue& Value ) {
                return static_cast<unsigned long>( Value.GetValue<__int64>() );
            },

            // "ull"
            []( TJSONValue& Value ) {
                return std::stoull( Value.GetValue<String>().c_str() );
            },

            // "us"
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned short>();
            },

            // "vb"
            []( TJSONValue& Value ) {
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

        ValueContType Values;

        if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, ValuesNodeName ) ) ) {
            for ( int Idx = 0 ; Idx < Key->Count ; ++Idx ) {
                auto const & Pair = Key->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    if ( auto InnerObj = dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                        if ( InnerObj->Count == 1 ) {
                            auto const & InnerPair = InnerObj->Pairs[0];
                            auto TypeName = InnerPair->JsonString->Value();
                            auto TypeIdIt =
                                std::lower_bound(
                                    std::begin( TypeIds ), std::end( TypeIds ),
                                    TypeName,
                                    []( auto Lhs, auto Rhs ) {
                                        return String( Lhs ) < Rhs;
                                    }
                                );
                            if ( String( *TypeIdIt ) == TypeName ) {
                                auto const BuilderIdx =
                                    std::distance( std::begin( TypeIds ), TypeIdIt );
                                    auto Val = Builders[BuilderIdx]( *InnerPair->JsonValue );
                                    PutItemTo(
                                        Values, Pair->JsonString->Value(),
                                        { Val, Operation::None }
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

    virtual NodeContType DoCreateNodeList( String KeyName ) override {
        NodeContType Nodes;

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

    virtual void DoSaveValueList( String KeyName, ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, ValuesNodeName ), true ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                        SaveValue( *Key, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        DeleteValue( *Key, v );
                    }
                }
            }
        }
    }

    virtual void DoSaveNodeList( String KeyName, NodeContType const & Nodes ) override {
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
