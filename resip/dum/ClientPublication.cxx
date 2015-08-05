#include "rutil/ResipAssert.h"

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumTimeout.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/PublicationHandler.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientPublicationHandle 
ClientPublication::getHandle()
{
   return ClientPublicationHandle(mDum, getBaseHandle().getId());
}

ClientPublication::ClientPublication(DialogUsageManager& dum,
                                     DialogSet& dialogSet,
                                     SharedPtr<SipMessage> req)
   : NonDialogUsage(dum, dialogSet),
     mPublished(false),
     mWaitingForResponse(false),
     mPendingPublish(false),
     mPendingEnd(false),
     mPublish(req),
     mEventType(req->header(h_Event).value()),
     mTimerSeq(0),
     mDocument(mPublish->releaseContents().release())
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
ClientPublication::end()
{
   end(false);
}

void
ClientPublication::end(bool immediate)
{
   if (immediate)
   {
      InfoLog(<< "End client publication immediately to " << mPublish->header(h_RequestLine).uri());
      delete this;
      return;
   }
   if (mWaitingForResponse)
   {
      InfoLog(<< "Waiting for response, pending End of client publication to " << mPublish->header(h_RequestLine).uri());
      mPendingEnd = true;
      return;
   }
   if (mPublished)
   {
      InfoLog(<< "End client publication to " << mPublish->header(h_RequestLine).uri());
      mPublish->header(h_Expires).value() = 0;
      mPublish->releaseContents();
      send(mPublish);
   }
   else
   {
      InfoLog(<< "End client publication immediately (not published) to " << mPublish->header(h_RequestLine).uri());
      delete this;
   }
}

class ClientPublicationEndCommand : public DumCommandAdapter
{
public:
   ClientPublicationEndCommand(const ClientPublicationHandle& clientPublicationHandle, bool immediate)
      : mClientPublicationHandle(clientPublicationHandle), mImmediate(immediate)
   {
   }

   virtual void executeCommand()
   {
      if(mClientPublicationHandle.isValid())
      {
         mClientPublicationHandle->end(mImmediate);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientPublicationEndCommand";
   }
private:
   ClientPublicationHandle mClientPublicationHandle;
   bool mImmediate;
};

void
ClientPublication::endCommand(bool immediate)
{
   mDum.post(new ClientPublicationEndCommand(getHandle(), immediate));
}

void 
ClientPublication::dispatch(const SipMessage& msg)
{
   ClientPublicationHandler* handler = mDum.getClientPublicationHandler(mEventType);
   resip_assert(handler);   

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

      resip_assert(code >= 200);
      mWaitingForResponse = false;

      if (code < 300)
      {
         mPublished = true;
         if (mPublish->exists(h_Expires) && mPublish->header(h_Expires).value() == 0)
         {
            handler->onRemove(getHandle(), msg);
            delete this;
            return;
         }
         else if (msg.exists(h_SIPETag) && msg.exists(h_Expires))
         {
            mPublish->header(h_SIPIfMatch) = msg.header(h_SIPETag);
            if(!mPendingPublish)
            {
               mPublish->releaseContents();
            }
            mDum.addTimer(DumTimeout::Publication, 
                          Helper::aBitSmallerThan(msg.header(h_Expires).value()), 
                          getBaseHandle(),
                          ++mTimerSeq);
            handler->onSuccess(getHandle(), msg);
         }
         else
         {
            // Any PUBLISH/200 must have an ETag. This should not happen. Not
            // sure what the app can do in this case. 
            WarningLog (<< "PUBLISH/200 received with no ETag " << mPublish->header(h_From).uri());
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
            mPublish->remove(h_SIPIfMatch);
            update(mDocument);
            return;
         }         
         else if (code == 423) // interval too short
         {
            if (msg.exists(h_MinExpires))
            {
               mPublish->header(h_Expires).value() = msg.header(h_MinExpires).value();
               update(mDocument); // !dys! since contents not released until on success, no need to call update any more.
            }
            else
            {
               handler->onFailure(getHandle(), msg);
               delete this;
               return;
            }
         }
         else if (code == 408 ||
                  (code == 503 && !msg.isFromWire()) ||
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
            handler->onFailure(getHandle(), msg);
            delete this;
            return;
         }
      }

      if (mPendingEnd)
      {
         mPendingEnd = false;
         if (mPublished)
         {
            mPublish->header(h_Expires).value() = 0;
            mPublish->releaseContents();
            InfoLog(<< "Sending pending end PUBLISH: " << mPublish->brief());
            send(mPublish);
         }
         else
         {
             InfoLog(<< "Pending end PUBLISH, but not published, so ending immediately: " << mPublish->brief());
             delete this;
             return;
         }
      }
      else if (mPendingPublish)
      {
         InfoLog (<< "Sending pending PUBLISH: " << mPublish->brief());
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
   if (expiration != 0)
   {
       mPublish->header(h_Expires).value() = expiration;
   }
   send(mPublish);
}

class ClientPublicationRefreshCommand : public DumCommandAdapter
{
public:
   ClientPublicationRefreshCommand(const ClientPublicationHandle& clientPublicationHandle, unsigned int expiration)
      : mClientPublicationHandle(clientPublicationHandle),
        mExpiration(expiration)
   {
   }

   virtual void executeCommand()
   {
      if(mClientPublicationHandle.isValid())
      {
         mClientPublicationHandle->refresh(mExpiration);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientPublicationRefreshCommand";
   }

private:
   ClientPublicationHandle mClientPublicationHandle;
   unsigned int mExpiration;
};

void
ClientPublication::refreshCommand(unsigned int expiration)
{
   mDum.post(new ClientPublicationRefreshCommand(getHandle(), expiration));
}

void
ClientPublication::update(const Contents* body)
{
   InfoLog (<< "Updating presence document: " << mPublish->header(h_To).uri());

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

   mPublish->setContents(mDocument);
   send(mPublish);
}

class ClientPublicationUpdateCommand : public DumCommandAdapter
{
public:
   ClientPublicationUpdateCommand(const ClientPublicationHandle& clientPublicationHandle, const Contents* body)
      : mClientPublicationHandle(clientPublicationHandle),
      mBody(body?body->clone():0)
   {
   }

   virtual void executeCommand()
   {
      if(mClientPublicationHandle.isValid())
      {
         mClientPublicationHandle->update(mBody.get());
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientPublicationUpdateCommand";
   }

private:
   ClientPublicationHandle mClientPublicationHandle;
   std::auto_ptr<Contents> mBody;
};

void
ClientPublication::updateCommand(const Contents* body)
{
   mDum.post(new ClientPublicationUpdateCommand(getHandle(), body));
}

void 
ClientPublication::send(SharedPtr<SipMessage> request)
{
   if (mWaitingForResponse)
   {
      mPendingPublish = true;
   }
   else
   {
      request->header(h_CSeq).sequence()++;
      mDum.send(request);
      mWaitingForResponse = true;
      mPendingPublish = false;
   }
}

EncodeStream& 
ClientPublication::dump(EncodeStream& strm) const
{
   strm << "ClientPublication " << mId << " " << mPublish->header(h_From).uri();
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
