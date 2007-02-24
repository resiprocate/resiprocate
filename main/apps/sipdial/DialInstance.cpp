

#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

#include "DialerConfiguration.hxx"
#include "DialInstance.hxx"
#include "MyInviteSessionHandler.hxx"

using namespace resip;
using namespace std;

DialInstance::DialInstance(const DialerConfiguration& dialerConfiguration, const resip::Uri& targetUri) :
   mDialerConfiguration(dialerConfiguration),
   mTargetUri(targetUri),
   mResult(Error)
{
}

DialInstance::DialResult DialInstance::execute()
{
  
   prepareAddress();

   mSipStack = new SipStack();
   mDum = new DialogUsageManager(*mSipStack);
   mDum->addTransport(UDP, 5067, V4);
   SharedPtr<MasterProfile> masterProfile = SharedPtr<MasterProfile>(new MasterProfile);
   mDum->setMasterProfile(masterProfile);
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);
   mDum->setClientAuthManager(clientAuth);
   MyInviteSessionHandler *ish = new MyInviteSessionHandler(*this);
   mDum->setInviteSessionHandler(ish);

   sendInvite();

   while(mSipStack != 0) 
   {
      FdSet fdset;
      mSipStack->buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(resipMin((int)mSipStack->getTimeTillNextProcessMS(), 50));
      if(err == -1) {
         if(errno != EINTR) {
            //B2BUA_LOG_ERR("fdset.select returned error code %d", err);
            assert(0);  // FIXME
         }
      }
      // Process all SIP stack activity
      mSipStack->process(fdset);
      while(mDum->process());
      if(mProgress == Connected && mClient->isConnected()) 
      {
         mClient->refer(NameAddr(mFullTarget));
         mProgress = ReferSent;
      }
      
      if(mProgress == Done)
      {
         delete mDum;
         delete ish;
         delete mSipStack;
         mSipStack = 0;
      }
   }

   return mResult;

}

void DialInstance::prepareAddress() 
{
   if(mTargetUri.scheme() == Symbols::Sip) {
      mFullTarget = mTargetUri;
      return;
   }

   if(mTargetUri.scheme() == Symbols::Tel) {
      Data num = processNumber(mTargetUri.user());
      if(num.size() < 1)
      {
         // FIXME - check size
         assert(0);
      }
      if(num[0] == '+')
      {
         // E.164
         mFullTarget = Uri("sip:" + mDialerConfiguration.getTargetPrefix() + num.substr(1, num.size() - 1) + "@" + mDialerConfiguration.getTargetDomain());
         return;
      }
      mFullTarget = Uri("sip:" + num + "@" + mDialerConfiguration.getTargetDomain());
      return;
   }

   // FIXME Unsupported scheme 
   assert(0);
}

void DialInstance::sendInvite() 
{
   SharedPtr<UserProfile> outboundUserProfile(mDum->getMasterUserProfile());
   outboundUserProfile->setDefaultFrom(mDialerConfiguration.getDialerIdentity());
   outboundUserProfile->setDigestCredential(mDialerConfiguration.getAuthRealm(), mDialerConfiguration.getAuthUser(), mDialerConfiguration.getAuthPassword());
   SharedPtr<SipMessage> msg = mDum->makeInviteSession(NameAddr(mDialerConfiguration.getCallerUserAgentAddress()), outboundUserProfile, 0);
   HeaderFieldValue *hfv = 0;
   switch(mDialerConfiguration.getCallerUserAgentVariety())
   {
   case DialerConfiguration::Generic:
      break;
   case DialerConfiguration::LinksysSPA941:
      hfv = new HeaderFieldValue("\\;answer-after=0", 16);
      msg->header(h_CallInfos).push_back(GenericUri(hfv, Headers::CallInfo));
      break;
   case DialerConfiguration::PolycomIP501:
      hfv = new HeaderFieldValue("AA", 2);
      msg->header(h_AlertInfos).push_back(GenericUri(hfv, Headers::AlertInfo));
      break;
   case DialerConfiguration::Cisco7940:
      break;
   default:
      break;
   }
   mDum->send(msg);
   if(hfv != 0)
      delete hfv;
}

// Get rid of punctuation like `.' and `-'
// Keep a leading `+' if present
// assert if not a real number
Data DialInstance::processNumber(const Data& verboseNumber)
{
   Data num = Data("");
   int len = verboseNumber.size();
   for(int i = 0; i < len; i++)
   {
      char c = verboseNumber[i];
      switch(c)
      {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         num.append(&c, 1);
         break;
      case '+':
         assert(i == 0);   // FIXME - better error handling needed
         num.append(&c, 1);
         break;
      case '.':
      case '-':
         // just ignore those characters
         break;
      default:
         // any other character is garbage
         assert(0);
      }
   }
   return num;
}

void DialInstance::onFailure()
{
   mResult = ReferUnsuccessful;
   mProgress = Done;
}

void DialInstance::onConnected(ClientInviteSessionHandle cis) 
{
   mClient = cis;
   mProgress = Connected;
}

void DialInstance::onReferSuccess()
{
   mResult = ReferSuccessful;
   mProgress = Done;
}

void DialInstance::onReferFailed()
{
   mResult = ReferUnsuccessful;
   mProgress = Done;
}

void DialInstance::onTerminated()
{
   mProgress = Done;
}

