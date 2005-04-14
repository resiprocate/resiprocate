#include "resiprocate/KeepAliveMessage.hxx"
#include "resiprocate/dum/KeepAliveManager.hxx"
#include "resiprocate/dum/KeepAliveTimeout.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

void KeepAliveManager::add(const Tuple& target, int keepAliveInterval)
{
   assert(mStack);
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it == mNetworkAssociations.end())
   {
      InfoLog( << "First keep alive for: " << target );
      NetworkAssociationInfo info;
      info.refCount = 1;
      info.keepAliveInterval = keepAliveInterval;
      mNetworkAssociations.insert(NetworkAssociationMap::value_type(target, info));
      KeepAliveTimeout t(target);
      mStack->post(t, keepAliveInterval);
   }
   else
   {
      (*it).second.refCount++;
      if(keepAliveInterval < (*it).second.keepAliveInterval)
      {
          (*it).second.keepAliveInterval = keepAliveInterval;  // !slg! only allow value to be shortened???  What if 2 different profiles with different keepAliveTime settings are sharing this network association?
      }
   }
}

void KeepAliveManager::remove(const Tuple& target)
{
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it != mNetworkAssociations.end())
   {
      if (0 == --(*it).second.refCount)
      {
         mNetworkAssociations.erase(target);
      }
   }
}

void KeepAliveManager::process(KeepAliveTimeout& timeout)
{
   assert(mStack);
   static KeepAliveMessage msg;
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(timeout.target());
   if (it != mNetworkAssociations.end())
   {
      mStack->sendTo(msg, timeout.target());
      KeepAliveTimeout t(it->first);
      mStack->post(t, it->second.keepAliveInterval);
      InfoLog( << "Refreshing keep alive of " << it->second.keepAliveInterval << " seconds for: " << timeout.target());
   }
}
