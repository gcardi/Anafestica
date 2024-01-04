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
#include <vector>
#include <array>
#include <functional>
#include <iterator>

#include <anafestica/FileVersionInfo.h>
#include <anafestica/Cfg.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSON {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
private:
    static constexpr LPCTSTR ValuesNodeName = _D( "values" );
    static constexpr LPCTSTR NodesNodeName = _D( "nodes" );
public:
    TConfig( String FileName, bool ReadOnly = false, bool Compact = true,
             bool FlushAllItems = false, bool ExplicitTypes = false )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_{ FileName }, compact_{ Compact }
        , explicitTypes_{ ExplicitTypes }
    {
        if ( TFile::Exists( fileName_ ) ) {
            JSONObjRAII JSON{ *this };
            GetRootNode().Read( *this, TConfigPath{} );
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
    bool explicitTypes_;

    void CreateJSONObject() {
        document_.reset();
        if ( TFile::Exists( fileName_ ) ) {
            document_.reset(
                TJSONObject::ParseJSONValue( TFile::ReadAllText( fileName_ ) )
            );
        }
        if ( !document_ ) {
            document_.reset( new TJSONObject{} );
        }
    }

    void DestroyAndCloseJSONObject() {
        document_.reset();
    }

    TJSONObject* OpenPath( TConfigPath const & Path ) {
        if ( auto Node = dynamic_cast<TJSONObject*>( document_.get() ) ) {
            for ( auto const & NodeName : Path ) {
                if ( auto NodesInner = Node->FindValue( NodesNodeName ) ) {
                    if ( auto Inner = NodesInner->FindValue( NodeName ) ) {
                        if ( auto NodeObj = dynamic_cast<TJSONObject*>( Inner ) ) {
                            Node = NodeObj;
                            continue;
                        }
                    }
                }
                return nullptr;
            }
            return Node;
        }
        return nullptr;
    }

    TJSONObject* OpenNodes( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            if ( auto Inner = Node->FindValue( NodesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
        }
        return nullptr;
    }

    TJSONObject* OpenValues( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            if ( auto Inner = Node->FindValue( ValuesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
        }
        return nullptr;
    }

    TJSONObject* ForcePath( TConfigPath const & Path ) {
        if ( auto Node = dynamic_cast<TJSONObject*>( document_.get() ) ) {
            TConfigPath FPath;
            FPath.reserve( Path.size() * 2 );
            for ( auto const & NodeName : Path ) {
                FPath.push_back( NodesNodeName );
                FPath.push_back( NodeName );
            }
            for ( auto const & NodeName : FPath ) {
                if ( auto Inner = Node->FindValue( NodeName ) ) {
                    if ( auto ANode = dynamic_cast<TJSONObject*>( Inner ) ) {
                        Node = ANode;
                    }
                }
                else {
                    auto InnerObj = std::make_unique<TJSONObject>();
                    Node->AddPair( NodeName, InnerObj.get() );
                    Node = InnerObj.release();
                }
            }
            return Node;
        }
        return nullptr;
    }

    TJSONObject* ForceValues( TConfigPath const & Path ) {
        if ( auto Node = ForcePath( Path ) ) {
            if ( auto Inner = Node->FindValue( ValuesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
            else {
                auto InnerObj = std::make_unique<TJSONObject>();
                Node->AddPair( ValuesNodeName, InnerObj.get() );
                return InnerObj.release();
            }
        }
        return nullptr;
    }

    template<typename J, typename T>
    static void Write( J& Obj, String Name, std::unique_ptr<T> Val ) {
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = Val.release();
        }
        else {
            Obj.AddPair( Name, Val.release() );
        }
    }

    template<typename J, typename T>
    static void Write( J& Obj, String Type, String Name, std::unique_ptr<T> Val ) {
        auto ValueObj = std::make_unique<TJSONObject>();
        ValueObj->AddPair( Type, Val.get() );
        Val.release();
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = ValueObj.release();
        }
        else {
            Obj.AddPair( Name, ValueObj.release() );
        }
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
            Pair->JsonValue = ValueObj.release();
        }
        else {
            Obj.AddPair( Name, ValueObj.release() );
        }
    }

    // https://www.bfilipek.com/2019/02/2lines3featuresoverload.html
    //template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    //template<class... Ts> overload( Ts... ) -> overload<Ts...>;

    // https://andreasfertig.blog/2023/07/visiting-a-stdvariant-safely/
	template<class...>
	constexpr bool always_false_v = false;

	template<class... Ts>
	struct overload : Ts...
	{
	  using Ts::operator()...;

	  // Prevent implicit type conversions
	  template<typename T>
	  constexpr void operator()(T) const
	  {
		static_assert(always_false_v<T>, "Unsupported type");
	  }
	};

	template<class... Ts>
	overload(Ts...) -> overload<Ts...>;

    std::unique_ptr<TBase64Encoding> base64_ { new TBase64Encoding{ 0 } };

    void SaveValue( TJSONObject& Obj, ValueContType::value_type const & v ) {
        boost::apply_visitor(
            overload {
                [this, &Obj, &v]( int Val ) {
                    if ( explicitTypes_ ) {
                        WriteNumber<int>( Obj, cnv_xstr( TT_I ), v.first, Val );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONNumber>( Val )
                        );
                    }
                },

                [&Obj, &v]( unsigned int Val ) {
                    WriteNumber<__int64>( Obj, cnv_xstr( TT_U ), v.first, Val );
                },

                [&Obj, &v]( long Val ) {
                    WriteNumber<__int64>( Obj, cnv_xstr( TT_L ), v.first, Val );
                },

                [&Obj, &v]( unsigned long Val ) {
                    WriteNumber<__int64>( Obj, cnv_xstr( TT_UL ), v.first, Val );
                },

                [&Obj, &v]( char Val ) {
                    WriteNumber<int>( Obj, cnv_xstr( TT_C ), v.first, Val );
                },

                [&Obj, &v]( unsigned char Val ) {
                    WriteNumber<int>( Obj, cnv_xstr( TT_UC ), v.first, Val );
                },

                [&Obj, &v]( short Val ) {
                    WriteNumber<int>( Obj, cnv_xstr( TT_S ), v.first, Val );
                },

                [&Obj, &v]( unsigned short Val ) {
                    WriteNumber<int>( Obj, cnv_xstr( TT_US ), v.first, Val );
                },

                [&Obj, &v]( long long Val ) {
                    WriteNumber<__int64>( Obj, cnv_xstr( TT_LL ), v.first, Val );
                },

                [&Obj, &v]( unsigned long long Val ) {
                    Write(
                        Obj, cnv_xstr( TT_ULL ), v.first,
                        std::make_unique<TJSONString>(
#if defined( _UNICODE )
                            std::to_wstring( Val ).c_str()
#else
                            std::to_string( Val ).c_str()
#endif
                        )
                    );
                },

                [this, &Obj, &v]( bool Val ) {
                    if ( explicitTypes_ ) {
                        Write(
                            Obj, cnv_xstr( TT_B ), v.first,
                            std::make_unique<TJSONBool>( Val )
                        );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONBool>( Val )
                        );
                    }
                },

                [this, &Obj, &v]( System::UnicodeString Val ) {
                    if ( explicitTypes_ ) {
                        Write(
                            Obj, cnv_xstr( TT_SZ ), v.first,
                            std::make_unique<TJSONString>( Val )
                        );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONString>( Val )
                        );
                    }
                },

                [&Obj, &v]( System::TDateTime Val ) {
                    Write(
                        Obj, cnv_xstr( TT_DT ), v.first,
                        std::make_unique<TJSONString>(
                            DateToISO8601( Val, false )
                        )
                    );
                },

                [&Obj, &v]( float Val ) {
                    WriteNumber<double>( Obj, cnv_xstr( TT_FLT ), v.first, Val );
                },

                [&Obj, &v]( double Val ) {
                    WriteNumber<double>( Obj, cnv_xstr( TT_DBL ), v.first, Val );
                },

                [&Obj, &v]( System::Currency Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    Write(
                        Obj, cnv_xstr( TT_CUR ), v.first,
                        std::make_unique<TJSONString>( CurrToStr( Val, FS ) )
                    );
                },

                [&Obj, &v]( StringCont const & Val ) {
                    WriteStrings( Obj, cnv_xstr( TT_SV ), v.first, Val );
                },

                [this, &Obj, &v]( TBytes Val ) {
                    Write(
                        Obj, cnv_xstr( TT_DAB ), v.first,
                        std::make_unique<TJSONString>(
                            //TNetEncoding::Base64->EncodeBytesToString(
                            base64_->EncodeBytesToString(
                                &Val[0], Val.High
                            )
                        )
                    );
                },

                [this,&Obj, &v]( std::vector<Byte> const & Val ) {
                    Write(
                        Obj, cnv_xstr( TT_VB ), v.first,
                        std::make_unique<TJSONString>(
                            Val.empty() ?
                              String()
                            :
                              //TNetEncoding::Base64->EncodeBytesToString(
                              base64_->EncodeBytesToString(
                                  Val.data(), Val.size() - 1
                              )
                        )
                    );
                }
            },
            v.second.first
        );
    }

protected:
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {

        using Fn = std::function<TConfigNodeValueType(TJSONValue&)>;

        static std::array<Fn,TConfigNodeValueType::types::size::value> Builders {
            // TT_I
            []( TJSONValue& Value ) {
                return Value.GetValue<int>();
            },

            // TT_U
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned>();
            },

            // TT_L
            []( TJSONValue& Value ) {
                return static_cast<long>( Value.GetValue<__int64>() );
            },

            // TT_UL
            []( TJSONValue& Value ) {
                return static_cast<unsigned long>( Value.GetValue<__int64>() );
            },

            // TT_C
            []( TJSONValue& Value ) {
                return static_cast<char>( Value.GetValue<int>() );
            },

            // TT_UC
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned char>();
            },

            // TT_S
            []( TJSONValue& Value ) {
                return Value.GetValue<short>();
            },

            // TT_US
            []( TJSONValue& Value ) {
                return Value.GetValue<unsigned short>();
            },

            // TT_LL
            []( TJSONValue& Value ) {
                return Value.GetValue<__int64>();
            },

            // TT_ULL
            []( TJSONValue& Value ) {
                return std::stoull( Value.GetValue<String>().c_str() );
            },

            // TT_B
            []( TJSONValue& Value ) {
                return Value.GetValue<bool>();
            },

            // TT_SZ
            []( TJSONValue& Value ) {
                return Value.GetValue<String>();
            },

            // TT_DT
            []( TJSONValue& Value ) {
                return ISO8601ToDate( Value.GetValue<String>(), false );
            },

            // TT_FLT
            []( TJSONValue& Value ) {
                return Value.GetValue<float>();
            },

            // TT_DBL
            []( TJSONValue& Value ) {
                return Value.GetValue<double>();
            },

            // TT_CUR
            []( TJSONValue& Value ) {
                TFormatSettings FS;
                FS.DecimalSeparator = _D( '.' );
                return StrToCurr( Value.GetValue<String>(), FS );
            },

            // TT_SV
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

            // TT_DAB
            []( TJSONValue& Value ) {
                return
                    TNetEncoding::Base64->DecodeStringToBytes(
                        Value.GetValue<String>()
                    );
            },

            // TT_VB
            []( TJSONValue& Value ) {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes(
                    Value.GetValue<String>()
                );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
					std::begin( Bytes ),
					std::end( Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
        };

        ValueContType Values;

        if ( auto Key = OpenValues( Path ) ) {
            for ( int Idx = 0 ; Idx < Key->Count ; ++Idx ) {
                auto const & Pair = Key->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    if ( auto InnerObj = dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                        if ( InnerObj->Count == 1 ) {
                            auto const & InnerPair = InnerObj->Pairs[0];
                            auto TypeName = InnerPair->JsonString->Value();
                            if ( auto Result = GetTypeTag( TypeName ) ) {
                                // Handle explicit types
                                PutItemTo(
                                    Values, Pair->JsonString->Value(),
                                    {
                                        Builders[static_cast<size_t>( Result.value() )](
                                            *InnerPair->JsonValue
                                        ),
                                        Operation::None
                                    }
                                );
                            }
                        }
                    }
                }
                // Handle implicit types
                else if ( auto Ptr = dynamic_cast<TJSONNumber*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->AsInt },
                            Operation::None
                        }
                    );
                }
                else if ( auto Ptr = dynamic_cast<TJSONString*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->Value() },
                            Operation::None
                        }
                    );
                }
                else if ( auto Ptr = dynamic_cast<TJSONBool*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->AsBoolean },
                            Operation::None
                        }
                    );
                }
            }
        }
        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;

        if ( auto Node = OpenNodes( Path ) ) {
            for ( int Idx = 0 ; Idx < Node->Count ; ++Idx ) {
                auto const & Pair = Node->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    auto NodeName = Pair->JsonString->Value();
                    Nodes[NodeName] = std::move( std::make_unique<TConfigNode>() );
                }
            }
        }
        return Nodes;
    }

    virtual void DoSaveValueList( TConfigPath const & Path, ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            if ( auto Node = ForceValues( Path ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                        SaveValue( *Node, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        Node->RemovePair( v.first );
                    }
                }
            }
        }
    }

    virtual void DoDeleteNode( TConfigPath const & Path ) override {
        if ( !Path.empty() ) {
            auto Last = std::end( Path );
            std::advance( Last, -1 );
            TConfigPath TmpPath( std::begin( Path ), Last );
            if ( auto Node = OpenNodes( TmpPath ) ) {
                Node->RemovePair( Path.back() );
            }
        }
    }

    virtual void DoFlush() override {
        JSONObjRAII JSON{ *this };
        GetRootNode().Write( *this, TConfigPath{} );

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
