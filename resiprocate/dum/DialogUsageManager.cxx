#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionCreator.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/PublicationCreator.hxx"
#include "resiprocate/dum/RegistrationCreator.hxx"
#include "resiprocate/dum/ServerAuthManager.hxx"
#include "resiprocate/dum/SubscriptionCreator.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DialogUsageManager::DialogUsageManager(SipStack& stack) :
   mProfile(0),
   mRedirectManager(0),
   mClientAuthManager(0),
   mServerAuthManager(0),
   mInviteSessionHandler(0),
   mClientRegistrationHandler(0),
   mServerRegistrationHandler(0),
   mAppDialogSetFactory(0),
   mStack(stack),
   mDumShutdownHandler(0)
{
}

DialogUsageManager::~DialogUsageManager()
{  
   DebugLog ( << "~DialogUsageManager" );
   // !jf! iterate through dialogsets and dispose, this will cause them to be
   // removed from HandleManager on destruction
   // !dcm! -- figure out what this means when coherent

   while(!mDialogSetMap.empty())
   {
      delete mDialogSetMap.begin()->second;
   }
   if (mDumShutdownHandler)
   {
      mDumShutdownHandler->dumDestroyed();
   }
}

void DialogUsageManager::shutdown(DumShutdownHandler* h)
{
   mDumShutdownHandler = h;
}


void DialogUsageManager::setAppDialogSetFactory(AppDialogSetFactory* factory)
{
   assert(mAppDialogSetFactory = 0);
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
   assert(!mRedirectManager);
   mRedirectManager = manager;
}

void 
DialogUsageManager::setClientAuthManager(ClientAuthManager* manager)
{
   assert(!mClientAuthManager);
   mClientAuthManager = manager;
}

void 
DialogUsageManager::setServerAuthManager(ServerAuthManager* manager)
{
   assert(!mServerAuthManager);
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
DialogUsageManager::addClientSubscriptionHandler(const Data& eventType, ClientSubscriptionHandler* handler)
{
   assert(handler);
   assert(mClientSubscriptionHandler.count(eventType) == 0);
   mClientSubscriptionHandler[eventType] = handler;
}

void 
DialogUsageManager::addServerSubscriptionHandler(const Data& eventType, ServerSubscriptionHandler* handler)
{
   assert(handler);
   assert(mServerSubscriptionHandler.count(eventType) == 0);
   mServerSubscriptionHandler[eventType] = handler;
}

void 
DialogUsageManager::addClientPublicationHandler(const Data& eventType, ClientPublicationHandler* handler)
{
   assert(handler);
   assert(mClientPublicationHandler.count(eventType) == 0);
   mClientPublicationHandler[eventType] = handler;
}

void 
DialogUsageManager::addServerPublicationHandler(const Data& eventType, ServerPublicationHandler* handler)
{
   assert(handler);
   assert(mServerPublicationHandler.count(eventType) == 0);
   mServerPublicationHandler[eventType] = handler;
}

void 
DialogUsageManager::addOutOfDialogHandler(MethodTypes& type, OutOfDialogHandler* handler)
{
   assert(handler);
   assert(mOutOfDialogHandler.count(type) == 0);
   mOutOfDialogHandler[type] = handler;
}

SipMessage& 
DialogUsageManager::makeNewSession(BaseCreator* creator, AppDialogSet* appDs)
{
   if (appDs == 0)
   {
      appDs = new AppDialogSet(*this);
   }
   prepareInitialRequest(creator->getLastRequest());
   DialogSet* ds = new DialogSet(creator, *this);
   
   appDs->mDialogSetId = ds->getId();
   ds->mAppDialogSet = appDs;
   
   mDialogSetMap[ds->getId()] = ds;

   DebugLog (<< "RegistrationCreator: " << creator->getLastRequest());
   return creator->getLastRequest();
}

void 
DialogUsageManager::makeResponse(SipMessage& response, 
                                 const SipMessage& request, 
                                 int responseCode, 
                                 const Data& reason) const
{
   assert(request.isRequest());
   assert(response.isResponse());
   Helper::makeResponse(response, request, responseCode, reason);
}

void
DialogUsageManager::sendResponse(const SipMessage& response)
{
   assert(response.isResponse());
   mStack.send(response);
}
   

SipMessage&
DialogUsageManager::makeInviteSession(const Uri& target, const SdpContents* initialOffer, AppDialogSet* appDs)
{
   SipMessage& inv = makeNewSession(new InviteSessionCreator(*this, target, initialOffer), appDs);
   inv.header(h_RequestLine).uri() = target;
   return inv;   
}

SipMessage&
DialogUsageManager::makeSubscription(const Uri& aor, const NameAddr& target, const Data& eventType, AppDialogSet* appDs)
{
   return makeNewSession(new SubscriptionCreator(*this, target, eventType), appDs);
}

SipMessage& 
DialogUsageManager::makeRegistration(const NameAddr& aor, AppDialogSet* appDs)
{
   return makeNewSession(new RegistrationCreator(*this, aor), appDs); 
}

SipMessage& 
DialogUsageManager::makePublication(const Uri& targetDocument,  
                                    const Contents& body, 
                                    const Data& eventType, 
                                    unsigned expiresSeconds, 
                                    AppDialogSet* appDs)
{ 
   return makeNewSession(new PublicationCreator(*this, targetDocument, body, eventType, expiresSeconds), appDs); 
}


void
DialogUsageManager::send(const SipMessage& request)
{
   InfoLog (<< "SEND: " << request);
   mStack.send(request);
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

void
DialogUsageManager::process(FdSet& fdset)
{
   mStack.process(fdset);
   std::auto_ptr<Message> msg( mStack.receiveAny() );
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
   if (!msg.get())  return;
   if (sipMsg)
   {
      if (sipMsg->isRequest())
      {
         if( !validateRequest(*sipMsg) )
         {
            InfoLog (<< "Failed request validation " << *sipMsg);
            return;
         }
         if ( !validateTo(*sipMsg) )
         {
            InfoLog (<< "Failed to validation " << *sipMsg);
            return;
         }
         if (mergeRequest(*sipMsg) )
         {
            InfoLog (<< "Merged request: " << *sipMsg);
            return;
         }
         if ( mServerAuthManager )
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

   ErrLog(<<"Unknown message received." << msg->brief());
   assert(0);
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
               InfoLog (<< "Received a CANCEL on a non-existent transaction ");
               SipMessage failure;
               makeResponse(failure, request, 481);
               sendResponse(failure);
            }
            else
            {
               ds->cancel();
            }
            break;
         }

         case INVITE:  // new INVITE
         case SUBSCRIBE:
         case REFER: // out-of-dialog REFER
         case REGISTER:
         case PUBLISH:
         case MESSAGE :
         case OPTIONS :
         case INFO :   // handle non-dialog (illegal) INFOs
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
         {
            DialogSetId id(request);
            assert(mDialogSetMap.find(id) == mDialogSetMap.end());
            try
            {
               DialogSet* dset =  new DialogSet(request, *this);

               AppDialogSet* appDs = mAppDialogSetFactory->createAppDialogSet(*this, request);
               appDs->mDialogSetId = dset->getId();
               dset->mAppDialogSet = appDs;

               mDialogSetMap[id] = dset;
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
   DialogSet* ds = findDialogSet(DialogSetId(response));

   InfoLog( << "Dum processing response for dailog set " 
            << DialogSetId(response) 
            << " with map " 
            << Inserter(mDialogSetMap));

   if (ds)
   {
      ds->dispatch(response);
   }
   else
   {
      InfoLog (<< "Throwing away stray response: " << response.brief());
   }
}
 
DialogSet*
DialogUsageManager::findDialogSet(const DialogSetId& id)
{
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

void DialogUsageManager::removeDialogSet(const DialogSetId& dsId)
{
   mDialogSetMap.erase(dsId);

   if (mDialogSetMap.empty() && mDumShutdownHandler)
   {
      delete this;      
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
