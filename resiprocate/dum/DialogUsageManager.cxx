#include "resiprocate/SipStack.hxx"
#include "DialogUsageManager.hxx"
#include "resiprocate/Helper.hxx"
#include "Profile.hxx"
#include "resiprocate/SipMessage.hxx"
#include "InviteSessionCreator.hxx"
#include "SubscriptionCreator.hxx"
#include "PublicationCreator.hxx"
#include "ClientAuthManager.hxx"
#include "ServerAuthManager.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DialogUsageManager::DialogUsageManager(SipStack& stack) 
   : mStack(stack)
{
   
}

void DialogUsageManager::setProfile(Profile* profile)
{
   mProfile = profile;
}


void DialogUsageManager::setManager(RedirectManager* manager)
{
   mRedirectManager = manager;
}

void 
DialogUsageManager::setManager(ClientAuthManager* manager)
{
   mClientAuthManager = manager;
}

void 
DialogUsageManager::setManager(ServerAuthManager* manager)
{
   mServerAuthManager = manager;
}

void 
DialogUsageManager::setClientRegistrationHandler(ClientRegistrationHandler* handler)
{
   mClientRegistrationHandler = handler;
}

void 
DialogUsageManager::setServerRegistrationHandler(ServerRegistrationHandler* handler)
{
   mServerRegistrationHandler = handler;
}

void 
DialogUsageManager::setInviteSessionHandler(InviteSessionHandler* handler)
{
   mInviteSessionHandler = handler;
}

void 
DialogUsageManager::addHandler(const Data& eventType, ClientSubscriptionHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ServerSubscriptionHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ClientPublicationHandler*)
{
}

void 
DialogUsageManager::addHandler(const Data& eventType, ServerPublicationHandler*)
{
}

void 
DialogUsageManager::addHandler(MethodTypes&, OutOfDialogHandler*)
{
}

SipMessage& 
DialogUsageManager::makeNewSession(BaseCreator* creator)
{
   prepareInitialRequest(creator->getLastRequest());
   DialogSet* inv = new DialogSet(creator);
   mDialogSetMap[inv->getId()] = inv;
   return creator->getLastRequest();
}

SipMessage&
DialogUsageManager::makeInviteSession(const Uri& target, const SdpContents* initialOffer)
{
   return makeNewSession(new InviteSessionCreator(*this, target, initialOffer));
}

SipMessage&
DialogUsageManager::makeSubscription(const Uri& aor, const Data& eventType)
{
   return makeNewSession(new SubscriptionCreator(*this, eventType));
}

#if 0
SipMessage&
DialogUsageManager::makeRefer(const Uri& aor, const H_ReferTo::Type& referTo)
{
   //return makeNewSession(new SubscriptionCreator(???));
}

SipMessage& 
DialogUsageManager::makePublication(const Uri& aor, const Data& eventType)
{
   return makeNewSession(new PublicationCreator()); // !jf!
}

SipMessage& 
DialogUsageManager::makeRegistration(const Uri& aor)
{
   //return makeNewSession(new RegistrationCreator(???)); // !jf!
}

SipMessage& 
DialogUsageManager::makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth)
{
   //return makeNewSession(new OutOfDialogReqCreator(???)); // !jf!
}

SipMessage&
DialogUsageManager::makeInviteSession(DialogId id, const Uri& target)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makeInviteSession(target);
}

SipMessage* 
DialogUsageManager::makeSubscription(const Uri& aor, const Data& eventType)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makeSubscription(aor, eventType);
}

SipMessage* 
DialogUsageManager::makeRefer(const Uri& aor, const H_ReferTo::Type& referTo)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makeRefer(aor, referTo);
}

SipMessage* 
DialogUsageManager::makePublication(const Uri& aor, const Data& eventType)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makePublication(aor, eventType);
}

SipMessage* 
DialogUsageManager::makeRegistration(const Uri& aor)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makeRegistration(aor);
}

SipMessage* 
DialogUsageManager::makeOutOfDialogRequest(const Uri& aor, const MethodTypes& meth)
{
   Dialog& dialog = findDialog(id); // could throw
   return dialog.makeOutOfDialogRequest(aor, meth);
}
#endif

void
DialogUsageManager::prepareInitialRequest(SipMessage& request)
{
   // !jf! 
   request.header(h_Supporteds) = mProfile->getSupportedOptionTags();
   request.header(h_Allows) = mProfile->getAllowedMethods();
}

void
DialogUsageManager::process(FdSet& fdset)
{
   mStack.process(fdset);
   SipMessage* msg = mStack.receive();
   if (msg)
   {
      if (msg->isRequest())
      {
         if (validateRequest(*msg) && 
             validateTo(*msg) && 
             !mergeRequest(*msg) &&
             mServerAuthManager && mServerAuthManager->handle(*msg))
         {
            processRequest(*msg);
         }
      }
      else if (msg->isResponse())
      {
         if (mClientAuthManager && mClientAuthManager->handle(*msg))
         {
            processResponse(*msg);
         }
      }
   }

   delete msg;
}

bool
DialogUsageManager::validateRequest(const SipMessage& request)
{
   if (!mProfile->isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 416));
      mStack.send(*failure);
      return false;
   }
   else if (!mProfile->isMethodSupported(request.header(h_RequestLine).getMethod()))
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 405));
      failure->header(h_Allows) = mProfile->getAllowedMethods();
      mStack.send(*failure);
      return false;
   }
   else
   {
      Tokens unsupported = mProfile->isSupported(request.header(h_Requires));
      if (!unsupported.empty())
      {
         InfoLog (<< "Received an unsupported option tag(s): " << request.brief());
         std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 420));
         failure->header(h_Unsupporteds) = unsupported;
         mStack.send(*failure);
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
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 415));
               failure->header(h_Accepts) = mProfile->getSupportedMimeTypes();
               mStack.send(*failure);

               return false;
            }
            else if (!mProfile->isContentEncodingSupported(request.header(h_ContentEncoding)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 415));
               failure->header(h_AcceptEncodings) = mProfile->getSupportedEncodings();
               mStack.send(*failure);
            }
            else if (!mProfile->isLanguageSupported(request.header(h_ContentLanguages)))
            {
               InfoLog (<< "Received an unsupported language: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 415));
               failure->header(h_AcceptLanguages) = mProfile->getSupportedLanguages();
               mStack.send(*failure);
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
   return true;
}

bool
DialogUsageManager::mergeRequest(const SipMessage& request)
{
   if (!request.header(h_To).exists(p_tag))
   {
      DialogSet& dialogs = findDialogSet(DialogSetId(request));
      if (dialogs.mergeRequest(request))
      {
         std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 482));
         mStack.send(*failure);
         return true;
      }
   }
   return false;
}

void
DialogUsageManager::processRequest(const SipMessage& request)
{
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
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 481));
            mStack.send(*failure);
            break;
         }
         case CANCEL:
         {
            // find the appropropriate ServerInvSession
            DialogSet& dialogs = findDialogSet(DialogSetId(request));
            dialogs.cancel(request);
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
            mDialogSetMap[id] = new DialogSet(request);
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
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 400, "rjs says, Go to hell"));
            mStack.send(*failure);
            break;
         }
         
         default:
         {
            DialogSet& dialogs = findDialogSet(DialogSetId(request));
            dialogs.dispatch(request);
         }
      }
   }
}

void
DialogUsageManager::processResponse(const SipMessage& response)
{
   DialogSet& dialogs = findDialogSet(DialogSetId(response));
   if (!dialogs.empty())
   {
      dialogs.dispatch(response);
   }
   else
   {
      DebugLog (<< "Throwing away stray response: " << response);
   }
}


Dialog&  
DialogUsageManager::findDialog(const DialogId& id)
{

    DialogSetId setId = id.getDialogSetId();
    DialogSetMap::const_iterator it = mDialogSetMap.find(setId);
    
    if (it == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) */
    }
    DialogSet* dialogSet = it->second;
    Dialog* dialog = dialogSet->findDialog(id);
    if (!dialog)
    {
        /**  @todo: return empty object (?) */
    }
    return *dialog;
}
 
DialogSet&
DialogUsageManager::findDialogSet(const DialogSetId& id)
{
   DialogSetMap::const_iterator it = mDialogSetMap.find(id);

    if (it == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) **/
    }
    return *it->second;
}

BaseCreator&
DialogUsageManager::findCreator(const DialogId& id)
{
   const DialogSetId& setId = id.getDialogSetId();
   const DialogSet& dialogSet = findDialogSet(setId);
   BaseCreator* creator = dialogSet.getCreator();
   if (creator == NULL)
   {
      /** @todo; return empty object (?) */
   }
   return *creator;
}

BaseUsage* 
DialogUsageManager::getUsage(const BaseUsage::Handle& handle)
{
   UsageHandleMap::iterator i = mUsageMap.find(handle.mId);
   if (i == mUsageMap.end())
   {
      throw Exception("Stale BaseUsage handle",
                      __FILE__, __LINE__);
   }
   else
   {
      return i->second;
   }
}

ServerInviteSession*
DialogUsageManager::makeServerInviteSession(Dialog& dialog,
                                            const SipMessage& request)
{
   ServerInviteSession* usage = new ServerInviteSession(*this, dialog, request);
   
   assert(mUsageMap.find(usage->getBaseHandle().mId) == mUsageMap.end());
   mUsageMap[usage->getBaseHandle().mId] = usage;

   return usage;
}

void
DialogUsageManager::destroyUsage(BaseUsage* usage)
{
   UsageHandleMap::iterator i = mUsageMap.find(usage->getBaseHandle().mId);
   if (i != mUsageMap.end())
   {
      delete i->second;
      mUsageMap.erase(i);
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
