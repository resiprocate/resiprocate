#include <cassert>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/PublicationHandler.hxx"
#include "resiprocate/dum/InternalClientPublicationMessage.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientPublicationHandle 
ClientPublication::getHandle()
{
   return ClientPublicationHandle(mDum, getBaseHandle().getId());
}

ClientPublication::ClientPublication(DialogUsageManager& dum,
                                     DialogSet& dialogSet,
                                     SipMessage& req)
   : NonDialogUsage(dum, dialogSet),
     mWaitingForResponse(false),
     mPendingPublish(false),
     mPublish(req),
     mEventType(req.header(h_Event).value()),
     mTimerSeq(0),
     mDocument(mPublish.releaseContents().release())
{
   DebugLog( << "ClientPublication::ClientPublication: " << mId);   
}

ClientPublication::~ClientPublication()
{
   DebugLog( << "ClientPublication::~ClientPublication: " << mId);   
   mDialogSet.mClientPublication = 0;
   delete mDocument;
}

void
ClientPublication::refreshAsync(unsigned int expiration)
{
   mDum.post(new InternalClientPublicationMessage_Refresh(getHandle(), expiration));
}

void
ClientPublication::updateAsync(std::auto_ptr<Contents> body)
{
   InfoLog (<< "Updating presence document (Async): " << mPublish.header(h_To).uri());
   mDum.post(new InternalClientPublicationMessage_Update(getHandle(), body));
}

void
ClientPublication::endAsync()
{
   InfoLog (<< "End client publication (Async) to " << mPublish.header(h_RequestLine).uri());
   mDum.post(new InternalClientPublicationMessage_End(getHandle()));
}

void
ClientPublication::dispatchAsync(const SipMessage& msg)
{
   InfoLog (<< "Dispatch client publication msg (async)");
   mDum.post(new InternalClientPublicationMessage_SipMsg(getHandle(), msg));
}

void
ClientPublication::dispatchAsync(const DumTimeout& timer)
{
   InfoLog (<< "Dispatch client publication timeout msg (async)");
   mDum.post(new InternalClientPublicationMessage_TimeoutMsg(getHandle(), timer));
}

void
ClientPublication::end()
{
   InfoLog (<< "End client publication (Sync) to " << mPublish.header(h_RequestLine).uri());
   mPublish.header(h_CSeq).sequence()++;
   mPublish.header(h_Expires).value() = 0;
   send(mPublish);
}

void 
ClientPublication::dispatch(const SipMessage& msg)
{
   ClientPublicationHandler* handler = mDum.getClientPublicationHandler(mEventType);
   assert(handler);   

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
         if (mPublish.header(h_Expires).value() == 0)
         {
            handler->onRemove(getHandle(), msg);
            delete this;
            return;
         }
         else if (msg.exists(h_SIPETag) && msg.exists(h_Expires))
         {
            mPublish.releaseContents();
            mPublish.header(h_SIPIfMatch) = msg.header(h_SIPETag);
            mDum.addTimer(DumTimeout::Publication, 
                          Helper::smallerThan(msg.header(h_Expires).value()), 
                          getBaseHandle(),
                          ++mTimerSeq);
            handler->onSuccess(getHandle(), msg);
         }
         else
         {
            // Any PUBLISH/200 must have an ETag. This should not happen. Not
            // sure what the app can do in this case. 
            WarningLog (<< "PUBLISH/200 received with no ETag " << mPublish.header(h_From).uri());
            handler->onFailure(getHandle(), msg);
            delete this;
            return;
         }
      }
      else
      {
         if (code == 412)
         {
            InfoLog(<< "SIPIfMatch failed -- republish");
            mPublish.remove(h_SIPIfMatch);
            update(mDocument);
            return;
         }
         else if (code == 415)
         {
            InfoLog(<< "415 body required -- republish with body");
            update(mDocument);
            return;
         }
         else if (code == 423) // interval too short
         {
            if (msg.exists(h_MinExpires))
            {
               mPublish.header(h_Expires).value() = msg.header(h_MinExpires).value();
               refresh();
            }
            else
            {
               handler->onFailure(getHandle(), msg);
               delete this;
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
               handler->onFailure(getHandle(), msg);
               delete this;
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
               mDum.addTimer(DumTimeout::Publication, 
                             retry, 
                             getBaseHandle(),
                             ++mTimerSeq);       
               return;
               
            }
         }
         else
         {
            InfoLog(<< "Received " << msg.header(h_StatusLine).statusCode() << " to PUBLISH "
               << mPublish.header(h_To) << " : publish in 20 seconds");

            mDum.addTimer(DumTimeout::Publication, 
               20,
               getBaseHandle(),
               ++mTimerSeq);
 
            //handler->onFailure(getHandle(), msg);
            //delete this;
            return;
         }

      }

      if (mPendingPublish)
      {
         InfoLog (<< "Sending pending PUBLISH: " << mPublish.brief());
         send(mPublish);
      }
   }
}

void 
ClientPublication::dispatch(const DumTimeout& timer)
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
      expiration = mPublish.header(h_Expires).value();
   }
   mPublish.header(h_CSeq).sequence()++;
   send(mPublish);
}

void
ClientPublication::update(const Contents* body) // Sync.
{
   InfoLog (<< "Updating presence document (Sync): " << mPublish.header(h_To).uri());

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

   mPublish.header(h_CSeq).sequence()++;
   mPublish.setContents(mDocument);
   send(mPublish);
}

void 
ClientPublication::send(SipMessage& request)
{
   if (mWaitingForResponse)
   {
      DebugLog(<< "Waiting For Pending Publish now, pending message:\n" << request);
      mPendingPublish = true;
   }
   else
   {
      mDum.send(request);
      mWaitingForResponse = true;
      mPendingPublish = false;
   }
}

std::ostream& 
ClientPublication::dump(std::ostream& strm) const
{
   strm << "ClientPublication " << mId << " " << mPublish.header(h_From).uri();
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
