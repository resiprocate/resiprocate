#include "resiprocate/Helper.hxx"
#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/StatisticsMessage.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/DefaultServerReferHandler.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumException.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/InviteSessionCreator.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/OutOfDialogReqCreator.hxx"
#include "resiprocate/dum/PagerMessageCreator.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/PublicationCreator.hxx"
#include "resiprocate/dum/RedirectManager.hxx"
#include "resiprocate/dum/RegistrationCreator.hxx"
#include "resiprocate/dum/ServerAuthManager.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/SubscriptionCreator.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK)// Used for tracking down memory leaks in Visual Studio
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif // defined(WIN32) && defined(_DEBUG)

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

DialogUsageManager::DialogUsageManager(SipStack& stack) :
   mProfile(0),
   mRedirectManager(new RedirectManager()),
   mInviteSessionHandler(0),
   mClientRegistrationHandler(0),
   mServerRegistrationHandler(0),
   mRedirectHandler(0),
   mDialogSetHandler(0),
   mClientPagerMessageHandler(0),
   mServerPagerMessageHandler(0),
   mAppDialogSetFactory(new AppDialogSetFactory()),
   mStack(stack),
   mStackThread(stack),
   mDumShutdownHandler(0),
   mDestroying(false)
{
   addServerSubscriptionHandler("refer", DefaultServerReferHandler::Instance());   
}

DialogUsageManager::~DialogUsageManager()
{  
   mDestroying = true;   
   DebugLog ( << "~DialogUsageManager" );
   while(!mDialogSetMap.empty())
   {
      delete mDialogSetMap.begin()->second;
   }
}

void 
DialogUsageManager::shutdown()
{
   if (mDumShutdownHandler && !mDestroying)
   {
      mDumShutdownHandler->onDumCanBeDeleted();      
   }
}


void DialogUsageManager::shutdown(DumShutdownHandler* h)
{
   mDumShutdownHandler = h;
   shutdownWhenEmpty();
}


void DialogUsageManager::setAppDialogSetFactory(AppDialogSetFactory* factory)
{
   delete mAppDialogSetFactory;   
   mAppDialogSetFactory = factory;
}

#if 0
AppDialogSet* 
DialogUsageManager::createAppDialogSet()
{
   return new AppDialogSet(*this);
}

AppDialog* 
DialogUsageManager::createAppDialog()
{
   return new AppDialog(*this);
}

ClientRegistration* 
DialogUsageManager::createAppClientRegistration(Dialog& dialog, BaseCreator& creator)
{
   return new ClientRegistration(*this, dialog, creator.getLastRequest());
}

ClientInviteSession* 
DialogUsageManager::createAppClientInviteSession(Dialog& dialog, const InviteSessionCreator& creator)
{
   return new ClientInviteSession(*this, dialog, creator.getLastRequest(), creator.getInitialOffer());   
}

ClientSubscription* 
DialogUsageManager::createAppClientSubscription(Dialog& dialog, BaseCreator& creator)
{
   return new ClientSubscription(*this, dialog, creator.getLastRequest());
}

ClientOutOfDialogReq* 
DialogUsageManager::createAppClientOutOfDialogRequest(Dialog& dialog, BaseCreator& creator)
{
   return new ClientOutOfDialogReq(*this, dialog, creator.getLastRequest());
}

ClientPublication* 
DialogUsageManager::createAppClientPublication(Dialog& dialog, BaseCreator& creator)
{
   return new ClientPublication(*this, dialog, creator.getLastRequest());
}

ServerInviteSession*
DialogUsageManager::createAppServerInviteSession()
{
   return 0;
}

ServerSubscription* 
DialogUsageManager::createAppServerSubscription()
{
   return 0;
}

ServerPublication* 
DialogUsageManager::createAppServerPublication()
{
   return 0;
}

ServerRegistration* 
DialogUsageManager::createAppServerRegistration()
{
   return 0;
}

ServerOutOfDialogReq* 
DialogUsageManager::createAppServerOutOfDialogRequest()
{
   return 0;
}
#endif


Profile* 
DialogUsageManager::getProfile()
{
   return mProfile;
}

void DialogUsageManager::setProfile(Profile* profile)
{
   mProfile = profile;
}

void DialogUsageManager::setRedirectManager(RedirectManager* manager)
{
   delete mRedirectManager;
   mRedirectManager = manager;
}

void DialogUsageManager::setRedirectHandler(RedirectHandler* handler)
{
   mRedirectHandler = handler;
}
   
RedirectHandler* DialogUsageManager::getRedirectHandler()
{
   return mRedirectHandler;
}

void 
DialogUsageManager::setClientAuthManager(std::auto_ptr<ClientAuthManager> manager)
{
   mClientAuthManager = manager;
}

void 
DialogUsageManager::setServerAuthManager(std::auto_ptr<ServerAuthManager> manager)
{
   mServerAuthManager = manager;
}

void 
DialogUsageManager::setClientRegistrationHandler(ClientRegistrationHandler* handler)
{
   assert(!mClientRegistrationHandler);
   mClientRegistrationHandler = handler;
}

void 
DialogUsageManager::setServerRegistrationHandler(ServerRegistrationHandler* handler)
{
   assert(!mServerRegistrationHandler);
   mServerRegistrationHandler = handler;
}

void 
DialogUsageManager::setDialogSetHandler(DialogSetHandler* handler)
{
   mDialogSetHandler = handler;   
}

void 
DialogUsageManager::setInviteSessionHandler(InviteSessionHandler* handler)
{
   assert(!mInviteSessionHandler);
   mInviteSessionHandler = handler;
}

void 
DialogUsageManager::addTimer(DumTimeout::Type type, unsigned long duration, 
                             BaseUsageHandle target, int cseq, int rseq)
{
   DumTimeout t(type, duration, target, cseq, rseq);
   mStack.post(t, duration);
}

void 
DialogUsageManager::addTimerMs(DumTimeout::Type type, unsigned long duration, 
                               BaseUsageHandle target, int cseq, int rseq)
{
   DumTimeout t(type, duration, target, cseq, rseq);
   mStack.postMS(t, duration);
}

void 
DialogUsageManager::addClientSubscriptionHandler(const Data& eventType, ClientSubscriptionHandler* handler)
{
   assert(handler);
   assert(mClientSubscriptionHandlers.count(eventType) == 0);
   mClientSubscriptionHandlers[eventType] = handler;
}

void 
DialogUsageManager::addServerSubscriptionHandler(const Data& eventType, ServerSubscriptionHandler* handler)
{
   assert(handler);
   //default do-nothing server side refer handler can be replaced
   if (eventType == "refer")
   {
      delete mServerSubscriptionHandlers[eventType];      
   }
   else
   {
      assert(mServerSubscriptionHandlers.count(eventType) == 0);
   }
   mServerSubscriptionHandlers[eventType] = handler;
}

void 
DialogUsageManager::addClientPublicationHandler(const Data& eventType, ClientPublicationHandler* handler)
{
   assert(handler);
   assert(mClientPublicationHandlers.count(eventType) == 0);
   mClientPublicationHandlers[eventType] = handler;
}

void 
DialogUsageManager::addServerPublicationHandler(const Data& eventType, ServerPublicationHandler* handler)
{
   assert(handler);
   assert(mServerPublicationHandlers.count(eventType) == 0);
   mServerPublicationHandlers[eventType] = handler;
}

void 
DialogUsageManager::addOutOfDialogHandler(MethodTypes type, OutOfDialogHandler* handler)
{
   assert(handler);
   assert(mOutOfDialogHandlers.count(type) == 0);
   mOutOfDialogHandlers[type] = handler;
}

void 
DialogUsageManager::setClientPagerMessageHandler(ClientPagerMessageHandler* handler)
{
   mClientPagerMessageHandler = handler;
}
    
void 
DialogUsageManager::setServerPagerMessageHandler(ServerPagerMessageHandler* handler)
{
   mServerPagerMessageHandler = handler;
}

DialogSet* 
DialogUsageManager::makeUacDialogSet(BaseCreator* creator, AppDialogSet* appDs)
{
   if (mDumShutdownHandler)
   {
      throw new DumException("Cannot create new sessions when DUM is shutting down.", __FILE__, __LINE__);
   }
      
   if (appDs == 0)
   {
      appDs = new AppDialogSet(*this);
   }
   prepareInitialRequest(creator->getLastRequest());
   DialogSet* ds = new DialogSet(creator, *this);
   
   appDs->mDialogSetId = ds->getId();
   ds->mAppDialogSet = appDs;
   
   DebugLog ( << "************* Adding DialogSet ***************" ); 
   DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
   mDialogSetMap[ds->getId()] = ds;
   DebugLog ( << "After: " << Inserter(mDialogSetMap) );
   return ds;   
}

SipMessage& 
DialogUsageManager::makeNewSession(BaseCreator* creator, AppDialogSet* appDs)
{
   makeUacDialogSet(creator, appDs);   
   return creator->getLastRequest();
}

void 
DialogUsageManager::makeResponse(SipMessage& response, 
                                 const SipMessage& request, 
                                 int responseCode, 
                                 const Data& reason) const
{
   assert(request.isRequest());
   Helper::makeResponse(response, request, responseCode, reason);
}

void
DialogUsageManager::sendResponse(SipMessage& response)
{
   assert(response.isResponse());
   mStack.send(response);
}
   

SipMessage&
DialogUsageManager::makeInviteSession(const NameAddr& target, const NameAddr& from, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this, target, from, initialOffer), appDs);
   return inv;
}

SipMessage&
DialogUsageManager::makeInviteSessionFromRefer(const SipMessage& refer,
                                               ServerSubscriptionHandle serverSub, 
                                               const SdpContents* initialOffer, 
                                               AppDialogSet* appDs)
{
   //generate and send 100
   SipFrag contents;
   contents.message().header(h_StatusLine).statusCode() = 100;   
   contents.message().header(h_StatusLine).reason() = "Trying";
   //will be cloned...ServerSub may not have the most efficient API possible
   serverSub->setSubscriptionState(Active);   
   SipMessage& notify = serverSub->update(&contents);
//   mInviteSessionHandler->onReadyToSend(InviteSessionHandle::NotValid(), notify);
   serverSub->send(notify);

   //19.1.5
   NameAddr target = refer.header(h_ReferTo);
   target.uri().embedded() = SipMessage();   
   target.uri().remove(p_method);
   
   //could pass dummy target, then apply merge rules from 19.1.5...or
   //makeNewSession would use rules from 19.1.5
   NameAddr from  = serverSub->mDialog.mLocalNameAddr;
   from.remove(p_tag);   
   
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this, 
                                                             target,                                                                                                                  from,
                                                             initialOffer, serverSub), appDs);

   if (refer.exists(h_ReferredBy))
   {
      inv.header(h_ReferredBy) =  refer.header(h_ReferredBy);
   }

   const Uri& referTo = refer.header(h_ReferTo).uri();
   //19.1.5
   if (referTo.hasEmbedded() && referTo.embedded().exists(h_Replaces))      
   {
      inv.header(h_Replaces) = referTo.embedded().header(h_Replaces);
   }
   return inv;
}



SipMessage&
DialogUsageManager::makeSubscription(const NameAddr& target, const NameAddr& from, const Data& eventType, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, from, eventType), appDs);
}

SipMessage& 
DialogUsageManager::makeRegistration(const NameAddr& target, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, target), appDs); 
}

SipMessage& 
DialogUsageManager::makePublication(const NameAddr& targetDocument,  
                                    const NameAddr& from, 
                                    const Contents& body, 
                                    const Data& eventType, 
                                    unsigned expiresSeconds, 
                                    AppDialogSet* appDs)
{ 
   return makeNewSession(new PublicationCreator(*this, targetDocument, from, body, eventType, expiresSeconds), appDs); 
}

SipMessage& 
DialogUsageManager::makeOutOfDialogRequest(const NameAddr& target, const NameAddr& from, const MethodTypes meth, AppDialogSet* appDs)
{
	return makeNewSession(new OutOfDialogReqCreator(*this, meth, target, from), appDs);
}

ClientPagerMessageHandle 
DialogUsageManager::makePagerMessage(const NameAddr& target, const NameAddr& from, AppDialogSet* appDs)
{
   if (!mClientPagerMessageHandler)
   {
      throw new DumException("Cannot send MESSAGE messages without a ClientPagerMessageHandler", __FILE__, __LINE__);
   }
   DialogSet* ds = makeUacDialogSet(new PagerMessageCreator(*this, target, from), appDs);
   ClientPagerMessage* cpm = new ClientPagerMessage(*this, *ds);
   ds->mClientPagerMessage = cpm;
   return cpm->getHandle();
}

void
DialogUsageManager::send(SipMessage& msg)
{
   DebugLog (<< "SEND: " << msg);
   if (msg.isRequest())
   {
      if (getProfile()->hasUserAgent())
      {
         msg.header(h_UserAgent).value() = getProfile()->getUserAgent();
      }      
         
      //this is all very scary and error-prone, as the TU has some retramissions
      if (msg.header(h_RequestLine).method() != CANCEL &&
          msg.header(h_RequestLine).method() != ACK && 
          msg.exists(h_Vias))
      {
         msg.header(h_Vias).front().param(p_branch).reset();
      }

      if (msg.exists(h_Vias) && !mProfile->rportEnabled())
      {
         msg.header(h_Vias).front().remove(p_rport);
      }
      
      if (mClientAuthManager.get() && msg.header(h_RequestLine).method() != ACK)
      {
         mClientAuthManager->addAuthentication(msg);
      }
      
      //copy message if processStrictRoute will modify it
      if (msg.exists(h_Routes) && 
          !msg.header(h_Routes).empty() &&
          !msg.header(h_Routes).front().uri().exists(p_lr))
      {
         SipMessage copyOfMessage(msg);
         Helper::processStrictRoute(copyOfMessage);
         sendUsingOutboundIfAppropriate(copyOfMessage);          
      }
      else
      {
         sendUsingOutboundIfAppropriate(msg);          
      }
   }   
   else
   {
      sendResponse(msg);
   }   
}

void 
DialogUsageManager::sendUsingOutboundIfAppropriate(SipMessage& msg)
{
   if (getProfile()->hasOutboundProxy())
   {
      DebugLog ( << "Using outbound proxy");
      mStack.sendTo(msg, getProfile()->getOutboundProxy().uri());         
   }
   else
   {
      mStack.send(msg);
   }
}


void
DialogUsageManager::cancel(DialogSetId setid)
{
   DialogSet* ds = findDialogSet(setid);
   if (ds == 0)
   {
      throw Exception("Request no longer exists", __FILE__, __LINE__);
   }
   else
   {
      ds->cancel();
   }
}


// !jf! maybe this should just be a handler that the application can provide
// (one or more of) to futz with the request before it goes out
void
DialogUsageManager::prepareInitialRequest(SipMessage& request)
{
   // !jf! 
   //request.header(h_Supporteds) = mProfile->getSupportedOptionTags();
   //request.header(h_Allows) = mProfile->getAllowedMethods();
}

void 
DialogUsageManager::buildFdSet(FdSet& fdset)
{
   mStack.buildFdSet(fdset);   
}

int 
DialogUsageManager::getTimeTillNextProcessMS()
{
   return mStack.getTimeTillNextProcessMS();   
}

Dialog* 
DialogUsageManager::findDialog(const DialogId& id)
{
   DialogSet* ds = findDialogSet(id.getDialogSetId());
   if (ds)
   {
      return ds->findDialog(id);
   }
   else
   {
      return 0;
   }
}


InviteSessionHandle 
DialogUsageManager::findInviteSession(DialogId id)
{
   Dialog* dialog = findDialog(id);
   if (dialog && dialog->mInviteSession)
   {
      return dialog->mInviteSession->getSessionHandle();
   }
   else
   {
      return InviteSessionHandle::NotValid();
   }
}

pair<InviteSessionHandle, int> 
DialogUsageManager::findInviteSession(CallId replaces)
{
   //486/481 decision making logic where?  App may not wish to keep track of
   //invitesession state
   return make_pair(findInviteSession(DialogId(replaces.value(), 
                                               replaces.param(p_toTag), 
                                               replaces.param(p_fromTag))),
                    481);
}

void
DialogUsageManager::process(bool separateThread)
{
   if (separateThread)
   {
      static bool started = false;
      if (!started)
      {
         mStackThread.run();
         started = true;
      }
   }

   try 
   {
      std::auto_ptr<Message> msg( mStack.receiveAny() );
      SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
      if (!msg.get())  return;
      if (sipMsg)
      {
         DebugLog ( << "DialogUsageManager::process: " << sipMsg->brief());      
         if (sipMsg->isRequest())
         {

//          if( !validateRequest(*sipMsg) )
//          {
//             DebugLog (<< "Failed request validation " << *sipMsg);
//             return;
//          }
//          if ( !validateTo(*sipMsg) )
//          {
//             DebugLog (<< "Failed to validation " << *sipMsg);
//             return;
//          }
            if (sipMsg->header(h_From).exists(p_tag))
            {
               if (mergeRequest(*sipMsg) )
               {
                  InfoLog (<< "Merged request: " << *sipMsg);
                  return;
               }
            }
         
            if ( mServerAuthManager.get() )
            { 
               if ( mServerAuthManager->handle(*sipMsg) )
               {
                  return;
               }
            }
            processRequest(*sipMsg);
         }
         else if (sipMsg->isResponse())
         {
            processResponse(*sipMsg);
         }
         return;
      }

      DumTimeout* dumMsg = dynamic_cast<DumTimeout*>(msg.get());
      if (dumMsg )
      {
         if ( !dumMsg->getBaseUsage().isValid())
         {
            return;
         }
      
         dumMsg->getBaseUsage()->dispatch(*dumMsg);
         return;
      }

      StatisticsMessage* stats = dynamic_cast<StatisticsMessage*>(msg.get());
      if (stats)
      {
         static StatisticsMessage::Payload payload;
         stats->loadOut(payload);
         stats->logStats(RESIPROCATE_SUBSYSTEM, payload);
      }
      
      // !jf! might want to do something with StatisticsMessage
      //ErrLog(<<"Unknown message received." << msg->brief());
      //assert(0);
   }
   catch(BaseException& e)
   {
      //unparseable, bad 403 w/ 2543 trans it from FWD, etc
	  ErrLog(<<"Illegal message rejected: " << e.getMessage());
   }
   
}

void
DialogUsageManager::process(FdSet& fdset)
{
   try 
   {
      mStack.process(fdset);
      process(false);
   }
   catch(BaseException& e)
   {
      //unparseable, bad 403 w/ 2543 trans it from FWD, etc
	  ErrLog(<<"Illegal message rejected: " << e.getMessage());
   }
}

bool
DialogUsageManager::validateRequest(const SipMessage& request)
{
   if (!mProfile->isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      SipMessage failure;
      makeResponse(failure, request, 416);
      sendResponse(failure);
   }
   else if (!mProfile->isMethodSupported(request.header(h_RequestLine).getMethod()))
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());

      SipMessage failure;
      makeResponse(failure, request, 405);
      failure.header(h_Allows) = mProfile->getAllowedMethods();
      sendResponse(failure);

      return false;
   }
   else
   {
      Tokens unsupported = mProfile->isSupported(request.header(h_Requires));
      if (!unsupported.empty())
      {
         InfoLog (<< "Received an unsupported option tag(s): " << request.brief());

         SipMessage failure;
         makeResponse(failure, request, 420);
         failure.header(h_Unsupporteds) = unsupported;
         sendResponse(failure);

         return false;
      }
      else if (request.exists(h_ContentDisposition))
      {
         if (request.header(h_ContentDisposition).exists(p_handling) && 
             isEqualNoCase(request.header(h_ContentDisposition).param(p_handling), Symbols::Required))
         {
            if (!mProfile->isMimeTypeSupported(request.header(h_ContentType)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());

               SipMessage failure;
               makeResponse(failure, request, 415);
               failure.header(h_Accepts) = mProfile->getSupportedMimeTypes();
               sendResponse(failure);

               return false;
            }
            else if (!mProfile->isContentEncodingSupported(request.header(h_ContentEncoding)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               SipMessage failure;
               makeResponse(failure, request, 415);
               failure.header(h_AcceptEncodings) = mProfile->getSupportedEncodings();
               sendResponse(failure);
            }
            else if (!mProfile->isLanguageSupported(request.header(h_ContentLanguages)))
            {
               InfoLog (<< "Received an unsupported language: " << request.brief());

               SipMessage failure;
               makeResponse(failure, request, 415);
               failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               sendResponse(failure);
            }
         }
      }
   }
         
   return true;
}


bool
DialogUsageManager::validateTo(const SipMessage& request)
{
   // !jf! check that the request is targeted at me!
   // will require support in profile
   // This should check the Request-Uri (not the To)
   return true;
}

bool
DialogUsageManager::mergeRequest(const SipMessage& request)
{
   assert(request.isRequest());
   assert(request.isExternal());
   
   if (!request.header(h_To).exists(p_tag))
   {
      if (mMergedRequests.count(MergedRequestKey(request)))
      {
         SipMessage failure;
         makeResponse(failure, request, 482, "Merged Request");
         failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
         sendResponse(failure);
         return true;
      }
   }
   
   return false;
}

void
DialogUsageManager::processRequest(const SipMessage& request)
{
   DebugLog ( << "DialogUsageManager::processRequest: " << request.brief());
   
   assert(mAppDialogSetFactory);
   if (!request.header(h_To).exists(p_tag))
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case ACK:
            DebugLog (<< "Discarding request: " << request.brief());
            break;
                        
         case PRACK:
         case BYE:
         case UPDATE:
            //case INFO: // !rm! in an ideal world
            //case NOTIFY: // !rm! in an ideal world
         {
            SipMessage failure;
            makeResponse(failure, request, 481);
            failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
            sendResponse(failure);
            break;
         }
         case CANCEL:
         {
            // find the appropropriate ServerInvSession
            DialogSet* ds = findDialogSet(DialogSetId(request));
            if (ds == 0)
            {
               //!dcm! -- temporary hack...do a map by TID?
               for (DialogSetMap::iterator it = mDialogSetMap.begin(); it != mDialogSetMap.end(); it++)
               {
                  if (it->second->getId().getCallId() == request.header(h_CallID).value())
                  {
                     it->second->dispatch(request);
                     return;
                  }
               }
               InfoLog (<< "Received a CANCEL on a non-existent transaction ");
               SipMessage failure;
               makeResponse(failure, request, 481);
               sendResponse(failure);
            }
            else
            {
               ds->dispatch(request);
            }
            break;
         }

         case SUBSCRIBE:
         case PUBLISH:
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
            if (!checkEventPackage(request))
            {
               return;
            }
         case INVITE:   // new INVITE
         case REFER:    // out-of-dialog REFER
         case INFO :    // handle non-dialog (illegal) INFOs
         case OPTIONS : // handle non-dialog OPTIONS
         case MESSAGE :
         {
            {
               DialogSetId id(request);
               //cryptographically dangerous
               assert(mDialogSetMap.find(id) == mDialogSetMap.end()); 
            }
            if (mDumShutdownHandler)
            {
               SipMessage forbidden;
               makeResponse(forbidden, request, 480);
               forbidden.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               sendResponse(forbidden);
               return;
            }
            try
            {               
               DialogSet* dset =  new DialogSet(request, *this);

               DebugLog ( << "*********** Calling AppDialogSetFactory *************"  );
               AppDialogSet* appDs = mAppDialogSetFactory->createAppDialogSet(*this, request);
               appDs->mDialogSetId = dset->getId();
               dset->mAppDialogSet = appDs;

               DebugLog ( << "************* Adding DialogSet ***************" ); 
               DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
               mDialogSetMap[dset->getId()] = dset;
               DebugLog ( << "After: " << Inserter(mDialogSetMap) );
               
               dset->dispatch(request);
            }
            catch (BaseException& e)
            {
               SipMessage failure;
               makeResponse(failure, request, 400, e.getMessage());
               failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               sendResponse(failure);
            }
            
            break;
         }
         case REGISTER:
         {
               SipMessage failure;
               makeResponse(failure, request, 405);
               failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               sendResponse(failure);
         }
         break;         
         case RESPONSE:
         case SERVICE:
            assert(false);
            break;
         case UNKNOWN:
         case MAX_METHODS:
            assert(false);
            break;
      }
   }
   else // in a specific dialog
   {
      switch (request.header(h_RequestLine).getMethod())
      {
         case REGISTER:
         {
            SipMessage failure;
            makeResponse(failure, request, 400, "rjs says, Go to hell");
            failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
            sendResponse(failure);
            break;
         }
         
         default:
         {
            DialogSet* ds = findDialogSet(DialogSetId(request));
            if (ds == 0)
            {
               SipMessage failure;
               makeResponse(failure, request, 481);
               failure.header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               InfoLog (<< "Rejected request (which was in a dialog) " << request.brief());
               sendResponse(failure);
            }
            else
            {
               ds->dispatch(request);
            }
         }
      }
   }
}

void
DialogUsageManager::processResponse(const SipMessage& response)
{
   DebugLog ( << "DialogUsageManager::processResponse: " << response);
   if (/*response.header(h_StatusLine).statusCode() > 100 && */response.header(h_CSeq).method() != CANCEL)
   {
      DialogSet* ds = findDialogSet(DialogSetId(response));
  
      if (ds)
      {
         DebugLog ( << "DialogUsageManager::processResponse: " << response.brief());
         ds->dispatch(response);
      }
      else
      {
         InfoLog (<< "Throwing away stray response: " << response.brief());
      }
   }   
}

bool 
DialogUsageManager::checkEventPackage(const SipMessage& request)
{
   int failureCode = 0;   
   MethodTypes method = request.header(h_RequestLine).method();

//       || (method == NOTIFY && !request.exists(h_SubscriptionState)))
   if (!request.exists(h_Event))
   {
      failureCode = 400;
   }
   else 
   {
      switch(method)
      {
         case SUBSCRIBE:
            if (getServerSubscriptionHandler(request.header(h_Event).value()))
            {
               failureCode = 489;
            }
            break;
         case NOTIFY:
            if (getClientSubscriptionHandler(request.header(h_Event).value()))
            {
               failureCode = 489;
            }
            break;
         case PUBLISH:
            assert(0);
         default:
            assert(0);
      }
   }
   
   if (failureCode > 0)
   {
      SipMessage response;
      makeResponse(response, request, failureCode);
      send(response);
      return false;
   }
   return true;   
}
 
DialogSet*
DialogUsageManager::findDialogSet(const DialogSetId& id)
{
   DebugLog ( << "Looking for dialogSet: " << id << " in map:" );
   DebugLog ( << Inserter(mDialogSetMap) );   
   DialogSetMap::const_iterator it = mDialogSetMap.find(id);
   
    if (it == mDialogSetMap.end())
    {
       return 0;
    }
    else
    {
       return it->second;
    }
}

BaseCreator*
DialogUsageManager::findCreator(const DialogId& id)
{
   DialogSet* ds = findDialogSet(id.getDialogSetId());
   if (ds)
   {
      return ds->getCreator();
   }
   else 
   {
      return 0;
   }
}

void 
DialogUsageManager::removeDialogSet(const DialogSetId& dsId)
{
   DebugLog ( << "************* Removing DialogSet ***************" ); 
   DebugLog ( << "Before: " << Inserter(mDialogSetMap) );
   mDialogSetMap.erase(dsId);
   DebugLog ( << "After: " << Inserter(mDialogSetMap) );
   if (mRedirectManager)
   {
      mRedirectManager->removeDialogSet(dsId);
   }   
}

ClientSubscriptionHandler* 
DialogUsageManager::getClientSubscriptionHandler(const Data& eventType)
{
   map<Data, ClientSubscriptionHandler*>::iterator res = mClientSubscriptionHandlers.find(eventType);
   if (res != mClientSubscriptionHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ServerSubscriptionHandler* 
DialogUsageManager::getServerSubscriptionHandler(const Data& eventType)
{
   map<Data, ServerSubscriptionHandler*>::iterator res = mServerSubscriptionHandlers.find(eventType);
   if (res != mServerSubscriptionHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ClientPublicationHandler* 
DialogUsageManager::getClientPublicationHandler(const Data& eventType)
{
   map<Data, ClientPublicationHandler*>::iterator res = mClientPublicationHandlers.find(eventType);
   if (res != mClientPublicationHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

ServerPublicationHandler* 
DialogUsageManager::getServerPublicationHandler(const Data& eventType)
{
   map<Data, ServerPublicationHandler*>::iterator res = mServerPublicationHandlers.find(eventType);
   if (res != mServerPublicationHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}

OutOfDialogHandler*   
DialogUsageManager::getOutOfDialogHandler(const MethodTypes type)
{
   map<MethodTypes, OutOfDialogHandler*>::iterator res = mOutOfDialogHandlers.find(type);
   if (res != mOutOfDialogHandlers.end())
   {
      return res->second;
   }
   else
   {
      return 0;
   }
}





/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
