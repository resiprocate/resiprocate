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
#if defined(WIN32) && !defined(__GNUC__)
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
   mHaveSentFinalResponse(false),
   mOriginalRequest(0),
   mCurrentEvent(0),
   mRequestProcessorChain(requestP),
   mResponseProcessorChain(responseP),
   mTargetProcessorChain(targetP),
   mTransactionCount(1),
   mProxy(proxy),
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
      try
      {
         fixStrictRouterDamage();
         removeTopRouteIfSelf();
      }
      catch(resip::ParseBuffer::Exception& e)
      {
         InfoLog(<<"Parse failure Exception caught: " << e);
         resip::SipMessage response;
         Helper::makeResponse(response, *mOriginalRequest,400); 
         response.header(h_StatusLine).reason()="Malformed header-field-value: " + e.getMessage();
         sendResponse(response);
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<<"Exception caught: " << e);
         resip::SipMessage response;
         Helper::makeResponse(response, *mOriginalRequest,500); 
         response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
         sendResponse(response);
      }
   }

   Processor::processor_action_t ret=Processor::Continue;
   // if it's a CANCEL I need to call processCancel here 
   if (sip->isRequest())
   {
      DebugLog(<<"Got a request.");
      // !bwc! Totally different handling for ACK.
      if(sip->method()==ACK)
      {
         DebugLog(<<"This request is an ACK.");
         // !bwc! This takes care of ACK/200 and stray ACK failure
         // (ie, the ACK has its own transaction)
         if(mOriginalRequest->method() == ACK)
         {
            DebugLog(<<"This ACK has its own tid.");
            if(sip->exists(h_Routes) && !sip->header(h_Routes).empty())
            {
               mResponseContext.cancelAllClientTransactions();
               addTarget(NameAddr(sip->header(h_RequestLine).uri()),true);
            }
            else if(!getProxy().isMyUri(sip->header(h_RequestLine).uri()))
            {
               try
               {
                  if (getProxy().isMyUri(sip->header(h_From).uri()))
                  {
                     mResponseContext.cancelAllClientTransactions();
                     addTarget(NameAddr(sip->header(h_RequestLine).uri()),true);
                  }
                  else
                  {
                     // !bwc! Someone is using us to relay an ACK, but host in
                     // From isn't ours, host in request-uri isn't ours, and no
                     // Route headers. Refusing to do so.
                  }
               }
               catch(resip::ParseBuffer::Exception&)
               {
                  // !bwc! Someone is trying to get us to relay an ACK, but
                  // can't get a host out of From to authorize the relay.
               }
            }
            else
            {
               // !bwc! Someone sent an ACK with us in the Request-Uri, and no
               // Route headers. We will never perform location service or
               // retargeting on an ACK, and we shouldn't send it to ourselves.
               // So, just drop the thing.
               InfoLog(<<"Stray ACK aimed at us. Dropping it...");            
            }

            DebugLog(<<"Posting Ack200DoneMessage");
            mProxy.post(new Ack200DoneMessage(getTransactionId()));
         

         }
         else //This takes care of ACK/failure and malformed ACK/200
         {
            DebugLog(<<"This ACK has the same tid as the original INVITE.");
            DebugLog(<<"The reponse we sent back was a " 
                  << mResponseContext.mBestResponse.header(h_StatusLine).statusCode());
            // !bwc! Since this is not an ACK transaction, the stack will let
            // us know when we need to clean up.
            if(!mHaveSentFinalResponse)
            {
               // !bwc! Whoa, something went wrong here. We got an ACK, but we
               // haven't sent back a final response. The stack shouldn't have
               // allowed this through!
               ErrLog(<<"Got an ACK, but haven't sent a final response. "
                           "What happened here?");
            }
            else if(mResponseContext.mBestResponse.header(h_StatusLine).statusCode() / 100 == 2)
            {
               // !bwc! Ugh. Some bozo didn't change the transaction id for
               // the ACK/200.
               
               InfoLog(<<"Got an ACK within an INVITE transaction, but our "
                        "response was a 2xx. Someone didn't change their tid "
                        "like they were supposed to...");
               if(
                  (
                     sip->exists(h_Routes) && 
                     !sip->header(h_Routes).empty()
                  ) 
                  ||
                  (
                     !getProxy().isMyUri(sip->header(h_RequestLine).uri()) && 
                     (sip->header(h_From).isWellFormed() && getProxy().isMyUri(sip->header(h_From).uri())) 
                  )
                  )
               {
                  forwardAck(*sip);
               }
            }
            
         }
         
         return;
      }
      
      if (sip->method() == CANCEL)
      {
         mResponseContext.processCancel(*sip);
      }
      else
      {
         try
         {
            ret = mRequestProcessorChain.process(*this);
         }
         catch(resip::BaseException& e)
         {
            SipMessage response;
            Helper::makeResponse(response,*mOriginalRequest,500);
            response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
            ErrLog(<<"Exception caught: " << e);
            sendResponse(response);
         }
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
            
            try
            {
               ret = mTargetProcessorChain.process(*this);
            }
            catch(resip::BaseException& e)
            {
               if(mResponseContext.hasActiveTransactions())
               {
                  // !bwc! Whoops. We may have just forwarded garbage upstream.
                  // TODO is it appropriate to try to CANCEL here?
                  ErrLog(<<"Server error caught after"
                                 " request was forwarded. Exception was: "<<e);
               }
               else
               {
                  mResponseContext.clearCandidateTransactions();
                  SipMessage response;
                  Helper::makeResponse(response,*mOriginalRequest,500);
                  response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
                  ErrLog(<<"Exception caught: " << e);
                  sendResponse(response);
               }            
            }

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
      try
      {
         ret = mResponseProcessorChain.process(*this);
      }
      catch(resip::ParseBuffer::Exception& e)
      {
         InfoLog(<<"Garbage in response; dropping message. " << e);
         delete sip;
         return;
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<<"Exception thrown in response processor chain: " << e);
         //!bwc! TODO what do we do here? Continue processing? Give up?
      }

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
               Helper::makeResponse(response, *mOriginalRequest, 500); 
               // The last active transaction has ended, and the response processors
               // did not start any of the pending transactions.
               // Send a 500 response.
               ErrLog( << "In RequestContext, after processing a sip response:"
                       << " We have no active transactions, but there are candidates "
                       << " remaining. (Bad baboon?)"
                       << "Sending a 500 response for this request:" 
                       << mOriginalRequest->header(h_RequestLine).uri() );
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
            try
            {
               ret = mRequestProcessorChain.process(*this);
            }
            catch(resip::BaseException& e)
            {
               resip::SipMessage response;
               Helper::makeResponse(response,*mOriginalRequest,500);
               response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
               ErrLog(<<"Exception caught: " << e);
               sendResponse(response);            
            }

            if(ret != Processor::WaitingForEvent && !mHaveSentFinalResponse)
            {
               if (!mResponseContext.hasTargets())
               {
                  // make 480, send, dispose of memory
                  resip::SipMessage response;
                  Helper::makeResponse(response, *mOriginalRequest, 480); 
                  InfoLog (<< *this << ": no targets for " 
                           << mOriginalRequest->header(h_RequestLine).uri() 
                           << " send 480");
                  sendResponse(response);
               }
               else
               {
                  InfoLog (<< *this << " there are " 
                  << mResponseContext.mCandidateTransactionMap.size() 
                  << " candidates -> continue");

                  try
                  {
                     ret = mTargetProcessorChain.process(*this);
                  }
                  catch(resip::BaseException& e)
                  {
                     if(mResponseContext.hasActiveTransactions())
                     {
                        // !bwc! Whoops. We may have just forwarded garbage upstream.
                        // TODO is it appropriate to try to CANCEL here?
                        ErrLog(<<"Server error caught after"
                                       " request was forwarded. Exception was: "<<e);
                     }
                     else
                     {
                        mResponseContext.clearCandidateTransactions();
                        resip::SipMessage response;
                        Helper::makeResponse(response,*mOriginalRequest,500);
                        response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
                        ErrLog(<<"Exception caught: " << e);
                        sendResponse(response);
                     }            
                  }

                  if(ret != Processor::WaitingForEvent &&
                     !mHaveSentFinalResponse && 
                     !mResponseContext.hasActiveTransactions())
                  {
                     if(mResponseContext.hasCandidateTransactions())
                     {
                        resip::SipMessage response;
                        Helper::makeResponse(response, *mOriginalRequest, 500); 
                        // Someone forgot to start any of the targets they just added.
                        // Send a 500 response
                        ErrLog( << "In RequestContext, request and target processor"
                                 << " chains have run, and we have some Candidate Targets,"
                                 << " but no active Targets. (Bad baboon?)"
                                 << "Sending a 500 response for this request:" 
                                 << mOriginalRequest->header(h_RequestLine).uri() );
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

void
RequestContext::forwardAck(const resip::SipMessage& ack)
{
   resip::SipMessage toSend(ack);
   toSend.header(h_MaxForwards).value()--;
   Helper::processStrictRoute(toSend);
   
   toSend.header(h_Vias).push_front(Via());

   mProxy.send(toSend);
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

resip::Data
RequestContext::getTransactionId() const
{
   if(mOriginalRequest->method()==ACK)
   {
      static Data ack("ack");
      return mOriginalRequest->getTransactionId()+ack;
   }
   else
   {
      return mOriginalRequest->getTransactionId();
   }
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
   

   // We can't respond to an ACK request - so just drop it and generate an Ack200DoneMessage so the request context
   // gets cleaned up properly
   if(mOriginalRequest->method() == ACK)
   {
      ErrLog(<<"Posting Ack200DoneMessage: due to sendResponse(). This is probably a bug.");
      mProxy.post(new Ack200DoneMessage(getTransactionId()));
   }
   else
   {
      //!bwc! Provisionals are not final responses, and CANCEL/200 is not a final
      //response in this context.
      if (msg.header(h_StatusLine).statusCode()>199 && msg.method()!=CANCEL)
      {
         DebugLog(<<"Sending final response.");
         mHaveSentFinalResponse=true;
      }
      mProxy.send(msg);
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
