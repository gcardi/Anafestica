//---------------------------------------------------------------------------

#ifndef PersistFormBaseH
#define PersistFormBaseH

#include <System.Classes.hpp>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

template<typename B>
class TPersistBaseForm : public B {
public:
    enum class StoreOpts {
        None, OnlySize, OnlyPos, PosAndSize,
        OnlyState, StateAndSize, StateAndPos, All
    };

	template<typename...A>
	__fastcall TPersistBaseForm( A...Args )
      : B( std::forward<A...>( Args )... ) {}
private:
    StoreOpts storeOptions_;
};

//---------------------------------------------------------------------------
} // End of namespace Anafestica 
//---------------------------------------------------------------------------

#endif