#if defined(USE_SSL)

#include "resip/stack/Security.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/X509Contents.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "repro/stateAgents/CertServer.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace repro;

CertServer::CertServer(DialogUsageManager& dum) : 
   mDum(dum),
   mPrivateKeyServer(*mDum.getSecurity()),
   mPrivateKeyUpdater(*mDum.getSecurity()),
   mCertServer(*mDum.getSecurity()),
   mCertUpdater(*mDum.getSecurity())
{
   MasterProfile& profile = *mDum.getMasterProfile();
   profile.addSupportedMethod(PUBLISH);
   profile.addSupportedMethod(SUBSCRIBE);
   //profile.validateAcceptEnabled() = true;  // !slg! this causes Accept validation for registration requests as well, which is not really desired
   profile.validateContentEnabled() = true;
   profile.addSupportedMimeType(PUBLISH, Pkcs8Contents::getStaticType());
   profile.addSupportedMimeType(SUBSCRIBE, Pkcs8Contents::getStaticType());
   profile.addSupportedMimeType(PUBLISH, X509Contents::getStaticType());
   profile.addSupportedMimeType(SUBSCRIBE, X509Contents::getStaticType());
         
   mDum.addServerSubscriptionHandler(Symbols::Credential, &mPrivateKeyServer);
   mDum.addServerSubscriptionHandler(Symbols::Certificate, &mCertServer);
   mDum.addServerPublicationHandler(Symbols::Credential, &mPrivateKeyUpdater);
   mDum.addServerPublicationHandler(Symbols::Certificate, &mCertUpdater);
   //setServerAuthManager(std::auto_ptr<ServerAuthManager>(new ServerAuthManager(profile)));
}

CertServer::~CertServer()
{
}

#endif

