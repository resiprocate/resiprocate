#include "resiprocate/KeepAliveMessage.hxx"
#include "resiprocate/dum/KeepAliveManager.hxx"
#include "resiprocate/dum/KeepAliveTimeout.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

void KeepAliveManager::add(const Tuple& target)
{
   assert(mStack);
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it == mNetworkAssociations.end())
   {
      InfoLog( << "First keep alive for: " << target );
      mNetworkAssociations.insert(NetworkAssociationMap::value_type(target, 0));
      KeepAliveTimeout t(target);
      mStack->postMS(t, KeepAliveInterval);
   }
   else
   {
      (*it).second++;
   }
}

void KeepAliveManager::remove(const Tuple& target)
{
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it != mNetworkAssociations.end())
   {
      if (0 == --(*it).second)
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
      mStack->postMS(t, KeepAliveInterval);
      InfoLog( << "Refreshing keep alive for: " << timeout.target());
   }
}
