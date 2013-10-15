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

#include <boost/bind.hpp>
#include <boost/function.hpp>

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
                           boost::bind(fpProvideOffer, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)),
                                       offer, level, alternative));
}

CommonAction* 
TestInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "InviteSession::provideAnswer",
                           boost::bind(&InviteSession::provideAnswer, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)),
                                       answer));
}

CommonAction* 
TestInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::InviteSession::*fn)(resip::InviteSession::EndReason) = &InviteSession::end;
   return new CommonAction(mUa, "InviteSession::end",
                           boost::bind(fn, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), reason));
}
 
CommonAction* 
TestInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "InviteSession::reject",
                           boost::bind(&InviteSession::reject, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)),
                                       statusCode, warning));
}
 
CommonAction* 
TestInviteSession::requestOffer()
{
   return new CommonAction(mUa, "requestOffer",
                           boost::bind(&InviteSession::requestOffer, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle))));
}
  
CommonAction* 
TestInviteSession::targetRefresh(const resip::NameAddr& localUri)
{
   return new CommonAction(mUa, "targetRefresh",
                           boost::bind(&InviteSession::targetRefresh, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), localUri));
}
 
CommonAction* 
TestInviteSession::refer(const resip::NameAddr& referTo, bool referSub)
{
   return new CommonAction(mUa, "refer",
                           boost::bind(&InviteSession::refer, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), referTo, referSub));
}

CommonAction*
TestInviteSession::refer(const resip::NameAddr& referTo, resip::InviteSessionHandle sessionToReplace, bool referSub)
{
   void (InviteSession::*refer)(const resip::NameAddr&, resip::InviteSessionHandle, bool) = &InviteSession::refer;
   return new CommonAction(mUa, "refer", boost::bind(refer, boost::bind<InviteSession*>(&InviteSessionHandle::get, 
                                                                                                        boost::ref(mSessionHandle)),
                                                     referTo, sessionToReplace, referSub));
}
 
CommonAction* 
TestInviteSession::info(const resip::Contents& contents)
{
   return new CommonAction(mUa, "info",
                           boost::bind(&InviteSession::info, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), boost::ref(contents)));
}
 
CommonAction*
TestInviteSession:: message(const resip::Contents& contents)
{
   return new CommonAction(mUa, "message",
                           boost::bind(&InviteSession::message, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), boost::ref(contents)));
}

CommonAction* 
TestInviteSession::acceptNIT(int statusCode, const resip::Contents* contents)
{
   return new CommonAction(mUa, "acceptNIT",
                           boost::bind(&InviteSession::acceptNIT, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), 
                                       statusCode, contents));
}
 
CommonAction* 
TestInviteSession::rejectNIT(int statusCode)
{
   return new CommonAction(mUa, "rejectNIT",
                           boost::bind(&InviteSession::rejectNIT, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), statusCode));
}

CommonAction*
TestInviteSession::acceptReferNoSub(int code)
{
   return new CommonAction(mUa, "acceptReferNoSub",
                           boost::bind(&InviteSession::acceptReferNoSub, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), code));
}

CommonAction*
TestInviteSession::rejectReferNoSub(int code)
{
   return new CommonAction(mUa, "rejectReferNoSub",
                           boost::bind(&InviteSession::rejectReferNoSub, boost::bind<InviteSession*>(&InviteSessionHandle::get, boost::ref(mSessionHandle)), code));
}

SendingAction<resip::ServerSubscriptionHandle>* 
TestInviteSession::acceptRefer(int statusCode)
{
   InfoLog(<< "acceptRefer");
   return new SendingAction<ServerSubscriptionHandle>(mUa, mServerSubscription, "acceptRefer", 
                                                      boost::bind(&ServerSubscription::accept, 
                                                                  boost::bind<ServerSubscription*>(&ServerSubscriptionHandle::get, boost::ref(mServerSubscription)), statusCode), 
                                                      NoAdornment::instance());
}
 
SendingAction<resip::ServerSubscriptionHandle>* 
TestInviteSession::rejectRefer(int responseCode)
{
   InfoLog(<< "rejectRefer");
   return new SendingAction<ServerSubscriptionHandle>(mUa, mServerSubscription, "rejectRefer",
                                                      boost::bind(&ServerSubscription::reject, 
                                                                  boost::bind<ServerSubscription*>(&ServerSubscriptionHandle::get, boost::ref(mServerSubscription)), responseCode),
                                                      NoAdornment::instance());
}

/*
SendingCommand*
TestInviteSession::inviteFromRefer(const SdpContents* contents)
{
   InfoLog(<< "inviteFromRefer");
   return new SendingCommand(getDumUserAgent(), "makeInviteSessionFromRefer",
                             boost::bind(&DialogUsageManager::makeInviteSessionFromRefer, getDumUserAgent()->getDum(),
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
                           boost::bind(&ClientInviteSession::provideOffer, boost::bind<ClientInviteSession*>(&ClientInviteSessionHandle::get, boost::ref(mHandle)),
                                       offer, level, alternative));
}

CommonAction* 
TestClientInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "ClientInviteSession::provideAnswer",
                           boost::bind(&ClientInviteSession::provideAnswer, boost::bind<ClientInviteSession*>(&ClientInviteSessionHandle::get, boost::ref(mHandle)),
                                       answer));
}

CommonAction* 
TestClientInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::ClientInviteSession::*fn)(resip::InviteSession::EndReason) = &ClientInviteSession::end;
   return new CommonAction(mUa, "ClientInviteSession::end",
                           boost::bind(fn, boost::bind<ClientInviteSession*>(&ClientInviteSessionHandle::get, boost::ref(mHandle)), 
                                       reason));
}
 
CommonAction* 
TestClientInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "ClientInviteSession::reject",
                           boost::bind(&ClientInviteSession::reject, boost::bind<ClientInviteSession*>(&ClientInviteSessionHandle::get, boost::ref(mHandle)),
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
                           boost::bind(&ServerInviteSession::provideOffer, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                                       offer, level, alternative, sendOfferAtAccept));
}

CommonAction* 
TestServerInviteSession::provideAnswer(const resip::SdpContents& answer)
{
   return new CommonAction(mUa, "ServerInviteSession::provideAnswer",
                           boost::bind(&ServerInviteSession::provideAnswer, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                                       answer));
}
 
CommonAction* 
TestServerInviteSession::end(resip::InviteSession::EndReason reason)
{
   void (resip::ServerInviteSession::*fn)(resip::InviteSession::EndReason) = &ServerInviteSession::end;
   return new CommonAction(mUa, "ServerInviteSession::end",
                           boost::bind(fn, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                              reason));
}

CommonAction* 
TestServerInviteSession::reject(int statusCode, resip::WarningCategory* warning)
{
   return new CommonAction(mUa, "ServerInviteSession::reject",
                           boost::bind(&ServerInviteSession::reject, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                                       statusCode, warning));
}
 
CommonAction* 
TestServerInviteSession::accept(int statusCode)
{
   return new CommonAction(mUa, "ServerInviteSession::accept",
                           boost::bind(&ServerInviteSession::accept, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                                       statusCode));
}
 
CommonAction* 
TestServerInviteSession::redirect(const resip::NameAddrs& contacts, int code)
{
   return new CommonAction(mUa, "ServerInviteSession::redirect",
                           boost::bind(&ServerInviteSession::redirect, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, boost::ref(mHandle)),
                                       contacts, code));
}

CommonAction*
TestServerInviteSession::provisional(int code, bool earlyFlag)
{
   return new CommonAction(mUa, "ServerInviteSession::provisional",
                           boost::bind(&ServerInviteSession::provisional, boost::bind<ServerInviteSession*>(&ServerInviteSessionHandle::get, 
                                                                                                            boost::ref(mHandle)), code, earlyFlag));
}
