#ifndef RESIP_KEEPALIVE_MANAGER_HXX
#define RESIP_KEEPALIVE_MANAGER_HXX

#include <map>
#include "resiprocate/os/Tuple.hxx"

namespace resip 
{

class KeepAliveTimeout;
class SipStack;

class KeepAliveManager
{
   public:
      typedef std::map<Tuple, int> NetworkAssociationMap;

      KeepAliveManager() {}
      void setStack(SipStack* stack) { mStack = stack; }
      void add(const Tuple& target);
      void remove(const Tuple& target);
      void process(KeepAliveTimeout& timeout);

   protected:
      enum { KeepAliveInterval = 30 * 1000 };
      SipStack* mStack;
      NetworkAssociationMap mNetworkAssociations;
      
};

}


#endif
