#include "resip/stack/WarningCategory.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "tfm/TestEndPoint.hxx"
#include "tfm/CommonAction.hxx"
#include "TestInviteSession.hxx"
#include "DumUserAgent.hxx"
#include "DumExpect.hxx"
#include "DumUaAction.hxx"

#include "resip/dum/Handles.hxx"
#include "resip/stack/SdpContents.hxx"

#include <functional>

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

TestInviteSession::TestInviteSession(DumUserAgent* ua)
   : TestUsage(ua)
{
   //mUa->add(this);
}

CommonAction* 
TestInviteSession::provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level, resip::SdpContents* alternative)
{
   void (InviteSession::*fpProvideOffer) (const resip::Contents&, resip::DialogUsageManager::EncryptionLevel, const resip::Contents*) = &InviteSession::provideOffer;
   return new CommonAction(mUa, "InviteSession::provideOffer",
                           std::bind(fpProvideOffer, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)),
                                       offer, level, alternative));
}

CommonAction* 
TestInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "InviteSession::provideAnswer",
                           std::bind(&InviteSession::provideAnswer, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)),
                                       answer));
}

CommonAction* 
TestInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::InviteSession::*fn)(resip::InviteSession::EndReason) = &InviteSession::end;
   return new CommonAction(mUa, "InviteSession::end",
                           std::bind(fn, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), reason));
}
 
CommonAction* 
TestInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "InviteSession::reject",
                           std::bind(&InviteSession::reject, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)),
                                       statusCode, warning));
}
 
CommonAction* 
TestInviteSession::requestOffer()
{
   return new CommonAction(mUa, "requestOffer",
                           std::bind(&InviteSession::requestOffer, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle))));
}
  
CommonAction* 
TestInviteSession::targetRefresh(const resip::NameAddr& localUri)
{
   return new CommonAction(mUa, "targetRefresh",
                           std::bind(&InviteSession::targetRefresh, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), localUri));
}
 
CommonAction* 
TestInviteSession::refer(const resip::NameAddr& referTo, bool referSub)
{
   return new CommonAction(mUa, "refer",
                           std::bind(static_cast<void(InviteSession::*)(const NameAddr&, bool)>(&InviteSession::refer), std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), referTo, referSub));
}

CommonAction*
TestInviteSession::refer(const resip::NameAddr& referTo, resip::InviteSessionHandle sessionToReplace, bool referSub)
{
   void (InviteSession::*refer)(const NameAddr&, InviteSessionHandle, bool) = &InviteSession::refer;
   return new CommonAction(mUa, "refer", std::bind(refer, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), 
                                                                                                        std::ref(mSessionHandle)),
                                                     referTo, sessionToReplace, referSub));
}
 
CommonAction* 
TestInviteSession::info(const resip::Contents& contents)
{
   return new CommonAction(mUa, "info",
                           std::bind(&InviteSession::info, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), std::ref(contents)));
}
 
CommonAction*
TestInviteSession:: message(const resip::Contents& contents)
{
   return new CommonAction(mUa, "message",
                           std::bind(&InviteSession::message, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), std::ref(contents)));
}

CommonAction* 
TestInviteSession::acceptNIT(int statusCode, const resip::Contents* contents)
{
   return new CommonAction(mUa, "acceptNIT",
                           std::bind(&InviteSession::acceptNIT, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), 
                                       statusCode, contents));
}
 
CommonAction* 
TestInviteSession::rejectNIT(int statusCode)
{
   return new CommonAction(mUa, "rejectNIT",
                           std::bind(&InviteSession::rejectNIT, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), statusCode));
}

CommonAction*
TestInviteSession::acceptReferNoSub(int code)
{
   return new CommonAction(mUa, "acceptReferNoSub",
                           std::bind(&InviteSession::acceptReferNoSub, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), code));
}

CommonAction*
TestInviteSession::rejectReferNoSub(int code)
{
   return new CommonAction(mUa, "rejectReferNoSub",
                           std::bind(&InviteSession::rejectReferNoSub, std::bind<InviteSession*>(static_cast<InviteSession*(InviteSessionHandle::*)()>(&InviteSessionHandle::get), std::ref(mSessionHandle)), code));
}

SendingAction<resip::ServerSubscriptionHandle>* 
TestInviteSession::acceptRefer(int statusCode)
{
   InfoLog(<< "acceptRefer");
   return new SendingAction<ServerSubscriptionHandle>(mUa, mServerSubscription, "acceptRefer", 
                                                      std::bind(&ServerSubscription::accept, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mServerSubscription)), statusCode), 
                                                      NoAdornment::instance());
}
 
SendingAction<resip::ServerSubscriptionHandle>* 
TestInviteSession::rejectRefer(int responseCode)
{
   InfoLog(<< "rejectRefer");
   return new SendingAction<ServerSubscriptionHandle>(mUa, mServerSubscription, "rejectRefer",
                                                      std::bind(&ServerSubscription::reject, 
                                                                  std::bind<ServerSubscription*>(static_cast<ServerSubscription*(ServerSubscriptionHandle::*)()>(&ServerSubscriptionHandle::get), std::ref(mServerSubscription)), responseCode),
                                                      NoAdornment::instance());
}

/*
SendingCommand*
TestInviteSession::inviteFromRefer(const SdpContents* contents)
{
   InfoLog(<< "inviteFromRefer");
   return new SendingCommand(getDumUserAgent(), "makeInviteSessionFromRefer",
                             std::bind(&DialogUsageManager::makeInviteSessionFromRefer, getDumUserAgent()->getDum(),
                                         (mReferMessage), (mServerSubscription), contents));
}
*/

bool
TestClientInviteSession::isMyEvent(Event* e)
{
   StackLog(<< "TestClientInviteSession::isMyEvent");
   ClientInviteEvent* client = dynamic_cast<ClientInviteEvent*>(e);
   if (client)
   {
      StackLog(<< "ClientInviteEvent");
      return client->getHandle() == mHandle;
   }

   InviteEvent* inv = dynamic_cast<InviteEvent*>(e);
   if (inv)
   {
      StackLog(<< "InviteEvent");
      StackLog(<< "invite session handle id: " << mSessionHandle.getId() << " " 
              << "Compared invite session handle id: " << inv->getHandle().getId() << " " << *e);
      return  inv->getHandle() == mSessionHandle;
   }

   StackLog(<< "not a InviteEvent");
   return false;
}

bool
TestServerInviteSession::isMyEvent(Event* e)
{
   StackLog(<< "TestServerInviteSession::isMyEvent");
   ServerInviteEvent* server = dynamic_cast<ServerInviteEvent*>(e);
   if (server)
   {
      if (mHandle.isValid())
      {
         StackLog(<< "My handle id: " << mHandle.getId() << " " 
                 << "Compared handle id: " << server->getHandle().getId() << " " << *e);
         return server->getHandle() == mHandle;
      }
      else
      {
         StackLog(<< "Handle has not been bound yet: " << *e);
         return true; //!dcm! -- not ideal, may have to suffice until dumv2
      }
   }

   InviteEvent* inv = dynamic_cast<InviteEvent*>(e);
   if (inv)
   {
      if (mHandle.isValid())
      {
         StackLog(<< "My handle id: " << mSessionHandle.getId() << " " 
                 << "Compared handle id: " << inv->getHandle().getId() << " " << *e);
         return inv->getHandle() == mSessionHandle;
      }
      else
      {
         StackLog(<< "Handle has not been bound yet: " << *e);
         return true; //!dcm! -- not ideal, may have to suffice until dumv2
      }
   }
   
   StackLog(<< "not a InviteEvent");
   return false;
}

TestClientInviteSession::TestClientInviteSession(DumUserAgent* ua)
   : TestInviteSession(ua)
{
}

TestClientInviteSession::ExpectBase* 
TestClientInviteSession::expect(InviteEvent::Type t,
                                MessageMatcher* matcher,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

TestClientInviteSession::ExpectBase* 
TestClientInviteSession::expect(InviteEvent::Type t,
                                ExpectPreCon& pred,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestClientInviteSession::ExpectBase* 
TestClientInviteSession::expect(InviteEvent::Type t,
                                MessageMatcher* matcher,
                                ExpectPreCon& pred,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}

CommonAction* 
TestClientInviteSession::provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level, resip::SdpContents* alternative)
{
   return new CommonAction(mUa, "ClientInviteSession::provideOffer",
                           std::bind(static_cast<void(ClientInviteSession::*)(const Contents&, DialogUsageManager::EncryptionLevel, const Contents*)>(&ClientInviteSession::provideOffer), std::bind<ClientInviteSession*>(static_cast<ClientInviteSession*(ClientInviteSessionHandle::*)()>(&ClientInviteSessionHandle::get), std::ref(mHandle)),
                                       offer, level, alternative));
}

CommonAction* 
TestClientInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "ClientInviteSession::provideAnswer",
                           std::bind(&ClientInviteSession::provideAnswer, std::bind<ClientInviteSession*>(static_cast<ClientInviteSession*(ClientInviteSessionHandle::*)()>(&ClientInviteSessionHandle::get), std::ref(mHandle)),
                                       answer));
}

CommonAction* 
TestClientInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::ClientInviteSession::*fn)(resip::InviteSession::EndReason) = &ClientInviteSession::end;
   return new CommonAction(mUa, "ClientInviteSession::end",
                           std::bind(fn, std::bind<ClientInviteSession*>(static_cast<ClientInviteSession*(ClientInviteSessionHandle::*)()>(&ClientInviteSessionHandle::get), std::ref(mHandle)), 
                                       reason));
}
 
CommonAction* 
TestClientInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "ClientInviteSession::reject",
                           std::bind(&ClientInviteSession::reject, std::bind<ClientInviteSession*>(static_cast<ClientInviteSession*(ClientInviteSessionHandle::*)()>(&ClientInviteSessionHandle::get), std::ref(mHandle)),
                                       statusCode, warning));
}

TestServerInviteSession::TestServerInviteSession(DumUserAgent* ua)
   : TestInviteSession(ua)
{
}

TestServerInviteSession::ExpectBase* 
TestServerInviteSession::expect(InviteEvent::Type t,
                                MessageMatcher* matcher,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        *TestEndPoint::AlwaysTruePred,
                        timeoutMs,
                        expectAction);
}

TestServerInviteSession::ExpectBase* 
TestServerInviteSession::expect(InviteEvent::Type t,
                                ExpectPreCon& pred,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        0,
                        pred,
                        timeoutMs,
                        expectAction);
}

TestServerInviteSession::ExpectBase* 
TestServerInviteSession::expect(InviteEvent::Type t,
                                MessageMatcher* matcher,
                                ExpectPreCon& pred,
                                int timeoutMs,
                                ActionBase* expectAction)
{
   return new DumExpect(*this,
                        new DumEventMatcherSpecific<InviteEvent>(t),
                        matcher,
                        pred,
                        timeoutMs,
                        expectAction);
}


CommonAction* 
TestServerInviteSession::provideOffer(const resip::SdpContents& offer, resip::DialogUsageManager::EncryptionLevel level, resip::SdpContents* alternative, bool sendOfferAtAccept)
{
   return new CommonAction(mUa, "ServerInviteSession::provideOffer",
                           std::bind(static_cast<void(ServerInviteSession::*)(const Contents&, DialogUsageManager::EncryptionLevel, const Contents*, bool)>(&ServerInviteSession::provideOffer), std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                                       offer, level, alternative, sendOfferAtAccept));
}

CommonAction* 
TestServerInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "ServerInviteSession::provideAnswer",
                           std::bind(&ServerInviteSession::provideAnswer, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                                       answer));
}
 
CommonAction* 
TestServerInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::ServerInviteSession::*fn)(resip::InviteSession::EndReason) = &ServerInviteSession::end;
   return new CommonAction(mUa, "ServerInviteSession::end",
                           std::bind(fn, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                              reason));
}

CommonAction* 
TestServerInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "ServerInviteSession::reject",
                           std::bind(&ServerInviteSession::reject, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                                       statusCode, warning));
}
 
CommonAction* 
TestServerInviteSession::accept(int statusCode)
{
   return new CommonAction(mUa, "ServerInviteSession::accept",
                           std::bind(&ServerInviteSession::accept, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                                       statusCode));
}
 
CommonAction* 
TestServerInviteSession::redirect(const resip::NameAddrs& contacts, int code)
{
   return new CommonAction(mUa, "ServerInviteSession::redirect",
                           std::bind(&ServerInviteSession::redirect, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), std::ref(mHandle)),
                                       contacts, code));
}

CommonAction*
TestServerInviteSession::provisional(int code, bool earlyFlag)
{
   return new CommonAction(mUa, "ServerInviteSession::provisional",
                           std::bind(&ServerInviteSession::provisional, std::bind<ServerInviteSession*>(static_cast<ServerInviteSession*(ServerInviteSessionHandle::*)()>(&ServerInviteSessionHandle::get), 
                                                                                                            std::ref(mHandle)), code, earlyFlag));
}
