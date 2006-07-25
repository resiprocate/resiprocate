#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <iostream>

#include "repro/Proxy.hxx"
#include "repro/RequestContext.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionTerminated.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"

#include "repro/Ack200DoneMessage.hxx"
#include "repro/ForkControlMessage.hxx"
#include "repro/ChainTraverser.hxx"
#include "rutil/WinLeakCheck.hxx"

// Remove warning about 'this' use in initiator list
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

RequestContext::RequestContext(Proxy& proxy, 
                               ProcessorChain& requestP,
                               ProcessorChain& responseP,
                               ProcessorChain& targetP) :
   mOriginalRequest(0),
   mCurrentEvent(0),
   mRequestProcessorChain(requestP),
   mResponseProcessorChain(responseP),
   mTargetProcessorChain(targetP),
   mTransactionCount(1),
   mProxy(proxy),
   mHaveSentFinalResponse(false),
   mTargetConnectionId(0),
   mResponseContext(*this),
   mTCSerial(0),
   mFromTrustedNode(false)
{
   mInitialTimerCSet=false;
}

RequestContext::~RequestContext()
{
   DebugLog (<< "RequestContext::~RequestContext() " << this);
   if (mOriginalRequest != mCurrentEvent)
   {
      delete mOriginalRequest;
      mOriginalRequest = 0;
   }
   delete mCurrentEvent;
   mCurrentEvent = 0;
}


void
RequestContext::process(resip::TransactionTerminated& msg)
{
   InfoLog (<< "RequestContext::process(TransactionTerminated) " 
            << msg.getTransactionId() << " : " << *this);

   if (msg.isClientTransaction())
   {
      mResponseContext.removeClientTransaction(msg.getTransactionId());
   }
   mTransactionCount--;
   if (mTransactionCount == 0)
   {
      delete this;
   }
}

void
RequestContext::process(std::auto_ptr<resip::SipMessage> sipMessage)
{
   DebugLog (<< "process(SipMessage) " << *this);

   if (mCurrentEvent != mOriginalRequest)
   {
      delete mCurrentEvent;
   }
   mCurrentEvent = sipMessage.release();
   
   SipMessage* sip = dynamic_cast<SipMessage*>(mCurrentEvent);
   if (!mOriginalRequest) 
   { 
      assert(sip);
      mOriginalRequest=sip;
	  
	  // RFC 3261 Section 16.4
      fixStrictRouterDamage();
      removeTopRouteIfSelf();
   }

   Processor::processor_action_t ret=Processor::Continue;
   // if it's a CANCEL I need to call processCancel here 
   if (sip->isRequest())
   {
      if (sip->header(h_RequestLine).method() == CANCEL)
      {
         mResponseContext.processCancel(*sip);
      }
      else
      {
         ret = mRequestProcessorChain.process(*this);
      }
      
      if(ret!=Processor::WaitingForEvent && !mHaveSentFinalResponse)
      {
         // if target list is empty return a 480
         if (!mResponseContext.hasTargets())
         {
            // make 480, send, dispose of memory
            resip::SipMessage response;
            InfoLog (<< *this << ": no targets for " 
                     << mOriginalRequest->header(h_RequestLine).uri() 
                     << " send 480");
            Helper::makeResponse(response, *mOriginalRequest, 480); 
            sendResponse(response);
         }
         else
         {
            InfoLog (<< *this << " there are " 
            << mResponseContext.mCandidateTransactionMap.size() 
            << " candidates -> continue");
            
            ret = mTargetProcessorChain.process(*this);

            if(ret != Processor::WaitingForEvent &&
               !mHaveSentFinalResponse && 
               !mResponseContext.hasActiveTransactions())
            {
               if(mResponseContext.hasCandidateTransactions())
               {
                  // Someone forgot to start any of the targets they just added.
                  // Send a 500 response
                  resip::SipMessage response;
                  ErrLog(  << "In RequestContext, target processor chain appears "
                           << "to have failed to process any targets. (Bad baboon?)"
                           << "Sending a 500 response for this request:" 
                           << mOriginalRequest->header(h_RequestLine).uri() );
                  Helper::makeResponse(response, *mOriginalRequest, 500); 
                  sendResponse(response);
               }
               else
               {
                  ErrLog(<< "In RequestContext, request processor chain "
                  << " appears to have added Targets, but all of these Targets"
                  << " are already Terminated. Further, there are no candidate"
                  << " Targets. (Bad monkey?)");

                  // Send best response
                  mResponseContext.forwardBestResponse();
               }
            }
         }
      }
   }
   else if (sip->isResponse())
   {
      // Do the lemurs if its a response (response processor chain)
      // Call handle Response if its a response
      
      Processor::processor_action_t ret = Processor::Continue;
      ret = mResponseProcessorChain.process(*this);

      // TODO
      // this is temporarily not allowed.  Allowing async requests in the
      // response chain means we will need to maintain a collection of all of the
      // outstanding responses that are still processing. 
      assert(ret != Processor::WaitingForEvent);
      
      if (ret == Processor::Continue) 
      {
         mResponseContext.processResponse(*sip);

         //If everything we have tried so far has gone quiescent, we
         //need to fire up some more Targets (if there are any left)
         mTargetProcessorChain.process(*this);

         if(!mHaveSentFinalResponse && 
            !mResponseContext.hasActiveTransactions())
         {
            if(mResponseContext.hasCandidateTransactions())
            {
               resip::SipMessage response;
               // The last active transaction has ended, and the response processors
               // did not start any of the pending transactions.
               // Send a 500 response.
               ErrLog( << "In RequestContext, after processing a sip response:"
                       << " We have no active transactions, but there are candidates "
                       << " remaining. (Bad baboon?)"
                       << "Sending a 500 response for this request:" 
                       << mOriginalRequest->header(h_RequestLine).uri() );
               Helper::makeResponse(response, *mOriginalRequest, 500); 
               sendResponse(response);
            }
            else
            {
               ErrLog(<<"In RequestContext, after processing "
               << "a sip response: all transactions are terminated, but we"
               << " have not sent a final response. (What happened here?) ");

               // Send best response
               mResponseContext.forwardBestResponse();
            }
         }
         
         return;
      }
   }
}


void
RequestContext::process(std::auto_ptr<ApplicationMessage> app)
{
   DebugLog (<< "process(ApplicationMessage) " << *app);


   if (mCurrentEvent != mOriginalRequest)
   {
      delete mCurrentEvent;
   }
   mCurrentEvent = app.release();


   Ack200DoneMessage* ackDone = dynamic_cast<Ack200DoneMessage*>(mCurrentEvent);
   if (ackDone)
     {
       delete this;
       return;
     }

   TimerCMessage* tc = dynamic_cast<TimerCMessage*>(mCurrentEvent);

   if(tc)
   {
      if(tc->mSerial == mTCSerial)
      {
         mResponseContext.processTimerC();
      }

      return;
   }



   ChainTraverser* ct=dynamic_cast<ChainTraverser*>(mCurrentEvent);
   
   if(ct)
   {
      Processor::ChainType type = ct->chainType();
      Processor::processor_action_t ret=Processor::Continue;
      
      switch(type)
      {
         case Processor::REQUEST_CHAIN:
            ret = mRequestProcessorChain.process(*this);
            
            if(ret != Processor::WaitingForEvent && !mHaveSentFinalResponse)
            {
               if (!mResponseContext.hasTargets())
               {
                  // make 480, send, dispose of memory
                  resip::SipMessage response;
                  InfoLog (<< *this << ": no targets for " 
                           << mOriginalRequest->header(h_RequestLine).uri() 
                           << " send 480");
                  Helper::makeResponse(response, *mOriginalRequest, 480); 
                  sendResponse(response);
               }
               else
               {
                  InfoLog (<< *this << " there are " 
                  << mResponseContext.mCandidateTransactionMap.size() 
                  << " candidates -> continue");
                  
                  ret = mTargetProcessorChain.process(*this);

                  if(ret != Processor::WaitingForEvent &&
                     !mHaveSentFinalResponse && 
                     !mResponseContext.hasActiveTransactions())
                  {
                     if(mResponseContext.hasCandidateTransactions())
                     {
                        resip::SipMessage response;
                        // Someone forgot to start any of the targets they just added.
                        // Send a 500 response
                        ErrLog( << "In RequestContext, request and target processor"
                                << " chains have run, and we have some Candidate Targets,"
                                << " but no active Targets. (Bad baboon?)"
                                << "Sending a 500 response for this request:" 
                                << mOriginalRequest->header(h_RequestLine).uri() );
                        Helper::makeResponse(response, *mOriginalRequest, 500); 
                        sendResponse(response);
                     }
                     else if(mResponseContext.mBestResponse.header(h_StatusLine).statusCode() != 408)
                     {
                        ErrLog(<< "In RequestContext, request and target processor "
                        << "chains have run, and all Targets are now Terminated."
                        << " However, we have not sent a final response, and our "
                        << "best final response is not a 408.(What happened here?)");

                        // Send best response
                        mResponseContext.forwardBestResponse();
                     }
                  }

               }
            }
            

            break;
         
         case Processor::RESPONSE_CHAIN:
            ret = mResponseProcessorChain.process(*this);
            
            mTargetProcessorChain.process(*this);
            break;
            
         case Processor::TARGET_CHAIN:
            ret = mTargetProcessorChain.process(*this);
            break;
      
         default:
            ErrLog(<<"RequestContext " << getTransactionId() << " got a "
                     << "ProcessorMessage addressed to a non existent chain "
                     << type);
      }
   }
}



resip::SipMessage& 
RequestContext::getOriginalRequest()
{
   return *mOriginalRequest;
}

const resip::SipMessage& 
RequestContext::getOriginalRequest() const
{
   return *mOriginalRequest;
}

const resip::Data&
RequestContext::getTransactionId() const
{
   return mOriginalRequest->getTransactionId();
}

resip::Message* 
RequestContext::getCurrentEvent()
{
   return mCurrentEvent;
}

const resip::Message* 
RequestContext::getCurrentEvent() const
{
   return mCurrentEvent;
}

void 
RequestContext::setDigestIdentity (const resip::Data& data)
{
   mDigestIdentity = data;
}

const resip::Data& 
RequestContext::getDigestIdentity() const
{
   return mDigestIdentity;
}

resip::Data
RequestContext::addTarget(const NameAddr& addr, bool beginImmediately)
{
   InfoLog (<< "Adding candidate " << addr);
   repro::Target target(addr);
   mResponseContext.addTarget(target,beginImmediately);
   return target.tid();
}


void
RequestContext::updateTimerC()
{
   InfoLog(<<"Updating timer C.");
   mTCSerial++;
   TimerCMessage* tc = new TimerCMessage(this->getTransactionId(),mTCSerial);
   mProxy.postTimerC(std::auto_ptr<TimerCMessage>(tc));
}

void
RequestContext::postTimedMessage(std::auto_ptr<resip::ApplicationMessage> msg,int seconds)
{
   mProxy.postMS(msg,seconds);
}


void
RequestContext::sendResponse(const SipMessage& msg)
{
   assert (msg.isResponse());
   

   // We can't respond to an ACK request - so just drop it and generate an Ack200DoneMessage so the request contexts
   // gets cleaned up properly
   if(mOriginalRequest->header(h_RequestLine).method() == ACK)
   {
      DebugLog(<<"Posting Ack200DoneMessage");
      static Data ack("ack");
      mProxy.post(new Ack200DoneMessage(getTransactionId()+ack));
   }
   else
   {
      mProxy.send(msg);
   }
   
   //!bwc! Provisionals are not final responses, and CANCEL/200 is not a final
   //response in this context.
   if (msg.header(h_StatusLine).statusCode()>199 && msg.header(h_CSeq).method()!=CANCEL)
   {
      DebugLog(<<"Sending final response.");
      mHaveSentFinalResponse=true;
   }
}

//      This function assumes that if ;lr shows up in the
//      RURI, that it's a URI we put in a Record-Route header
//      earlier. It will do the wrong thing if some other 
//      malbehaving implementation lobs something at us with
//      ;lr in the RURI and it wasn't us.
//		(from Section 16.4 of RFC 3261)
void
RequestContext::fixStrictRouterDamage()
{
   if (mOriginalRequest->header(h_RequestLine).uri().exists(p_lr))
   {
      if (    mOriginalRequest->exists(h_Routes)
              && !mOriginalRequest->header(h_Routes).empty())
      {
         mOriginalRequest->header(h_RequestLine).uri()=
            mOriginalRequest->header(h_Routes).back().uri();
         mOriginalRequest->header(h_Routes).pop_back();
      }
      else
      {
         //!RjS! When we wire this class for logging, here's a
         //      place to log a warning
      }
   }
}

/** @brief Pops the topmost route if it's us */
void
RequestContext::removeTopRouteIfSelf()
{
   if (    mOriginalRequest->exists(h_Routes)
           && !mOriginalRequest->header(h_Routes).empty()
           &&  mProxy.isMyUri(mOriginalRequest->header(h_Routes).front().uri())
      )
   {
      // save the top-most Route header field so monkeys can check it later
      mTopRoute = mOriginalRequest->header(h_Routes).front();

      mOriginalRequest->header(h_Routes).pop_front();
   }
}

Proxy& 
RequestContext::getProxy()
{
   return mProxy;
}

ResponseContext&
RequestContext::getResponseContext()
{
   return mResponseContext;
}

NameAddr&
RequestContext::getTopRoute()
{
   return mTopRoute;
}

void
RequestContext::setTargetConnection(ConnectionId cid)
{
   mTargetConnectionId = cid;
}

void 
RequestContext::setFromTrustedNode()
{
   mFromTrustedNode = true;
}

bool 
RequestContext::fromTrustedNode() const
{
   return mFromTrustedNode;
}

std::ostream&
repro::operator<<(std::ostream& strm, const RequestContext& rc)
{
   strm << "RequestContext: "
        << " identity=" << rc.mDigestIdentity
        << " count=" << rc.mTransactionCount
        << " final=" << rc.mHaveSentFinalResponse;

   if (rc.mOriginalRequest) strm << " orig requri=" << rc.mOriginalRequest->brief();
   //if (rc.mCurrentEvent) strm << " current=" << rc.mCurrentEvent->brief();
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
