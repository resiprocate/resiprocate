
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/dum/PagerMessageCreator.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "resip/dum/DumHelper.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Helper.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

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
     mRequest(dialogSet.getCreator()->getLastRequest()),
     mEnded(false)
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
   return *mRequest;
}

void
ClientPagerMessage::page(std::auto_ptr<Contents> contents,
                         DialogUsageManager::EncryptionLevel level)
{
    resip_assert(contents.get() != 0);
    bool do_page = mMsgQueue.empty();
    Item item;
    item.contents = contents.release();
    item.encryptionLevel = level;
    mMsgQueue.push_back(item);
    if(do_page)
    {
       this->pageFirstMsgQueued();
    }
}

class ClientPagerMessagePageCommand : public DumCommandAdapter
{
public:
   ClientPagerMessagePageCommand(const ClientPagerMessageHandle& clientPagerMessageHandle, 
      std::auto_ptr<Contents> contents,
      DialogUsageManager::EncryptionLevel level)
      : mClientPagerMessageHandle(clientPagerMessageHandle),
        mContents(contents),
        mLevel(level)
   {

   }

   virtual void executeCommand()
   {
      if(mClientPagerMessageHandle.isValid())
      {
         mClientPagerMessageHandle->page(mContents, mLevel);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientPagerMessagePageCommand";
   }
private:
   ClientPagerMessageHandle mClientPagerMessageHandle;
   std::auto_ptr<Contents> mContents;
   DialogUsageManager::EncryptionLevel mLevel;
};

void
ClientPagerMessage::pageCommand(std::auto_ptr<Contents> contents,
                                DialogUsageManager::EncryptionLevel level)
{
   mDum.post(new ClientPagerMessagePageCommand(getHandle(), contents, level));
}

void
ClientPagerMessage::dispatch(const SipMessage& msg)
{
   resip_assert(msg.isResponse());

   ClientPagerMessageHandler* handler = mDum.mClientPagerMessageHandler;
   resip_assert(handler);

   int code = msg.header(h_StatusLine).statusCode();

   DebugLog ( << "ClientPagerMessageReq::dispatch(msg)" << msg.brief() );
   {
      if (code < 200)
      {
         DebugLog ( << "ClientPagerMessageReq::dispatch - encountered provisional response" << msg.brief() );
      }
      else if (code < 300)
      {
         if(mMsgQueue.empty() == false)
         {
            delete mMsgQueue.front().contents;
            mMsgQueue.pop_front();
            if(mMsgQueue.empty() == false)
            {
               this->pageFirstMsgQueued();
            }
         }
         handler->onSuccess(getHandle(), msg);
      }
      else
      {
         if(!mMsgQueue.empty())
         {
            SipMessage errResponse;
            MsgQueue::iterator contents;
            for(contents = mMsgQueue.begin(); contents != mMsgQueue.end(); ++contents)
            {
               Contents* p = contents->contents;
               WarningLog ( << "Paging failed " << *p );
               Helper::makeResponse(errResponse, *mRequest, code);
               handler->onFailure(getHandle(), errResponse, std::auto_ptr<Contents>(p));
               contents->contents = 0;
            }
            mMsgQueue.clear();
         }
         else
         {
            handler->onFailure(getHandle(), msg, std::auto_ptr<Contents>(mRequest->releaseContents()));
         }
      }
   }
}

void
ClientPagerMessage::dispatch(const DumTimeout& timer)
{
}

void
ClientPagerMessage::end()
{
   if(!mEnded)
   {
      mEnded = true;
      mDum.destroy(this);
   }
}

class ClientPagerMessageEndCommand : public DumCommandAdapter
{
public:
   ClientPagerMessageEndCommand(ClientPagerMessage& clientPagerMessage)
      : mClientPagerMessage(clientPagerMessage)
   {
   }

   virtual void executeCommand()
   {
      mClientPagerMessage.end();
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientPagerMessageEndCommand";
   }
private:
   ClientPagerMessage& mClientPagerMessage;
};

void
ClientPagerMessage::endCommand()
{
   mDum.post(new ClientPagerMessageEndCommand(*this));
}

size_t
ClientPagerMessage::msgQueued () const
{
   return  mMsgQueue.size();
}

void
ClientPagerMessage::pageFirstMsgQueued ()
{
   resip_assert(mMsgQueue.empty() == false);
   mRequest->header(h_CSeq).sequence()++;
   mRequest->setContents(mMsgQueue.front().contents);
   DumHelper::setOutgoingEncryptionLevel(*mRequest, mMsgQueue.front().encryptionLevel);
   DebugLog(<< "ClientPagerMessage::pageFirstMsgQueued: " << *mRequest);
   mDum.send(mRequest);
}

void
ClientPagerMessage::clearMsgQueued ()
{
   MsgQueue::iterator   contents;
   for(contents = mMsgQueue.begin(); contents != mMsgQueue.end(); ++contents)
   {
      Contents* p = contents->contents;
      delete p;
   }
   mMsgQueue.clear();
}

EncodeStream& 
ClientPagerMessage::dump(EncodeStream& strm) const
{
   strm << "ClientPagerMessage queued: " << mMsgQueue.size();
   return strm;
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
