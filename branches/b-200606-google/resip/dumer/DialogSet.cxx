
#include "resip/stack/Helper.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/BaseCreator.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientOutOfDialogReq.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/DialogSetHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RedirectManager.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/ServerRegistration.hxx"
#include "resip/dum/DumHelper.hxx"
#include "resip/dum/SubscriptionCreator.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

// UAC 
DialogSet::DialogSet() :
   mDialogs()
{
}

// UAS 
DialogSet::DialogSet(const SipMessage& request) :
   mId(request),
{
   assert(request.isRequest());
   assert(request.isExternal());
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
// .dcm. how should ClientAuth clean up...esp if its a feature?
}

void DialogSet::possiblyDie()
{
   if(!isDestroying()
      && mDialogs.empty()
      && mChildUsages.empty())
   {
      setIsDestroying();
      mPrdManager.unManage(*this); //implies that unmanage is posts a commands to self
   }
}

DialogSetId
DialogSet::getId() const
{
   return mId;
}

void
DialogSet::addDialog(SharedPtr<Dialog> dialog)
{
   mDialogs.push_back(dialog);
}

Dialog*
DialogSet::findDialog(const SipMessage& msg)
{
   if (msg.isResponse() && msg.header(h_StatusLine).statusCode() == 100)
   {
      return 0;
   }
   return findDialog(DialogId(msg));
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

   // child usage want the message?
   for (std::list< SharedPtr<DialogUsage> >::const_iterator i = mChildUsages.begin();
        i != mChildUsages.end(); ++i) {
      if (i->isForMe(msg)) {
         DebugLog(<< "found child to handle message " << *i << endl << msg);
         i->dispatchChild(msg);
         return;
      }
   }

   // dialog wants the request?
   for (std::list< WeakdPtr<Dialog> >::const_iterator i = mDialogs.begin();
        i != mDialogss.end(); ++i) {
      SharedPtr<Dialog> dialog(*i);
      if (dialog.get()
          && dialog->!isDestroying()
          && dialog->isForMe(msg)) {
         if (msg->isRequest()) {
            DebugLog(<< "dialog message " << *dialog << endl << msg);
            std::auto_ptr<DialogUsage> child(mPrdManager.createDialogUsage(msg));
            if (child.get()) 
            {
               child->setDialog(dialog);
               addChild(SharedPtr<DialogUsage>(child.release()));
               child->dispatchChild(msg);
            }
            else 
            {
               DebugLog(<< "did not create usage");
            }
         else 
         {
            DebugLog(<< "discard stray response " << *dialog << endl << msg);
         }
         return;
         }
      }
   }

   // no dialog
   StackLog(<< "No matching dialog for " << endl << msg);
   if (!dispatchProgenitor(msg)) 
   {
      StackLog (<< "No matching dialog, sending 481 " << endl << msg);
      SharedPtr<SipMessage> response(new SipMessage);
      mPrdManager.makeResponse(*response, msg, 481);
      mPrdManager.send(response);
      return;
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
      if (i->second->isDestroying())
      {
         return 0;
      }
      else
      {
         return i->second;
      }
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
         assert (mCreator->getLastRequest()->header(h_CSeq).method() == INVITE);
         mState = Terminating;
         // !jf! this should be made exception safe
         SharedPtr<SipMessage> cancel(Helper::makeCancel(*getCreator()->getLastRequest()));
         mDum.send(cancel);

         if (mDialogs.empty())
         {
            // !jf! if 200/INV crosses a CANCEL that was sent after receiving
            // non-dialog creating provisional (e.g. 100), then we need to:
            // Add a new state, if we receive a 200/INV in this state, ACK and
            // then send a BYE and destroy the dialogset. 
            mState = Destroying;
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
      case Destroying:
         DebugLog (<< "DialogSet::end() called on a DialogSet that is already Terminating");
         //assert(0);
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
   return new ClientOutOfDialogReq(mDum, *this, *creator->getLastRequest());
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

void
DialogSet::dispatchProgenitor(const SipMessage& message)
{
   // if a request either create child or send response
   if (shouldCreateChild(message))
   {
      SharedPtr<Dialog> newDialog(new Dialog(msg));
      addDialog(newDialog);
      SharedPtr<DialogUsage> child(clone(newDialog));
      addChild(child);
      child->dispatch(msg);
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
