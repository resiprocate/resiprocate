#if !defined(RESIP_STATELESS_HANDLER_HXX)
#define RESIP_STATELESS_HANDLER_HXX

#include "resiprocate/os/HashMap.hxx"
namespace resip
{

class SipStack;

class StatelessHandler
{
   public:
      StatelessHandler(SipStack& stack);
      void process();
      
   private:
      // map from transactionId -> SipMessage*
      typedef HashMap<Data, SipMessage*> Map;
      typedef Map::iterator MapIterator;
      typedef Map::const_iterator MapConstIterator;

      SipStack& mStack;
      Map mMap;
      unsigned long mTid;
};

}

#endif
