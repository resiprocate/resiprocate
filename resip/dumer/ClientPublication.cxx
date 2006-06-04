#include <cassert>

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dummer/ClientPublication.hxx"
#include "resip/dummer/DumTimeout.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/PublicationHandler.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientPublication::ClientPublication()
   : NonDialogPrd(),
     mWaitingForResponse(false),
     mPendingPublish(false),
     mTimerSeq(0),
     mDocument(0)
{
   mDocument(mLastRequest->releaseContents().release())
   mEventType(mLastRequest->header(h_Event).value()),

   DebugLog( << "ClientPublication::ClientPublication: " << mId);   
}

ClientPublication::~ClientPublication()
{
   DebugLog( << "ClientPublication::~ClientPublication: " << mId);   
   mDialogSet.mClientPublication = 0;

   if (mDocument)
   {
      delete mDocument;
   }
}

SipMessage&
ClientPublication::initialize(const NameAddr& target, 
                              const Contents& body, 
                              const Data& eventType, 
                              unsigned expiresSeconds)
{
   makeInitialRequest(target, PUBLISH);

   mLastRequest->header(h_Event).value() = eventType;
   mLastRequest->setContents(&body);
   mLastRequest->header(h_Expires).value() = expireSeconds;
}

void
ClientPublication::end()
{
   InfoLog (<< "End client publication to " << mLastRequest.header(h_RequestLine).uri());
   mLastRequest.header(h_CSeq).sequence()++;
   mLastRequest.header(h_Expires).value() = 0;
   send(mLastRequest);
}

void 
ClientPublication::protectedDispatch(SipMessage& msg)
{
   if (msg.isRequest())
   {
      DebugLog( << "Dropping stray request to ClientPublication usage: " << msg);
   }
   else
   {
      const int code = msg.header(h_StatusLine).statusCode();
      if (code < 200)
      {
         return;
      }

      assert(code >= 200);
      mWaitingForResponse = false;

      if (code < 300)
      {
         if (mLastRequest.header(h_Expires).value() == 0)
         {
            onRemove(msg);

            unmanage();

            return;
         }
         else if (msg.exists(h_SIPETag) && msg.exists(h_Expires))
         {
            mLastRequest.header(h_SIPIfMatch) = msg.header(h_SIPETag);
            mLastRequest.releaseContents();
            addTimer(DumTimeout::Publication, 
                     Helper::aBitSmallerThan(msg.header(h_Expires).value()), 
                     getBaseHandle(),
                     ++mTimerSeq);
            onSuccess(msg);
         }
         else
         {
            // Any PUBLISH/200 must have an ETag. This should not happen. Not
            // sure what the app can do in this case. 
            WarningLog (<< "PUBLISH/200 received with no ETag " << mLastRequest.header(h_From).uri());
            onFailure(msg);
            
            unmanage();

            return;
         }
      }
      else
      {
         if (code == 412)
         {
            InfoLog(<< "SIPIfMatch failed -- republish");
            mLastRequest.remove(h_SIPIfMatch);
            update(mDocument);
            return;
         }         
         else if (code == 423) // interval too short
         {
            if (msg.exists(h_MinExpires))
            {
               mLastRequest.header(h_Expires).value() = msg.header(h_MinExpires).value();
               update(mDocument); // !dys! since contents not released until on success, no need to call update any more.
            }
            else
            {
               onFailure(msg);
               
               unmanage();

               return;
            }
         }
         else if (code == 408 ||
                  ((code == 404 ||
                    code == 413 ||
                    code == 480 ||
                    code == 486 ||
                    code == 500 ||
                    code == 503 ||
                    code == 600 ||
                    code == 603) &&
                   msg.exists(h_RetryAfter)))
         {
            int retryMinimum = 0;
            if (msg.exists(h_RetryAfter))
            {
               retryMinimum = msg.header(h_RetryAfter).value();
            }

            // RFC 3261:20.33 Retry-After
            int retry = handler->onRequestRetry(getHandle(), retryMinimum, msg);
            if (retry < 0)
            {
               DebugLog(<< "Application requested failure on Retry-After");
               onFailure(msg);
               
               unmanage();

               return;
            }
            else if (retry == 0 && retryMinimum == 0)
            {
               DebugLog(<< "Application requested immediate retry on Retry-After");
               refresh();
               return;
            }
            else
            {
               retry = resipMax(retry, retryMinimum);
               DebugLog(<< "Application requested delayed retry on Retry-After: " << retry);
               addTimer(DumTimeout::Publication, 
                        retry, 
                        getBaseHandle(),
                        ++mTimerSeq);       
               return;
               
            }
         }
         else
         {
            onFailure(getHandle(), msg);
            
            unmanage();

            return;
         }

      }

      if (mPendingPublish)
      {
         InfoLog (<< "Sending pending PUBLISH: " << mLastRequest.brief());
         send(mLastRequest);
      }
   }
}

void 
ClientPublication::protectedDispatch(DumTimeout& timer)
{
    if (timer.seq() == mTimerSeq)
    {
       refresh();
    }
}

void
ClientPublication::refresh(unsigned int expiration)
{
   if (expiration == 0)
   {
      expiration = mLastRequest.header(h_Expires).value();
   }
   mLastRequest.header(h_CSeq).sequence()++;
   send(mLastRequest);
}

void
ClientPublication::update(const Contents* body)
{
   InfoLog (<< "Updating presence document: " << mLastRequest.header(h_To).uri());

   if (mDocument != body)
   {
      delete mDocument;
      if (body)
      {
         mDocument = body->clone();
      }
      else
      {
         mDocument = body;
      }
   }

   mLastRequest.header(h_CSeq).sequence()++;
   mLastRequest.setContents(mDocument);
   send(mLastRequest);
}

void 
ClientPublication::send(SipMessage& request)
{
   if (mWaitingForResponse)
   {
      mPendingPublish = true;
   }
   else
   {
      NonDialogPrd::send(request);

      mWaitingForResponse = true;
      mPendingPublish = false;
   }
}

std::ostream& 
ClientPublication::dump(std::ostream& strm) const
{
   strm << "ClientPublication " << mId << " " << mLastRequest.header(h_From).uri();
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
