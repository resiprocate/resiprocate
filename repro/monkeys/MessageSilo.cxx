#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/dum/ServerRegistration.hxx"
#include "repro/monkeys/MessageSilo.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Helper.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/AsyncProcessorMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Logger.hxx"

#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

#define SILO_CLEANUP_PERIOD 86400   // look for expired records at most every 24 hours (86400 seconds)

class AsyncAddToSiloMessage : public AsyncProcessorMessage 
{
public:
   AsyncAddToSiloMessage(AsyncProcessor& proc,
                         const resip::Data& tid,
                         resip::TransactionUser* passedtu):
      AsyncProcessorMessage(proc, tid, passedtu)
   {
   }

   virtual EncodeStream& encode(EncodeStream& strm) const { strm << "AsyncAddToSiloMessage(tid=" << mTid << ", aor=" << mDestUri << ")"; return strm; }

   Data mDestUri;
   Data mSourceUri;
   time_t mOriginalSendTime;
   Data mMimeType;
   Data mMessageBody;
};

class AsyncDrainSiloMessage : public AsyncProcessorMessage 
{
public:
   AsyncDrainSiloMessage(AsyncProcessor& proc,
                         const resip::Data& tid,
                         resip::TransactionUser* passedtu):
      AsyncProcessorMessage(proc, tid, passedtu)
   {
   }

   virtual EncodeStream& encode(EncodeStream& strm) const { strm << "AsyncDrainSiloMessage(aor=" << mAor << ")"; return strm; }

   Data mAor;
   ContactList mRequestContacts;
};

MessageSilo::MessageSilo(ProxyConfig& config, Dispatcher* asyncDispatcher) : 
   AsyncProcessor("MessageSilo", asyncDispatcher),
   mSiloStore(config.getDataStore()->mSiloStore),
   mDestFilterRegex(0),
   mMimeTypeFilterRegex(0),
   mExpirationTime(config.getConfigUnsignedLong("MessageSiloExpirationTime", 2592000 /* 30 days */)),
   mAddDateHeader(config.getConfigBool("MessageSiloAddDateHeader", true)),
   mMaxContentLength(config.getConfigUnsignedLong("MessageSiloMaxContentLength", 4096)),
   mSuccessStatusCode(config.getConfigUnsignedShort("MessageSiloSuccessStatusCode", 202)),
   mFilteredMimeTypeStatusCode(config.getConfigUnsignedShort("MessageSiloFilteredMimeTypeStatusCode", 200)),
   mFailureStatusCode(config.getConfigUnsignedShort("MessageSiloFailureStatusCode", 480)),
   mLastSiloCleanupTime(time(0))  // set to now
{
   Data destFilterRegex = config.getConfigData("MessageSiloDestFilterRegex", "", false);
   Data mimeTypeFilterRegex = config.getConfigData("MessageSiloMimeTypeFilterRegex", "application\\/im\\-iscomposing\\+xml", false);
   if(!destFilterRegex.empty())
   {
      mDestFilterRegex= new regex_t;
      int ret = regcomp(mDestFilterRegex, destFilterRegex.c_str(), REG_EXTENDED | REG_NOSUB);
      if( ret != 0 )
      {
         delete mDestFilterRegex;
         ErrLog( << "MessageSilo has invalid destination filter regular expression: " << destFilterRegex);
         mDestFilterRegex = 0;
      }
   }
   if(!mimeTypeFilterRegex.empty())
   {
      mMimeTypeFilterRegex= new regex_t;
      int ret = regcomp(mMimeTypeFilterRegex, mimeTypeFilterRegex.c_str(), REG_EXTENDED | REG_NOSUB);
      if( ret != 0 )
      {
         delete mMimeTypeFilterRegex;
         ErrLog( << "MessageSilo has invalid mime-type filter regular expression: " << mimeTypeFilterRegex);
         mMimeTypeFilterRegex = 0;
      }
   }
}

MessageSilo::~MessageSilo()
{
   // Clean up pcre memory
   if(mDestFilterRegex)
   {
      regfree(mDestFilterRegex);
      delete mDestFilterRegex;
      mDestFilterRegex = 0;
   }
   if(mMimeTypeFilterRegex)
   {
      regfree(mMimeTypeFilterRegex);
      delete mMimeTypeFilterRegex;
      mMimeTypeFilterRegex = 0;
   }
}

Processor::processor_action_t
MessageSilo::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << context);
   SipMessage& originalRequest = context.getOriginalRequest();

   // Note:  A potential enhancement could be to also silo messages that fail to route due to a 
   // 408 or 503 error.  In order to do this, this processor needs to be part of the ResponseChain
   // as well.

   // Check if request is a MESSAGE request and if there were no targets found
   if(originalRequest.method() == MESSAGE &&
      !context.getResponseContext().hasTargets())
   {
      // There are no targets for this request - silo candidate

      // Only need to silo if there is a message body
      Contents* contents = originalRequest.getContents();
      if(contents)
      {
         // Create async message now, so we can use it's storage and avoid some copies
         AsyncAddToSiloMessage* async = new AsyncAddToSiloMessage(*this, context.getTransactionId(), &context.getProxy());
         std::auto_ptr<ApplicationMessage> async_ptr(async);

         // Check Max ContentLength setting
         async->mMessageBody = contents->getBodyData();
         if(async->mMessageBody.size() > mMaxContentLength)
         {
            InfoLog( << " MESSAGE not silo'd due to content-length exceeding max: " << async->mMessageBody.size());
            SipMessage response;
            Helper::makeResponse(response, originalRequest, mFailureStatusCode);
            context.sendResponse(response);
            return SkipThisChain;
         }

         // Check if message passes Mime-type filter
         async->mMimeType = Data::from(contents->getType());
         if (mMimeTypeFilterRegex)
         {
            int ret = regexec(mMimeTypeFilterRegex, async->mMimeType.c_str(), 0, 0, 0/*eflags*/);
            if (ret == 0)
            {
               // match 
               DebugLog( << " MESSAGE not silo'd due to Mime-Type filter: " << async->mMimeType);
               if(mFilteredMimeTypeStatusCode == 0)
               {
                  return Processor::Continue;
               }
               else
               {
                  SipMessage response;
                  Helper::makeResponse(response, originalRequest, mFilteredMimeTypeStatusCode);
                  context.sendResponse(response);
                  return SkipThisChain;
               }
            }
         }

         // Check if message passes Destination filter
         async->mDestUri = originalRequest.header(h_To).uri().getAOR(false /* addPort? */);
         if (mDestFilterRegex)
         {
            int ret = regexec(mDestFilterRegex, async->mDestUri.c_str(), 0, 0, 0/*eflags*/);
            if (ret == 0)
            {
               // match 
               DebugLog( << " MESSAGE not silo'd due to destination filter: " << async->mDestUri);
               return Processor::Continue;
            }
         }

         // TODO (future) - check a max messages per user setting

         NameAddr from(originalRequest.header(h_From));
         from.remove(p_tag); // remove from tag
         async->mSourceUri = Data::from(from);
         time(&async->mOriginalSendTime);  // Get now timestamp

         // Dispatch async request to worker thread pool
         mAsyncDispatcher->post(async_ptr);

         SipMessage response;
         InfoLog(<<"Message was Silo'd responding with a " << mSuccessStatusCode);
         Helper::makeResponse(response, context.getOriginalRequest(), mSuccessStatusCode);
         context.sendResponse(response);
         return SkipThisChain;
      }
   }

   // In all cases we continue - this is just a passive monkey that stores MESSAGES for later replay to recipient
   return Processor::Continue;
}

bool 
MessageSilo::asyncProcess(AsyncProcessorMessage* msg)
{
   // Running inside a worker thread here
   AsyncAddToSiloMessage* addToSilo = dynamic_cast<AsyncAddToSiloMessage*>(msg);
   if(addToSilo)
   {
      // Check if database cleanup period has passed, and if so run a cleanup pass through the database to remove
      // silo'd messages that have been stored beyond the MessageSiloExpirationTime.  If mExpirationTime is configured
      // as 0, then records never expire, so no need to peform the cleanup.
      // Note: addToSilo->mOriginalSendTime is always now - so no need to requery current time
      // Run cleanup before adding new records to save iterating through 1 extra item
      if(mExpirationTime > 0 && (addToSilo->mOriginalSendTime - mLastSiloCleanupTime) > SILO_CLEANUP_PERIOD)
      {
         mLastSiloCleanupTime = addToSilo->mOriginalSendTime;  // reset stored silo cleanup time

         mSiloStore.cleanupExpiredSiloRecords(addToSilo->mOriginalSendTime, mExpirationTime);
      }

      // TODO - look for addMessage failures and queue up to be attempted to be written later (ie. when db is back and live)
      mSiloStore.addMessage(addToSilo->mDestUri, addToSilo->mSourceUri, addToSilo->mOriginalSendTime, addToSilo->getTransactionId(), addToSilo->mMimeType, addToSilo->mMessageBody);
      return false;
   }

   AsyncDrainSiloMessage* drainSilo = dynamic_cast<AsyncDrainSiloMessage*>(msg);
   if(drainSilo)
   {
      AbstractDb::SiloRecordList recordList;
      if(mSiloStore.getSiloRecords(drainSilo->mAor, recordList))
      {
         time_t now = time(0);

         // Note:  Tesing with BerkeleyDb and MySQL reveals that these databases return the records in insert order
         //        so there is no need to sort the records here.

         AbstractDb::SiloRecordList::iterator siloIt = recordList.begin();
         for(; siloIt != recordList.end(); siloIt++)
         {
            DebugLog(<< "DrainSilo:  Dest=" << siloIt->mDestUri << ", Source=" << siloIt->mSourceUri << ", Datetime=" << Data::from(DateCategory(siloIt->mOriginalSentTime)) << ", MimeType=" << siloIt->mMimeType << ", Body=" << siloIt->mMessageBody);
            
            // Only send if not too old
            if((unsigned long)(now - siloIt->mOriginalSentTime) <= mExpirationTime)
            {
               ContactList::iterator contactIt = drainSilo->mRequestContacts.begin();
               for(; contactIt != drainSilo->mRequestContacts.end(); contactIt++)
               {
                  // send messages to each contact from register message - honour path
                  ContactInstanceRecord& rec = *contactIt;

                  // Removed contacts can be in the list, but they will be expired, don't send to them
                  if(rec.mRegExpires > (UInt64)now)  
                  {
                     std::auto_ptr<SipMessage> msg(new SipMessage);
                     RequestLine rLine(MESSAGE);
                     rLine.uri() = rec.mContact.uri();
                     msg->header(h_RequestLine) = rLine;
                     msg->header(h_To) = NameAddr(siloIt->mDestUri);
                     msg->header(h_MaxForwards).value() = 20;
                     msg->header(h_CSeq).method() = MESSAGE;
                     msg->header(h_CSeq).sequence() = 1;
                     msg->header(h_From) = NameAddr(siloIt->mSourceUri);
                     msg->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
                     msg->header(h_CallId).value() = Helper::computeCallId();   
                     Via via;
                     msg->header(h_Vias).push_back(via);

                     // add routes from registration path
                     if(!rec.mSipPath.empty())
                     {
                        msg->header(h_Routes).append(rec.mSipPath);
                     }

                     // Add Date Header if enabled
                     if(mAddDateHeader)
                     {
                        msg->header(h_Date) = DateCategory(siloIt->mOriginalSentTime);
                     }

                     if(rec.mUseFlowRouting &&
                        rec.mReceivedFrom.mFlowKey)
                     {
                        // .bwc. We only override the destination if we are sending to an
                        // outbound contact. If this is not an outbound contact, but the
                        // endpoint has given us a Contact with the correct ip-address and 
                        // port, we might be able to find the connection they formed when they
                        // registered earlier, but that will happen down in TransportSelector.
                        msg->setDestination(rec.mReceivedFrom);
                     }

                     // Helper::processStrictRoute(*msg.get());  // Path headers must have ;lr so this isn't required

                     // Add mime body
                     HeaderFieldValue hfv(siloIt->mMessageBody.data(), siloIt->mMessageBody.size());
                     Mime type;
                     ParseBuffer pb(siloIt->mMimeType);
                     type.parse(pb);
                     PlainContents contents(hfv, type);
                     msg->setContents(&contents);  // need to clone since body data isn't owned by message yet

                     mAsyncDispatcher->mStack->send(msg);
                  }
               }
            }

            // Delete record from database
            // Note:  A potential feature enhancement would be to monitor the MESSAGE reponses and only remove
            //        from the database when a 200 reponses is seen.  Care must be taken to avoid
            //        looping and handle scenarios when a user never uses a device capable of IM.
            mSiloStore.deleteSiloRecord(siloIt->mOriginalSentTime, siloIt->mTid);
         }
      }
      return false;
   }

   return false; // Nothing to queue to stack
}

bool
MessageSilo::onAdd(resip::ServerRegistrationHandle h, const resip::SipMessage& reg)
{
   // Dispatch request to check message silo for newly registered user
   AsyncDrainSiloMessage* async = new AsyncDrainSiloMessage(*this, Data::Empty, 0);  // tid and tu not needed since no response expected
   async->mAor = reg.header(h_To).uri().getAOR(false /* addPort? */);
   async->mRequestContacts = h->getRequestContacts();
   std::auto_ptr<ApplicationMessage> async_ptr(async);
   mAsyncDispatcher->post(async_ptr);
   return true;
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
