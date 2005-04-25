#if !defined(Repro_CertServer_hxx)
#define Repro_CertServer_hxx

#if defined(USE_SSL)

#include "repro/stateAgents/CertPublicationHandler.hxx"
#include "repro/stateAgents/CertSubscriptionHandler.hxx"
#include "repro/stateAgents/PrivateKeyPublicationHandler.hxx"
#include "repro/stateAgents/PrivateKeySubscriptionHandler.hxx"

namespace resip
{
class DialogUsageManager;
}

namespace repro
{

class CertServer
{
   public:
      CertServer(resip::DialogUsageManager& dum);
      ~CertServer();

   private:
      resip::DialogUsageManager& mDum;
      
      PrivateKeySubscriptionHandler mPrivateKeyServer;
      PrivateKeyPublicationHandler mPrivateKeyUpdater;
      CertSubscriptionHandler mCertServer;
      CertPublicationHandler mCertUpdater;
};

}
#endif
#endif
