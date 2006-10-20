
#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/BaseCreator.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"
#include "resiprocate/dum/ServerPagerMessage.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/DialogSetHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/RedirectManager.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

// UAC 
DialogSet::DialogSet(BaseCreator* creator, DialogUsageManager& dum) :
   mMergeKey(),
   mDialogs(),
   mCreator(creator),
   mId(creator->getLastRequest()),
   mDum(dum),
   mAppDialogSet(0),
   mAppDialogSetNeedsRelease(false),
   mState(Initial),
   mClientRegistration(0),
   mServerRegistration(0),
   mClientPublication(0),
   mClientOutOfDialogRequests(),
   mServerOutOfDialogRequest(0),
   mClientPagerMessage(0),
   mServerPagerMessage(0),
   mUserProfile(0)
{
   setUserProfile(&creator->getUserProfile());
   assert(!creator->getLastRequest().isExternal());
   DebugLog ( << " ************* Created DialogSet(UAC)  -- " << mId << "*************" );
}

// UAS 
DialogSet::DialogSet(const SipMessage& request, DialogUsageManager& dum) :
   mMergeKey(request),
   mDialogs(),
   mCreator(0),
   mId(request),
   mDum(dum),
   mAppDialogSet(0),
   mAppDialogSetNeedsRelease(false),
   mState(Established),
   mClientRegistration(0),
   mServerRegistration(0),
   mClientPublication(0),
   mClientOutOfDialogRequests(),
   mServerOutOfDialogRequest(0),
   mClientPagerMessage(0),
   mServerPagerMessage(0),
   mUserProfile(0)
{
   assert(request.isRequest());
   assert(request.isExternal());
   mDum.mMergedRequests.insert(mMergeKey);
   assert(mDum.mCancelMap.count(request.getTransactionId()) == 0);
   if (request.header(h_RequestLine).method() == INVITE)
   {
      mCancelKey = request.getTransactionId();
      mDum.mCancelMap[mCancelKey] = this;
   }
   DebugLog ( << " ************* Created DialogSet(UAS)  -- " << mId << "*************" );
}

DialogSet::~DialogSet()
{
   if (mDum.mClientAuthManager.get())
   {
      mDum.mClientAuthManager->dialogSetDestroyed(getId());
   }

   if (mMergeKey != MergedRequestKey::Empty)
   {
      mDum.mMergedRequests.erase(mMergeKey);
   }

   if (!mCancelKey.empty())
   {
      mDum.mCancelMap.erase(mCancelKey);
   }

   delete mCreator;
   while(!mDialogs.empty())
   {
      delete mDialogs.begin()->second;
   }

   delete mClientRegistration;
   delete mServerRegistration;
   delete mClientPublication;
   delete mServerOutOfDialogRequest;
   delete mClientPagerMessage;
   delete mServerPagerMessage;

   while (!mClientOutOfDialogRequests.empty())
   {
      delete *mClientOutOfDialogRequests.begin();
   }

   DebugLog ( << " ********** DialogSet::~DialogSet: " << mId << "*************" );
   //!dcm! -- very delicate code, change the order things go horribly wrong

   mDum.removeDialogSet(this->getId());
   if (mAppDialogSet && mAppDialogSetNeedsRelease) 
   {
      mAppDialogSet->destroy();
   }
}

void DialogSet::possiblyDie()
{
   if(mState != Initial &&  // !jf! may not be correct
      mDialogs.empty() &&
      mClientOutOfDialogRequests.empty() &&
      !(mClientPublication ||
        mServerOutOfDialogRequest ||
        mClientPagerMessage ||
        mServerPagerMessage ||
        mClientRegistration ||
        mServerRegistration))
   {
      mDum.destroy(this);
   }
}

DialogSetId
DialogSet::getId() const
{
   return mId;
}

void
DialogSet::addDialog(Dialog *dialog)
{
   mDialogs[dialog->getId()] = dialog;
}

BaseCreator*
DialogSet::getCreator()
{
   return mCreator;
}

Dialog*
DialogSet::findDialog(const SipMessage& msg)
{
   if (msg.isResponse() && msg.header(h_StatusLine).statusCode() == 100)
   {
      return 0;
   }
   return findDialog(DialogId(msg));
#if 0   
   DialogId id(msg);
   Dialog* dlog = findDialog(id);
   //vonage/2543 matching here
   if (dlog)
   {
      return dlog;
   }
   //match off transaction ID
   else if (msg.isResponse() && !msg.header(h_To).exists(p_tag))
   {
   for(DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         if (it->second->matches(msg))
         {
            return it->second;            
         }
      }
   }
   else if (msg.exists(h_Contacts) && !msg.header(h_Contacts).empty()
            && msg.isResponse() 
            && mDum.getProfile()->looseToTagMatching()
            && msg.header(h_To).exists(p_tag))     
   {
      const Uri& contact = msg.header(h_Contacts).front().uri();
      
      //match by contact
      for(DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         if (it->second->mRemoteTarget.uri() == msg.header(h_Contacts).front().uri())
         {
            //!dcm! in the vonage case, the to tag should be updated to match the fake
            //vonage tag introduced in the 200 which is also used for the BYE.
            //find out how deep this rabbit hole goes, may just have a pugabble
            //filter api that can be added for dialog matching if things get any
            //more specific--this is the VonageKludgeFilter
            Dialog* dialog = it->second;
            DialogId old = dialog->getId();
            dialog->mId = DialogId(old.getCallId(), old.getLocalTag(), msg.header(h_To).param(p_tag));
            dialog->mRemoteNameAddr.param(p_tag) = msg.header(h_To).param(p_tag);
            mDialogs.erase(it);
            mDialogs[dialog->getId()] = dialog;
            return dialog;
         }
      }
   }
   return 0;
#endif
}

bool
DialogSet::empty() const
{
   return mDialogs.empty();
}

bool
DialogSet::handledByAuthOrRedirect(const SipMessage& msg)
{
   if (msg.isResponse() && !(mState == Terminating || mState == WaitingToEnd))
   {
      //!dcm! -- multiple usage grief...only one of each method type allowed
      if (getCreator() &&
          msg.header(h_CSeq) == getCreator()->getLastRequest().header(h_CSeq))
      {
         if (mDum.mClientAuthManager.get())
         {
            if (mDum.mClientAuthManager->handle(*getUserProfile(), getCreator()->getLastRequest(), msg))
            {
               DebugLog( << "about to re-send request with digest credentials" );
               StackLog( << getCreator()->getLastRequest() );
               
               mDum.send(getCreator()->getLastRequest());
               return true;                     
            }
         }
         //!dcm! -- need to protect against 3xx highjacking a dialogset which
         //has a fully established dialog. also could case strange behaviour
         //by sending 401/407 at the wrong time.
         if (mDum.mRedirectManager.get() && mState != Established)  // !slg! for now don't handle redirect in established dialogs - alternatively we could treat as a target referesh (using 1st Contact) and reissue request
         {
            if (mDum.mRedirectManager->handle(*this, getCreator()->getLastRequest(), msg))
            {
               //terminating existing dialogs(branches) as this is a final
               //response--?dcm?--merge w/ forking logic somehow?                              
               //!dcm! -- really, really horrible.  Should make a don't die
               //scoped guard
               mState = Initial;               
               for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); ++it)
               {
                  it->second->redirected(msg);         
               }
               
			   /* !slg! this won't allow any redirections - since Dialog destruction is delayed by queueing a destroy event
               if (!mDialogs.empty())
               {
                  //a dialog is refusing this 3xx(only implemented for INVITE,
                  //Subscribe dialogs always refuse as they don't have an early state)
                  return true; //(toss 3xx) !dcm! -- might leak dialog
               }*/

               InfoLog( << "about to re-send request to redirect destination" );
               DebugLog( << getCreator()->getLastRequest() );
               
               mDum.send(getCreator()->getLastRequest());
               return true;                     
            }

            // Check if a 422 response to initial Invite (sessionTimer draft)
            if(msg.header(h_StatusLine).statusCode() == 422 && msg.exists(h_MinSE))
            {
               // Change interval to min from 422 response
               getCreator()->getLastRequest().header(h_SessionExpires).value() = msg.header(h_MinSE).value();

               InfoLog( << "about to re-send request with new session expiration time" );
               DebugLog( << getCreator()->getLastRequest() );
               
               mDum.send(getCreator()->getLastRequest());
               return true;                     
            }
         }
      }
   }
   return false;
}


void
DialogSet::dispatch(const SipMessage& msg)
{
   assert(msg.isRequest() || msg.isResponse());

   if (mState == WaitingToEnd)
   {
      assert(mDialogs.empty());
      if (msg.isResponse())         
      {
         int code = msg.header(h_StatusLine).statusCode();
         switch(mCreator->getLastRequest().header(h_CSeq).method())               
         {
            case INVITE:
               if (code / 100 == 1)
               {
                  mState = ReceivedProvisional;
                  end();
               }
               else if (code / 100 == 2)
               {
                  Dialog dialog(mDum, msg, *this);

                  SipMessage ack;
                  dialog.makeRequest(ack, ACK);
                  dialog.send(ack);
                  
                  SipMessage bye;
                  dialog.makeRequest(bye, BYE);
                  dialog.send(bye);
               }
               else
               {
                  mDum.destroy(this);
               }
               break;
            case SUBSCRIBE:
               assert(0);
               break;
            default:
               break;
         }
      }
      else
      {
         SipMessage response;         
         mDum.makeResponse(response, msg, 481);
         mDum.send(response);
      }
      return;
   }

   if (handledByAuthOrRedirect(msg))
   {
      return;
   }

   Dialog* dialog = findDialog(msg);
   assert(!dialog || msg.header(h_CSeq).method() != MESSAGE);

   if (dialog)
   {
      DebugLog (<< "Found matching dialog " << *dialog << " for " << endl << msg);
   }
   else
   {
      StackLog (<< "No matching dialog for " << endl << msg);
   }
   
   if (msg.isRequest())
   {
      const SipMessage& request = msg;
      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case CANCEL:  //cancel needs work
         case SUBSCRIBE:
         case REFER: //need to add out-of-dialog refer logic
            break; //dialog creating/handled by dialog

         case BYE:
         case INFO:
         case ACK:
         case UPDATE:
            assert(dialog);
            break;
            
         case NOTIFY:
            // !jf! This should really not use to tag since 2543 endpoints won't
            // have a to tag. 
            // !jf! there shouldn't be a dialogset for ServerOutOfDialogReq
            if (request.header(h_To).exists(p_tag))
            {
               break; //dialog creating/handled by dialog
            }
            else // no to tag - unsolicited notify
            {
               // unsolicited - not allowed but commonly implemented
               // by large companies with a bridge as their logo
               assert(mServerOutOfDialogRequest == 0);
               mServerOutOfDialogRequest = makeServerOutOfDialog(request);
               mServerOutOfDialogRequest->dispatch(request);
               return;
            }
            break;

         case PUBLISH:
            assert(false); // handled in DialogUsageManager
            return;
            
         case REGISTER:
            // !jf! move this to DialogUsageManager
            if (mServerRegistration == 0)
            {
               mServerRegistration = makeServerRegistration(request);
            }
            mServerRegistration->dispatch(request);
            return;

         case MESSAGE:
            // !jf! move this to DialogUsageManager
            mServerPagerMessage = makeServerPagerMessage(request);
            mServerPagerMessage->dispatch(request);
            return;

         default:
            // !jf! move this to DialogUsageManager
            DebugLog ( << "In DialogSet::dispatch, default(ServerOutOfDialogRequest), msg: " << msg );
            // only can be one ServerOutOfDialogReq at a time
            assert(mServerOutOfDialogRequest == 0);
            mServerOutOfDialogRequest = makeServerOutOfDialog(request);
            mServerOutOfDialogRequest->dispatch(request);
   			return;
      }
   }
   else // the message is a response
   {
      const SipMessage& response = msg;

      int code = msg.header(h_StatusLine).statusCode();
      switch(mState)
      {
         case Initial:
            if (code < 200)
            {
               mState = ReceivedProvisional;
            }
            else if(code < 300)
            {
               mState = Established;
            }
            else
            {
               if (mDialogs.empty())
               {
                  mState = Established;
               }
               else
               {
                  dispatchToAllDialogs(msg);
                  return;
               }
            }
            break;
         case ReceivedProvisional:
            if (code < 200)
            {
               // fall through
            }
            else if (code < 300)
            {
               mState = Established;
               for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
               {
                  if (it->second != dialog) // this is dialog that accepted
                  {
                     it->second->onForkAccepted();
                  }
               }
            }
            else // failure response
            {
               if (mDialogs.empty())
               {
                  mState = Established;
               }
               else
               {
                  dispatchToAllDialogs(msg);
                  return;
               }
            }
            break;
         default:
            // !jf!
            break;
            
      }

      if (response.header(h_StatusLine).statusCode() == 100)
      {
         if (mDum.mDialogSetHandler)
         {
            mDum.mDialogSetHandler->onTrying(mAppDialogSet->getHandle(), msg);
         }
         return;
      }
      
      switch (response.header(h_CSeq).method())
      {
         case INVITE:
         case BYE:
         case ACK:
         case CANCEL:
         case REFER:  
            break; 

         case SUBSCRIBE:
            if (dialog == 0 &&
                response.header(h_StatusLine).statusCode() < 300 &&
                (!response.exists(h_Expires) ||
                 response.header(h_Expires).value() == 0))
            {
               return;
            }
            break;

         case PUBLISH:
            if (mClientPublication == 0)
            {
               mClientPublication = makeClientPublication(response);
            }
            mClientPublication->dispatch(response);
            return;

         case REGISTER:
            if (mClientRegistration == 0)
            {
               mClientRegistration = makeClientRegistration(response);
            }
            mClientRegistration->dispatch(response);
            return;

         case MESSAGE:
            if (mClientPagerMessage)
            {
               mClientPagerMessage->dispatch(response);
            }
            return;            

         case INFO:   
         case UPDATE:
            if (dialog)
            {
               break;
            }
            else // not allowed
            {
               return;
            }
            
         case NOTIFY:
            if (dialog)
            {
               break;
            }
            
         default:
         {
            ClientOutOfDialogReq* req = findMatchingClientOutOfDialogReq(response);
            if (req == 0)
            {
               req = makeClientOutOfDialogReq(response);
               mClientOutOfDialogRequests.push_back(req);
            }
            req->dispatch(response);
            return;
         }
      }
   }

   if (dialog == 0)
   {
      if (msg.isRequest() && msg.header(h_RequestLine).method() == CANCEL)
      {
         dispatchToAllDialogs(msg);
         return;
      }

      if (msg.isResponse())
      {
         int code = msg.header(h_StatusLine).statusCode();
         
         if (!msg.exists(h_Contacts) && code > 100 && code < 200)
         {
            InfoLog ( << "Cannot create a dialog, no Contact in 180." );
            if (mDum.mDialogSetHandler)
            {
               mDum.mDialogSetHandler->onNonDialogCreatingProvisional(mAppDialogSet->getHandle(), msg);
            }
            return;         
         }
         // If failure response and no dialogs, create a dialog, otherwise
         else if (code >= 300 && !mDialogs.empty())
         {
            dispatchToAllDialogs(msg);
            return;
         }
      }

      DebugLog ( << "Creating a new Dialog from msg: " << msg);
      try
      {
         // !jf! This could throw due to bad header in msg, should we catch and rethrow
         dialog = new Dialog(mDum, msg, *this);
      }
      catch(BaseException& e)
      {
         InfoLog( << "Unable to create dialog: " << e.getMessage());
         if (msg.isResponse())
         {
            //don't delete on provisional responses, as FWD will eventually send a
            //valid 200
            if(mDialogs.empty() && msg.header(h_StatusLine).statusCode() >= 200)
            {
               // really we should wait around 32s before deleting this
               mDum.destroy(this);
            }
         }
         else
         {
            // !jf! derek thinks we should destroy only on invalid CANCEL or
            // BYE, hmmphh. see draft-sparks-sipping-dialogusage-01.txt
            SipMessage response;
            mDum.makeResponse(response, msg, 400);
            mDum.send(response);
            if(mDialogs.empty())
            {
               mDum.destroy(this);
            }
         }
         return;
      }

      assert(mState != WaitingToEnd);
      DebugLog ( << "### Calling CreateAppDialog ### " << msg);
      AppDialog* appDialog = mAppDialogSet->createAppDialog(msg);
      dialog->mAppDialog = appDialog;
      appDialog->mDialog = dialog;
      dialog->dispatch(msg);
   }
   else
   {     
      dialog->dispatch(msg);
   }
}


ClientOutOfDialogReq*
DialogSet::findMatchingClientOutOfDialogReq(const SipMessage& msg)
{
   for (std::list<ClientOutOfDialogReq*>::iterator i=mClientOutOfDialogRequests.begin();
        i != mClientOutOfDialogRequests.end(); ++i)
   {
      if ((*i)->matches(msg))
      {
         return *i;
      }
   }
   return 0;
}

Dialog*
DialogSet::findDialog(const DialogId id)
{
   DebugLog (<< "findDialog: " << id << " in " << Inserter(mDialogs));

   DialogMap::iterator i = mDialogs.find(id);
   if (i == mDialogs.end())
   {
      return 0;
   }
   else
   {
      return i->second;
   }
}

void
DialogSet::end()
{
   switch(mState)
   {
      case Initial:
         mState = WaitingToEnd;
         break;
      case WaitingToEnd:
         break;         
      case ReceivedProvisional:
      {
         assert (mCreator->getLastRequest().header(h_CSeq).method() == INVITE);
         mState = Terminating;
         // !jf! this should be made exception safe
         auto_ptr<SipMessage> cancel(Helper::makeCancel(getCreator()->getLastRequest()));         
         mDum.send(*cancel);

         if (mDialogs.empty())
         {
            // !jf! if 200/INV crosses a CANCEL that was sent after receiving
            // non-dialog creating provisional (e.g. 100), then we need to:
            // Add a new state, if we receive a 200/INV in this state, ACK and
            // then send a BYE and destroy the dialogset. 
            mDum.destroy(this);
         }
         else
         {
            //need to lag and do last element ouside of look as this DialogSet will be
            //deleted if all dialogs are destroyed
            for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
            {
               try
               {
                  it->second->cancel();
               }
               catch(UsageUseException& e)
               {
                  InfoLog (<< "Caught: " << e);
               }
            }
         }
      }            
      break;         
      case Established:
      {
         for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); ++it)
         {
            try
            {
               it->second->end();
            }
            catch(UsageUseException& e)
            {
               InfoLog (<< "Caught: " << e);
            }
         }            
         mState = Terminating;
         break;
      }
      case Terminating:
         assert(0);
   }
}


ClientRegistrationHandle
DialogSet::getClientRegistration()
{
   if (mClientRegistration)
   {
      return mClientRegistration->getHandle();
   }
   else
   {
      return ClientRegistrationHandle::NotValid();
   }
}

ServerRegistrationHandle
DialogSet::getServerRegistration()
{
   if (mServerRegistration)
   {
      return mServerRegistration->getHandle();
   }
   else
   {
      return ServerRegistrationHandle::NotValid();
   }
}

ClientPublicationHandle
DialogSet::getClientPublication()
{
   if (mClientPublication)
   {
      return mClientPublication->getHandle();
   }
   else
   {
      return ClientPublicationHandle::NotValid();
   }
}

ClientRegistration*
DialogSet::makeClientRegistration(const SipMessage& response)
{
   BaseCreator* creator = getCreator();
   assert(creator);
   return new ClientRegistration(mDum, *this, creator->getLastRequest());
}

ClientPublication*
DialogSet::makeClientPublication(const SipMessage& response)
{
   BaseCreator* creator = getCreator();
   assert(creator);
   return new ClientPublication(mDum, *this, creator->getLastRequest());
}

ClientOutOfDialogReq*
DialogSet::makeClientOutOfDialogReq(const SipMessage& response)
{
   BaseCreator* creator = getCreator();
   assert(creator);
   return new ClientOutOfDialogReq(mDum, *this, creator->getLastRequest());
}

ServerRegistration*
DialogSet::makeServerRegistration(const SipMessage& request)
{
   return new ServerRegistration(mDum, *this, request);
}

ServerOutOfDialogReq*
DialogSet::makeServerOutOfDialog(const SipMessage& request)
{
   return new ServerOutOfDialogReq(mDum, *this, request);
}

ServerPagerMessage*
DialogSet::makeServerPagerMessage(const SipMessage& request)
{
   return new ServerPagerMessage(mDum, *this, request);
}

ServerOutOfDialogReqHandle
DialogSet::getServerOutOfDialog()
{
   if (mServerOutOfDialogRequest)
   {
      return mServerOutOfDialogRequest->getHandle();
   }
   else
   {
      return ServerOutOfDialogReqHandle::NotValid();
   }
}

void DialogSet::dispatchToAllDialogs(const SipMessage& msg)
{
   if (!mDialogs.empty())
   {
      for(DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
      {
         it->second->dispatch(msg);         
      }
   }
}

UserProfile*
DialogSet::getUserProfile()
{
   if(mUserProfile)
   {
      return mUserProfile;
   }
   else
   {
      // If no UserProfile set then use UserProfile of the MasterProfile
      return mDum.getMasterProfile();
   }
}

void
DialogSet::setUserProfile(UserProfile *userProfile)
{
   assert(!mUserProfile);
   mUserProfile = userProfile;
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
1 *    permission, please contact vocal@vovida.org.
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
