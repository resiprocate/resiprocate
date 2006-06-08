
#include "resip/dumer/PageModePrd.hxx"
#include "resip/dumer/EncryptionLevel.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


PageModePrd::PageModePrd()
{
}

PageModePrd::~PageModePrd()
{
}

SipMessage&
PageModePrd::initialize(const NameAddr& target)
{
   // Initialize a request for UAC
   makeInitialRequest(target, MESSAGE);

   //rfc3428 section 9 - remove the header that may have been added by the BaseCreator and are not allowed
   mLastRequest->remove(h_Supporteds);
   mLastRequest->remove(h_AcceptEncodings);
   mLastRequest->remove(h_AcceptLanguages);
   mLastRequest->remove(h_Contacts);

   return mLastRequest;
}

void 
PageModePrd::protectedDispatch(std::auto_ptr<SipMessage msg)
{
   if (msg->isRequest())
   {
      onMessageArrived(*msg);
   }
   else
   {
      assert(msg->isResponse());

      int code = msg->header(h_StatusLine).statusCode();

      DebugLog ( << "ClientPagerMessageReq::dispatch(msg)" << msg->brief() );

      assert(mOutboundMsgQueue.empty() == false);
      if (code < 200)
      {
         DebugLog ( << "ClientPagerMessageReq::dispatch - encountered provisional response" << msg->brief() );
      }
      else if (code < 300)
      {
         if(mOutboundMsgQueue.empty() == false)
         {
            delete mOutboundMsgQueue.front().contents;
            mOutboundMsgQueue.pop_front();
            if(mOutboundMsgQueue.empty() == false)
            {
               pageFirstMsgQueued();
            }

            onSuccess(*msg);
         }
      }
      else
      {
         SipMessage errResponse;
         MsgQueue::iterator contents;
         for(contents = mOutboundMsgQueue.begin(); contents != mOutboundMsgQueue.end(); ++contents)
         {
            Contents* p = contents->contents;
            WarningLog ( << "Paging failed" << *p );
            Helper::makeResponse(errResponse, *mRequest, code);
            onFailure(errResponse, std::auto_ptr<Contents>(p));
            contents->contents = 0;
         }
         mOutboundMsgQueue.clear();
      }
   }
}

void
PageModePrd::protectedDispatch(std::auto_ptr<DumTimeout>)
{
}

void
PageModePrd::clearMsgQueued ()
{
   MsgQueue::iterator it = mOutboundMsgQueue.begin();
   for(; contents != mOutboundMsgQueue.end(); ++contents)
   {
      Contents* p = contents->contents;
      delete p;
   }
   mOutboundMsgQueue.clear();
}

void
PageModePrd::pageFirstMsgQueued ()
{
   assert(!mOutboundMsgQueue.empty());

   mLastRequest.header(h_CSeq).sequence()++;
   mLastRequest->setContents(mOutboundMsgQueue.front().contents);
   DumHelper::setOutgoingEncryptionLevel(*mLastRequest, mOutboundMsgQueue.front().encryptionLevel);
   DebugLog(<< "ClientPagerMessage::pageFirstMsgQueued: " << *mLastRequest);
   
   send(mLastRequest);
}

void
PageModePrd::page(std::auto_ptr<Contents> contents,
                  EncryptionLevel level)
{
    assert(contents.get() != 0);
    bool do_page = mOutboundMsgQueue.empty();
    Item item;
    item.contents = contents.release();
    item.encryptionLevel = level;
    mOutboundMsgQueue.push_back(item);
    if(do_page)
    {
       pageFirstMsgQueued();
    }
}

SipMessage&
PageModePrd::accept(int statusCode)
{   
   //!dcm! -- should any responses include a contact?
   makeResponse(mResponse, mRequest, statusCode);
   mResponse->remove(h_Contacts);   
   return mResponse;
}

SipMessage&
PageModePrd::reject(int statusCode)
{
   //!dcm! -- should any responses include a contact?
   makeResponse(mResponse, mRequest, statusCode);
   return mResponse;
}

void
PageModePrd::end()
{
   unmanage();
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