#if !defined(RESIP_CLIENTREGISTRATION_HXX)
#define RESIP_CLIENTREGISTRATION_HXX

/** @file ClientRegistration.hxx
 *   @todo This file is empty
 */

namespace resip
{

class ClientRegistration: public BaseUsage
{
  public:

   const NameAddrs& myBindings();
   const NameAddrs& allBindings();
   void  makeMyBindingsReflect(const NameAddrs& bindings);
   void  makeAllBindingsReflect(const NameAddrs& bindings);
   void  refreshNow();

  private:

   NameAddrs mMyBindings;
   NameAddrs mAllBindings;
   UInt64    mExpirationTime;

};
 
}

#endif
