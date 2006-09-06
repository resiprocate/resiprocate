#include "resiprocate/SipMessage.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/PagerMessageCreator.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"
#include "resiprocate/dum/PagerMessageHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/UsageUseException.hxx"
#include "resiprocate/dum/InternalSendPagerMessage.hxx"
#include "resiprocate/dum/InternalEndClientPagerMessage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/ExtensionHeader.hxx"

using namespace resip;

#if(0)
//  for appcliation behaves like ICQ
app::onSendButtonClicked(im)
{
   disableSendButton();
   mPageManager.page(im);
}

app::onSuccess(...)
{
   enableSendButton();
}

app::onFailure(...)
{
   reportFailueToUI();
   enableSendButton();
}

//  for application behaves like MSN
app::onSendButtonClicked(im)
{
   mPageManager.page(im);
}

app::onSuccess(...)
{
   // most likely no-op
}

app::onFailure(...)
{
   reportFailueToUI();
}

//  for application behaves like MSN but wants to limit number of pages queued.
app::onSendButtonClicked(im)
{
   if(mPageManager.msgQueued() > 5 - 1)
   {
      disableSendButton();
   }
   mPageManager.page(im);
}

app::onSuccess(...)
{
   if(mPageManager.msgQueued() < 5)
   {
      enableSendButton();
   }
}

app::onFailure(...)
{
   reportFailueToUI();
   if(mPageManager.msgQueued() < 5)
   {
      enableSendButton();
   }
}
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

ClientPagerMessageHandle
ClientPagerMessage::getHandle()
{
   return ClientPagerMessageHandle(mDum, getBaseHandle().getId());
}

ClientPagerMessage::ClientPagerMessage(DialogUsageManager& dum, DialogSet& dialogSet)
: NonDialogUsage(dum, dialogSet),
mRequest(dialogSet.getCreator()->getLastRequest())//,
//mInTransaction(false)
{
}

ClientPagerMessage::~ClientPagerMessage()
{
   this->clearMsgQueued();
   mDialogSet.mClientPagerMessage = 0;
}

SipMessage&
ClientPagerMessage::getMessageRequest()
{
   return mRequest;
}

void
ClientPagerMessage::page(std::auto_ptr<Contents> contents,
                         std::auto_ptr< std::map<resip::Data, resip::Data> > extraHeaders)   // syunc.
{
   assert(contents.get() != 0);

   bool do_page = mMsgQueue.empty();
   mMsgQueue.push_back(MessageContents(contents.release(), extraHeaders.release()));
   if(do_page)
   {
      this->pageFirstMsgQueued();
   }
}

void
ClientPagerMessage::pageAsync(std::auto_ptr<Contents> contents, 
                              std::auto_ptr< std::map<resip::Data, resip::Data> > extraHeaders) // !polo! async.
{
   InfoLog(<< "Send pager message (async)");
   mDum.post(new InternalSendPagerMessage(getHandle(), contents, extraHeaders));
}

void
ClientPagerMessage::dispatch(const SipMessage& msg)
{
   assert(msg.isResponse());

   ClientPagerMessageHandler* handler = mDum.mClientPagerMessageHandler;
   assert(handler);

   int code = msg.header(h_StatusLine).statusCode();

   DebugLog ( << "ClientPagerMessageReq::dispatch(msg)" << msg.brief() );
   {
      assert(mMsgQueue.empty() == false);
      if (code < 200) // got 1xx
      {
         DebugLog ( << "ClientPagerMessageReq::dispatch - encountered provisional response" << msg.brief() );
      }
      else if (code < 300) // got 2xx
      {
         if(mMsgQueue.empty() == false)
         {
            MessageContents messageContents = mMsgQueue.front();
            std::auto_ptr<std::map<resip::Data, resip::Data> > extraHeadersMap(messageContents.mExtraHeadersMap);
            messageContents.mExtraHeadersMap = 0;
            messageContents.reset();
            mMsgQueue.pop_front();
            if(mMsgQueue.empty() == false)
            {
               this->pageFirstMsgQueued();
            }

            handler->onSuccess(getHandle(), msg, extraHeadersMap);
         }
      }
      else
      {
         SipMessage errResponse;
         MsgQueue::iterator message;
         for(message = mMsgQueue.begin(); message != mMsgQueue.end(); ++message)
         {
            std::auto_ptr<Contents> contents(message->mContents);
            std::auto_ptr<std::map<resip::Data, resip::Data> > extraHeadersMap(message->mExtraHeadersMap);
            message->mContents = 0;
            message->mExtraHeadersMap = 0;
            WarningLog ( << "Paging failed" << *contents );
            Helper::makeResponse(errResponse, mRequest, code);
            handler->onFailure(getHandle(), errResponse, 
               contents, extraHeadersMap);
            message->reset();
         }

         mMsgQueue.clear();
      }
   }
}
void
ClientPagerMessage::dispatch(const DumTimeout& timer)
{
}

void
ClientPagerMessage::end()  // sync.
{
   delete this;
}

void
ClientPagerMessage::endAsync() // !polo! async.
{
   InfoLog(<< "End client pager (async)");
   mDum.post(new InternalEndClientPagerMessage(getHandle()));
}

size_t
ClientPagerMessage::msgQueued () const
{
   return  mMsgQueue.size();
}

void
ClientPagerMessage::pageFirstMsgQueued ()
{
   assert(mMsgQueue.empty() == false);
   mRequest.setContents(mMsgQueue.front().mContents);
   if (mMsgQueue.front().mExtraHeadersMap)
   {
      for (std::map<resip::Data, resip::Data>::iterator i = mMsgQueue.front().mExtraHeadersMap->begin();
         i != mMsgQueue.front().mExtraHeadersMap->end(); ++i)
      {
         resip::StringCategories& msgSeqExtension = mRequest.header(resip::ExtensionHeader(i->first));
         msgSeqExtension.clear();
         msgSeqExtension.push_back(resip::StringCategory(i->second));
      }
   }

   DebugLog(<< "ClientPagerMessage::pageFirstMsgQueued: " << mRequest);
   mDum.send(mRequest);

   // clean up added extension headers
   if (mMsgQueue.front().mExtraHeadersMap)
   {
      for (std::map<resip::Data, resip::Data>::iterator i = mMsgQueue.front().mExtraHeadersMap->begin();
         i != mMsgQueue.front().mExtraHeadersMap->end(); ++i)
      {
         resip::StringCategories& msgSeqExtension = mRequest.header(resip::ExtensionHeader(i->first));
         msgSeqExtension.clear();
      }
   }
   mRequest.header(h_CSeq).sequence()++;
}

void
ClientPagerMessage::clearMsgQueued ()
{
   MsgQueue::iterator message;
   for(message = mMsgQueue.begin(); message != mMsgQueue.end(); ++message)
   {
      message->reset();
   }
   mMsgQueue.clear();
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
