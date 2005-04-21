#ifndef RESIP_KEEPALIVE_MANAGER_HXX
#define RESIP_KEEPALIVE_MANAGER_HXX

#include <map>
#include "resiprocate/os/Tuple.hxx"

namespace resip 
{

class KeepAliveTimeout;
class DialogUsageManager;

class KeepAliveManager
{
   public:
      struct NetworkAssociationInfo
      {
         int refCount;
         int keepAliveInterval;  // In seconds
      };
      typedef std::map<Tuple, NetworkAssociationInfo> NetworkAssociationMap;

      KeepAliveManager() {}
      void setDialogUsageManager(DialogUsageManager* dum) { mDum = dum; }
      void add(const Tuple& target, int keepAliveInterval);
      void remove(const Tuple& target);
      void process(KeepAliveTimeout& timeout);

   protected:
      DialogUsageManager* mDum;
      NetworkAssociationMap mNetworkAssociations;
      
};

}


#endif

