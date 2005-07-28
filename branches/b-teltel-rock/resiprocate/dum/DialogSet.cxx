
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

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

DialogSet::DialogSet(BaseCreator* creator, DialogUsageManager& dum) :
   mMergeKey(),
   mDialogs(),
   mCreator(creator),
   mId(creator->getLastRequest()),
   mDum(dum),
   mAppDialogSet(0),
   mCancelled(false),
   mReceivedProvisional(false),
   mDestroying(false),
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

DialogSet::DialogSet(const SipMessage& request, DialogUsageManager& dum) : 
   mMergeKey(request),
   mDialogs(),
   mCreator(0),
   mId(request),
   mDum(dum),
   mAppDialogSet(0),
   mCancelled(false),
   mReceivedProvisional(false),
   mDestroying(false),
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
   DebugLog ( << " ************* Created DialogSet(UAS)  -- " << mId << "*************" );
}

DialogSet::~DialogSet()
{
   mDestroying = true;

   if (mDum.mClientAuthManager.get())
   {
      mDum.mClientAuthManager->dialogSetDestroyed(getId());
   }
   
   if (mMergeKey != MergedRequestKey::Empty)
   {
      mDum.mMergedRequests.erase(mMergeKey);
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
   mAppDialogSet->destroy();
}

void DialogSet::possiblyDie()
{
   if (!mDestroying)
   {
      if(mDialogs.empty() && 
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
}

DialogSetId
DialogSet::getId()
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

Dialog* 
DialogSet::findDialog(const SipMessage& msg)
{
   DialogId id(msg);
   Dialog* dlog = findDialog(id);
   //vonage/2543 matching here
   if (dlog)
   {
      return dlog;
   }
   else if (msg.exists(h_Contacts) && 
            msg.header(h_Contacts).size() == 1 
            && msg.isResponse() 
            && getUserProfile()->getLooseToTagMatching()
            && msg.header(h_To).exists(p_tag))     
   {
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
}

bool
DialogSet::empty() const
{
   return mDialogs.empty();
}

void
DialogSet::dispatch(const SipMessage& msg)
{
   assert(msg.isRequest() || msg.isResponse());

   if (msg.isResponse() && !mCancelled)
   {
      //!dcm! -- multiple usage grief...only one of each method type allowed
      if (getCreator() &&
          msg.header(h_CSeq) == getCreator()->getLastRequest().header(h_CSeq))
      {
         if (mDum.mClientAuthManager.get())
         {
            if (mDum.mClientAuthManager->handle( *getUserProfile(), getCreator()->getLastRequest(), msg))
            {
               DebugLog( << "about to re-send request with digest credentials" );
               StackLog( << getCreator()->getLastRequest() );
               
               mDum.send(getCreator()->getLastRequest());
               return;                     
            }
         }
         //!dcm! -- need to protect against 3xx highjacking a dialogset which
         //has a fully established dialog. also could case strange behaviour
         //by sending 401/407 at the wrong time.
         if (mDum.mRedirectManager.get())
         {
            if (mDum.mRedirectManager->handle(*this, getCreator()->getLastRequest(), msg))
            {
               //terminating existing dialogs(branches) as this is a final
               //response--?dcm?--merge w/ forking logic somehow?                              
               //!dcm! -- really, really horrible.  Should make a don't die
               //scoped guard
               mDestroying = true;               
               for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); )
               {
                  Dialog* d = it->second;
                  it++;
                  d->redirected(msg);         
               }
               mDestroying = false;
               
               if (!mDialogs.empty())
               {
                  //a dialog is refusing this 3xx(only implemented for INVITE,
                  //Subscribe dialogs always refuse as they don't have an early state)
                  return; //(toss 3xx)                  
               }

               InfoLog( << "about to re-send request to redirect destination" );
               DebugLog( << getCreator()->getLastRequest() );
               
               mDum.send(getCreator()->getLastRequest());
               return;                     
            }
         }
      }
   }

   Dialog* dialog = findDialog(msg);
   if (msg.isRequest())
   {
      const SipMessage& request = msg;
      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case BYE:
         case ACK:
         case CANCEL:  //cancel needs work
         case SUBSCRIBE:
         case REFER: //need to add out-of-dialog refer logic
            break; //dialog creating/handled by dialog
         case NOTIFY:
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
            assert(false);
            return; 
         case INFO:   
            if (dialog)
            {
               break;
            }
            else
            {
               return;
            }            
         case REGISTER:
            if (mServerRegistration == 0)
            {
               mServerRegistration = makeServerRegistration(request);
            }
            mServerRegistration->dispatch(request);
            return;
         case MESSAGE:
            mServerPagerMessage = makeServerPagerMessage(request);
            mServerPagerMessage->dispatch(request);
            return; 
         case UPDATE:
            //not implemented
            return;            
         default: 
            DebugLog ( << "In DialogSet::dispatch, default(ServerOutOfDialogRequest), msg: " << msg );            
            // only can be one ServerOutOfDialogReq at a time
            assert(mServerOutOfDialogRequest == 0);
            mServerOutOfDialogRequest = makeServerOutOfDialog(request);
            mServerOutOfDialogRequest->dispatch(request);
   			return;
            break;
      }
   }
   else
   {
      const SipMessage& response = msg;
      if (response.header(h_StatusLine).statusCode() < 200)
      {
         mReceivedProvisional = true;
         if (response.header(h_StatusLine).statusCode() == 100)
         {
            if (mDum.mDialogSetHandler)
            {
               mDum.mDialogSetHandler->onTrying(mAppDialogSet->getHandle(), msg);
            }
            return;
         }
      }      
         
      switch (response.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSCRIBE:
         case BYE:
         case ACK:
            break; //dialog creating/handled by dialog(2543 & illegal 3261 responses)
//             if(response.header(h_To).exists(p_tag))
//             {
//                break;  //dialog creating/handled by dialog
//             }
//             else
//             {
//                //throw away, informational status message eventually
//                return;               
//             }
         case CANCEL:
         case REFER:  //need to add out-of-dialog refer logic
            break; //dialog creating/handled by dialog
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
            if (dialog)
            {
               break;
            }
            else
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

   //!dcm! -- even if this matches, if a final reponses matches the inital request all
   //usages should be cancelled?
   if (dialog == 0)
   {
      if (msg.isRequest() && msg.header(h_RequestLine).method() == CANCEL)
      {
         for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); )
         {
            Dialog* d = it->second;
            it++;
            d->dispatch(msg);
         }
         return;         
      }

      if (msg.isResponse())
      {
         int code = msg.header(h_StatusLine).statusCode();
         
         if (code > 100 && code < 200 && !msg.exists(h_Contacts))
         {
            InfoLog ( << "Cannot create a dialog, no Contact in 180." );
            if (mDum.mDialogSetHandler)
            {
               mDum.mDialogSetHandler->onNonDialogCreatingProvisional(mAppDialogSet->getHandle(), msg);
            }

            //call OnProgress in proposed DialogSetHandler here
            return;         
         }
         else
         {
            InfoLog (<< "No matching dialog: " << msg.brief());
         }
      }
      
      DebugLog ( << "Creating a new Dialog from msg: " << msg);

      try
      {
         // !jf! This could throw due to bad header in msg, should we catch and rethrow
         // !jf! if this threw, should we check to delete the DialogSet? 
         // !dcm! -- check to delete for now, but we will need to keep the
         // Dialoset around in case something forked.
         dialog = new Dialog(mDum, msg, *this);
      }
      catch(BaseException& e)
      {
         InfoLog( << "Unable to create dialog: " << e.getMessage());
         //don't delete on provisional responses, as FWD will eventually send a
         //valid 200
         if(mDialogs.empty() && !(msg.isResponse() && msg.header(h_StatusLine).statusCode() >= 200))
         {
            mDum.destroy(this);
            return;            
         }
      }

      if (mCancelled && !(msg.isResponse() && msg.header(h_StatusLine).statusCode() >= 300))
      {
         dialog->cancel();
         return;         
      }
      else
      {
         DebugLog ( << "### Calling CreateAppDialog ### " << msg);
         AppDialog* appDialog = mAppDialogSet->createAppDialog(msg);
         dialog->mAppDialog = appDialog;
         appDialog->mDialog = dialog;         
      }
   }     
   if (dialog)
   {     
      dialog->dispatch(msg);
   }
   else if (msg.isRequest())
   {
      SipMessage response;
      mDum.makeResponse(response, msg, 481);
      mDum.send(response);
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
DialogSet::cancel()
{   
   mCancelled = true;
   if (mReceivedProvisional && getCreator())
   {
      // !jf! What is this comment about?
      //unify makeCancel w/ Dialog makeCancel, verify both
      //exception to cancel UAS DialogSet?

      InfoLog (<< "Canceling " << mId);
      auto_ptr<SipMessage> cancel(Helper::makeCancel(getCreator()->getLastRequest()));         
      mDum.send(*cancel);

      for (DialogMap::iterator it = mDialogs.begin(); it != mDialogs.end(); ++it)
      {
         // let the early dialogs know they are being canceled in case they get
         // a 200 to the INVITE which crossed the CANCEL so they will BYE them. 
         it->second->cancel();
      }

      // so it won't call me again
      mCancelled = false;
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

#if 0
ClientOutOfDialogReqHandle 
DialogSet::findClientOutOfDialog()
{
   if (mClientOutOfDialogRequests)
   {
      return mClientOutOfDialogReq->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such client out of dialog",
                                 __FILE__, __LINE__);
   }
}
#endif

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
