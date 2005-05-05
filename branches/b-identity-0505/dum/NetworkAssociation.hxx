#ifndef RESIP_NETWORKASSOCIATION_HXX
#define RESIP_NETWORKASSOCIATION_HXX

#include "resiprocate/os/Tuple.hxx"

namespace resip 
{

class SipMessage;
class DialogUsageManager;

class NetworkAssociation
{
   public:
      NetworkAssociation() : mDum(0) {}
      void setDum(DialogUsageManager* dum) { mDum = dum; }
      void update(const SipMessage& msg, int keepAliveInterval);
      ~NetworkAssociation();
   private:
      Tuple mTarget;
      DialogUsageManager* mDum;
      
};

}


#endif
