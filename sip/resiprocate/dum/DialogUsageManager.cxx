#include "resiprocate/SipStack.hxx"
#include "DialogUsageManager.hxx"

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
DialogUsageManager::setHandler(InvSessionHandler* handler)
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

SipMessage* 
DialogUsageManager::makeInviteSession(const Uri& target)
{
   SipMessage* invite = Helper::makeInvite(target, mProfile->getDefaultAor());
   prepareInitialRequest(*invite);
   return invite;
}

// !jf! add rest of make??? methods here


SipMessage* 
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
   if (!mProfile.isSchemeSupported(request.header(h_RequestLine).uri().scheme()))
   {
      InfoLog (<< "Received an unsupported scheme: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 416));
      mStack.send(*failure);
      return false;
   }
   else if (!mProfile.isMethodSupported(request.header(h_RequestLine).getMethod()))
   {
      InfoLog (<< "Received an unsupported method: " << request.brief());
      std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 405));
      failure->header(h_Allows) = mProfile.getAllowedMethods();
      mStack.send(*failure);
      return false;
   }
   else
   {
      Tokens unsupported = mProfile.isSupported(msg.header(h_Requires));
      if (!unsupported.empty())
      {
         InfoLog (<< "Received an unsupported option tag(s): " << request.brief());
         std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 420));
         failure->header(h_Unsupporteds) = unsupported;
         mStack.send(*failure);
         return false;
      }
      else if (msg.exists(h_ContentDisposition))
      {
         if (msg.header(h_ContentDisposition).exists(p_handling) && 
             isEqualNoCase(msg.header(h_ContentDisposition).param(p_handling), Symbols::Required))
         {
            if (!mProfile.isMimeTypeSupported(msg.header(h_ContentType)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_Accepts) = mProfile.getSupportedMimeTypes();
               mStack.send(*failure);

               return false;
            }
            else if (!mProfile.isContentEncodingSupported(msg.header(h_ContentEncoding)))
            {
               InfoLog (<< "Received an unsupported mime type: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_AcceptEncoding) = mProfile.getSupportedEncodings();
               mStack.send(*failure);
            }
            else if (!mProfile.isLanguageSupported(msg.header(h_ContentLanguages)))
            {
               InfoLog (<< "Received an unsupported language: " << request.brief());
               std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 415));
               failure->header(h_AcceptLanguages) = mProfile.getSupportedLanguages();
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
   // merge requrests here
   if ( !msg->header(h_To).param(p_tag).exists() )
   {
      DialogSet& dialogs = findDialogSet(DialogSetId(*msg));
      for (DialogSet::const_iterator i=dialogs.begin(); i!=dialogs.end(); i++)
      {
         if (i->shouldMerge(request))
         {
            InfoLog (<< "Merging request for: " << request.brief());
            
            std::auto_ptr<SipMessage> failure(Helper::makeResponse(msg, 482));
            mStack.send(*failure);
            
            return true;
         }
      }
   }
   return false;
}

void
DialogUsageManager::processRequest(const SipMessage& request)
{
   if ( !request.header(h_To).param(p_tag).exists() )
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
            assert(mDialogSetMap.count(id) == 0);
            mDialogSetMap[id] = DialogSet(request);
            Dialog* dialog = new Dialog(*this, request);
            mDialogSetMap[id].addDialog(dialog);
            dialog->dispatch(request);
            break;

/*
         case INVITE:  // new INVITE
         {
            ServerInvSession* session = new ServerInvSession(*this, request);
            break;
         }
         case SUBSCRIBE:
         case REFER: // out-of-dialog REFER
         {
            ServerSubscription* sub = new ServerSubscription(*this, request);
            break;
         }
         case REGISTER:
         {
            ServerRegistration* reg = new ServerRegistration(*this, request);
            break;
         }
         case PUBLISH:
         {
            ServerPublication* pub = new ServerPublication(*this, request);
            break;                       
         }
         case MESSAGE :
         case OPTIONS :
         case INFO :   // handle non-dialog (illegal) INFOs
         case NOTIFY : // handle unsolicited (illegal) NOTIFYs
         {
            // make a non-dialog BaseUsage
            ServerOutOfDialogReq* req = new ServerOutOfDialogReq(*this, request);
            break;
         }
         default:
            assert(0);
            break;
*/

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
   if (dialogs != DialogSet::Empty)
   {
      dialogs.dispatch(response);
   }
   else
   {
      DebugLog (<< "Throwing away stray response: " << response);
   }
}


Dialog&  
DialogUsageManager::findDialog(DialogId id)
{

    HashMap<DialogSetId, DialogSet>::const_iterator it;

    DialogSetId setId = id.getDialogSetId();
    if ((it = mDialogSetMap.find(setId)) == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) */
    }
    DialogSet dialogSet = it->second();
    Dialog dialog;
    if ((dialog = dialogSet.find(id)) == NULL)
    {
        /**  @todo: return empty object (?) */
    }
    return *dialog;
    
}
 
DialogSet&
DialogUsageManager::findDialogSet( DialogSetId id )
{
    HashMap<DialogSetId, DialogSet>::const_iterator it;

    if ((it = mDialogSetMap.find(id)) == mDialogSetMap.end())
    {
        /**  @todo: return empty object (?) **/
    }
    return it->first();
}

BaseCreator&
DialogUsageManager::findCreator(DialogId id)
{
    DialogSetId setId = id.getDialogSetId();
    DialogSet dialogSet = findDialogSet(setId);
    BaseCreator creator = dialogset.getCreator();
    if (creator == NULL)
    {
        /** @todo; return empty object (?) */
    }
    return (*creator);
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

ServerInviteSession::Handle 
DialogUsageManager::createServerInviteSession()
{
   BaseUsage* usage = new ServerInviteSession();
   
   assert(mUsage.find(usage->mHandle.mId) == mUsage.end());
   mUsage[usage->mHandle.mId] = usage;

   return usage->mHandle;
}

void
DialogUsageManager::destroyUsage(BaseUsage* usage)
{
   UsageHandleMap::iterator i = mUsageMap.find(usage->mHandle.mId);
   if (i =! mUsageMap.end())
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
