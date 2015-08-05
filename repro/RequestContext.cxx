#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>

#include "repro/Proxy.hxx"
#include "repro/RequestContext.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionTerminated.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"

#include "repro/Ack200DoneMessage.hxx"
#include "repro/ForkControlMessage.hxx"
#include "repro/ProcessorMessage.hxx"
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
   mAck200ToRetransmit(0),
   mRequestProcessorChain(requestP),
   mResponseProcessorChain(responseP),
   mTargetProcessorChain(targetP),
   mTransactionCount(1),
   mProxy(proxy),
   mResponseContext(*this),
   mTCSerial(0),
   mSessionCreatedEventSent(false),
   mSessionEstablishedEventSent(false),
   mKeyValueStore(*Proxy::getRequestKeyValueStoreKeyAllocator())
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
   delete mAck200ToRetransmit;
   mAck200ToRetransmit=0;
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
   bool original = false;
   InfoLog (<< "RequestContext::process(SipMessage) " << sipMessage->getTransactionId());

   if (mCurrentEvent != mOriginalRequest)
   {
      delete mCurrentEvent;
   }
   mCurrentEvent = sipMessage.release();
   
   SipMessage* sip = dynamic_cast<SipMessage*>(mCurrentEvent);
   if (!mOriginalRequest) 
   { 
      resip_assert(sip);
      mOriginalRequest=sip;
      original = true;
      mResponseContext.mIsClientBehindNAT = InteropHelper::getClientNATDetectionMode() != InteropHelper::ClientNATDetectionDisabled && 
                                            Helper::isClientBehindNAT(*sip, 
                                                  InteropHelper::getClientNATDetectionMode() == InteropHelper::ClientNATDetectionPrivateToPublicOnly);
     
      // RFC 3261 Section 16.4
      try
      {
         fixStrictRouterDamage();
         removeTopRouteIfSelf();
      }
      catch(resip::ParseException& e)
      {
         InfoLog(<<"Parse failure Exception caught: " << e);
         if(mOriginalRequest->method()==ACK)
         {
            postAck200Done();
         }
         else
         {
            resip::SipMessage response;
            Helper::makeResponse(response, *mOriginalRequest,400); 
            response.header(h_StatusLine).reason()="Malformed header-field-value: " + e.getMessage();
            sendResponse(response);
         }
         return;
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<<"Exception caught: " << e);
         if(mOriginalRequest->method()==ACK)
         {
            postAck200Done();
         }
         else
         {
            resip::SipMessage response;
            Helper::makeResponse(response, *mOriginalRequest,500); 
            response.header(h_StatusLine).reason()="Server error: " + e.getMessage();
            sendResponse(response);
         }
         return;
      }
   }

   if (sip->isRequest())
   {
      DebugLog(<<"Got a request.");
      bool postProcess=false;

      Uri& requestUri = sip->header(h_RequestLine).uri();
      if(requestUri.exists(resip::p_wsSrcIp) &&
            requestUri.exists(resip::p_wsSrcPort) &&
            !isWebSocket(sip->getSource().getType()))
      {
         requestUri.host() = requestUri.param(resip::p_wsSrcIp);
         requestUri.remove(resip::p_wsSrcIp);
         requestUri.port() = requestUri.param(resip::p_wsSrcPort);
         requestUri.remove(resip::p_wsSrcPort);
         requestUri.param(resip::p_transport) = "WS";
         DebugLog(<< "recognised request for WS peer, setting forceTarget to " << requestUri);
         sip->setForceTarget(requestUri);
      }

      switch(mOriginalRequest->method())
      {
         case ACK:
            processRequestAckTransaction(sip,original);
            break;
         case INVITE:
            postProcess=processRequestInviteTransaction(sip,original);
            if(postProcess) doPostRequestProcessing(sip,original);
            break;
         default:
            postProcess=processRequestNonInviteTransaction(sip,original);
            if(postProcess) doPostRequestProcessing(sip,original);
      }
   }
   else if (sip->isResponse())
   {
      resip_assert(!original);
      bool postProcess=false;
      switch(mOriginalRequest->method())
      {
         case ACK:
            // !bwc! Got a response to an ACK? 
            // Why did the stack let this through?
            resip_assert(0);
            break;
         case INVITE:
            postProcess=processResponseInviteTransaction(sip);
            break;
         default:
            postProcess=processResponseNonInviteTransaction(sip);
      }

      if(postProcess) 
      {
         doPostResponseProcessing(sip);
      }
   }
}

bool
RequestContext::processRequestInviteTransaction(SipMessage* msg, bool original)
{
   bool doPostProcess=false;
   resip_assert(msg->isRequest());
   
   if(original)
   {
      resip_assert(msg->method()==INVITE);

      try
      {
         Processor::processor_action_t ret=Processor::Continue;
         ret = mRequestProcessorChain.process(*this);
         if(ret!=Processor::WaitingForEvent && !mHaveSentFinalResponse)
         {
            doPostProcess=true;
         }
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
   else
   {
      if(msg->method()==CANCEL)
      {
         if(mSessionCreatedEventSent && !mSessionEstablishedEventSent)
         {
            getProxy().doSessionAccounting(*msg, true /* received */, *this);
         }
         mResponseContext.processCancel(*msg);
         doPostProcess=true;
      }
      else if(msg->method()==ACK)
      {
         // .bwc. The stack should not be forwarding ACK/failure to the TU,
         // nor should we be getting a bad ACK/200. (There is code further
         // up that makes bad ACK/200 look like a new transaction, like it
         // is supposed to be.)
         // TODO Remove this code block entirely.
         resip_assert(0);
         
         DebugLog(<<"This ACK has the same tid as the original INVITE.");
         DebugLog(<<"The reponse we sent back was a " 
               << mResponseContext.mBestResponse.header(h_StatusLine).statusCode());
         // .bwc. Since this is not an ACK transaction, the stack will let
         // us know when we need to clean up.
         if(!mHaveSentFinalResponse)
         {
            // .bwc. Whoa, something went wrong here. We got an ACK, but we
            // haven't sent back a final response. The stack shouldn't have
            // allowed this through!
            ErrLog(<<"Got an ACK, but haven't sent a final response. "
                        "What happened here?");
         }
         else if(mResponseContext.mBestResponse.header(h_StatusLine).statusCode() / 100 == 2)
         {
            InfoLog(<<"Got an ACK within an INVITE transaction, but our "
                     "response was a 2xx. Someone didn't change their tid "
                     "like they were supposed to...");
            if((msg->exists(h_Routes) && !msg->header(h_Routes).empty()) ||   // If ACK/200 has a Route header   OR
               (!getProxy().isMyUri(msg->header(h_RequestLine).uri()) &&      // RequestUri is not us and From Uri is our domain
                (msg->header(h_From).isWellFormed() && getProxy().isMyUri(msg->header(h_From).uri()))))
            {
               forwardAck200(*msg);
            }
         }
      }
      else
      {
         // .bwc. The stack should not have done this. This indicates either a 
         // bug in the stack, processInvite was called in a non-invite 
         // RequestContext, or this RequestContext was leaked and hit with a 
         // subsequent transaction 
         ErrLog(<<"We got an unexpected request from "
                                    "the stack in an invite RequestContext. "
                                    "Why?"
                                    " Orig: " << mOriginalRequest->brief() <<
                                    " This: " << msg->brief());
         resip_assert(0);
      }
   }

   return doPostProcess;

}

bool
RequestContext::processRequestNonInviteTransaction(SipMessage* msg, bool original)
{
   resip_assert(msg->isRequest());
   bool doPostProcess=false;
   
   if(original)
   {
      resip_assert(msg->method()==mOriginalRequest->method());
      try
      {
         Processor::processor_action_t ret=Processor::Continue;
         ret = mRequestProcessorChain.process(*this);
         if(ret!=Processor::WaitingForEvent && !mHaveSentFinalResponse)
         {
            doPostProcess=true;
         }
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
   else
   {
      if(msg->method()==CANCEL)
      {
         // .bwc. Got a CANCEL in a non-invite transaction. Just respond with
         // 200 and ignore.
         SipMessage response;
         Helper::makeResponse(response,*msg,200);
         send(response);
      }
      else
      {
         // ?bwc? We got a second request from the stack. Why?
         ErrLog(<<"We got a second non-invite request from the"
                                    " stack in an already-established non-"
                                    "invite RequestContext. "
                                    "Why?"
                                    " Orig: " << mOriginalRequest->brief() <<
                                    " This: " << msg->brief());
         if(msg->method()!=ACK)
         {
            SipMessage response;
            Helper::makeResponse(response,*msg,500);
            response.header(h_StatusLine).reason()="Server error: got an "
                           "unexpected request in a non-invite RequestContext";
            send(response);
         }
         resip_assert(0);
      }
   }

   return doPostProcess;
}

void
RequestContext::processRequestAckTransaction(SipMessage* msg, bool original)
{
   resip_assert(msg->isRequest());
   if(msg->method()!=ACK)
   {
      // !bwc! Somebody collided with an ACK/200. Send a failure response.
      SipMessage response;
      Helper::makeResponse(response,*msg,400);
      response.header(h_StatusLine).reason()="Transaction-id collision";
      send(response);
      return;
   }
   
   DebugLog(<<"This ACK has its own tid.");

   try
   {
      // .slg. look at mOriginalRequest for Routes since removeTopRouteIfSelf() is only called on mOriginalRequest
      if((!mOriginalRequest->exists(h_Routes) || mOriginalRequest->header(h_Routes).empty()) &&
          getProxy().isMyUri(msg->header(h_RequestLine).uri()))
      {
         // .bwc. Someone sent an ACK with us in the Request-Uri, and no
         // Route headers (after we have removed ourself). We will never perform 
         // location service or retargeting on an ACK, and we shouldn't send 
         // it to ourselves.  So, just drop the thing.
         handleSelfAimedStrayAck(msg);
      }
      // Note: mTopRoute is only populated if RemoveTopRouteIfSelf successfully removes the top route.
      else if(msg->hasForceTarget() || !mTopRoute.uri().host().empty() || getProxy().isMyUri(msg->header(h_From).uri()))
      {
         // Top most route is us, or From header uri is ours.  Note:  The From check is 
         // required to interoperate with endpoints that configure outbound proxy 
         // settings, and do not place the outbound proxy in a Route header.
         mResponseContext.cancelAllClientTransactions();
         forwardAck200(*mOriginalRequest);
      }
      else
      {
         // .slg. Someone is using us to relay an ACK, but we are not the 
         // top-most route and the host in From isn't ours. Refusing to do so.
         InfoLog(<<"Top most route or From header are not ours.  We do not allow relaying ACKs.  Dropping it...");            
      }
   }
   catch(resip::ParseException&)
   {
      InfoLog(<<"Parse error processing ACK. Dropping it...");            
   }

   if(original)  // Only queue Ack200Done if this is the original request
   {
      postAck200Done();
   }
}

void
RequestContext::doPostRequestProcessing(SipMessage* msg, bool original)
{
   resip_assert(msg->isRequest());
   
   // .bwc. This is called after an incoming request is done processing. This
   // IS NOT called if the request-processor chain goes async, and IS NOT called
   // when async work finishes. The intent of this function is to prompt either:
   // 1) The initiation of new client transactions.
   // 2) Failing 1, the sending of a failure response to the original request.
   
   
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
      Processor::processor_action_t ret=Processor::Continue;
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
            // .bwc. Whoops. We may have just forwarded garbage upstream.
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

bool
RequestContext::processResponseInviteTransaction(SipMessage* msg)
{
   resip_assert(msg->isResponse());
   
   bool doPostProcessing=false;
   resip::Data tid(msg->getTransactionId());
   tid.lowercase();
   if(msg->method()==INVITE)
   {
      try
      {
         Processor::processor_action_t ret = Processor::Continue;
         ret = mResponseProcessorChain.process(*this);
         resip_assert(ret != Processor::WaitingForEvent);
         
         if (ret == Processor::Continue)
         {
            doPostProcessing=true;
         }
         else
         {
            // This means the response has been eaten. Do not forward back.
            mResponseContext.terminateClientTransaction(tid);
         }
      }
      catch(resip::ParseException& e)
      {
         InfoLog(<<"Garbage in response; dropping message. " << e);
         mResponseContext.terminateClientTransaction(tid);
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<<"Exception thrown in response processor chain: " << e);
         // ?bwc? TODO what do we do here? Continue processing? Give up?
         mResponseContext.terminateClientTransaction(tid);
      }
   }
   else if(msg->method()==CANCEL)
   {
      // .bwc. Do nothing.
   }
   else
   {
      // ?bwc? Is this possible?
      resip_assert(0);
   }
   
   return doPostProcessing;
}

bool
RequestContext::processResponseNonInviteTransaction(SipMessage* msg)
{
   resip_assert(msg->isResponse());
   
   resip::Data tid(msg->getTransactionId());
   tid.lowercase();
   bool doPostProcessing=false;
   if(msg->method()==mOriginalRequest->method())
   {
      try
      {
         Processor::processor_action_t ret = Processor::Continue;
         ret = mResponseProcessorChain.process(*this);
         resip_assert(ret != Processor::WaitingForEvent);
         
         if (ret == Processor::Continue)
         {
            doPostProcessing=true;
         }
         else
         {
            // This means the response has been eaten. Do not forward back.
            mResponseContext.terminateClientTransaction(tid);
         }
      }
      catch(resip::ParseException& e)
      {
         InfoLog(<<"Garbage in response; dropping message. " << e);
         mResponseContext.terminateClientTransaction(tid);
      }
      catch(resip::BaseException& e)
      {
         ErrLog(<<"Exception thrown in response processor chain: " << e);
         // ?bwc? TODO what do we do here? Continue processing? Give up?
         mResponseContext.terminateClientTransaction(tid);
      }
   }
   else
   {
      // ?bwc? Is this possible?
      resip_assert(0);
   }
   
   return doPostProcessing;

}

void
RequestContext::doPostResponseProcessing(SipMessage* msg)
{
   bool nit408 = msg->method() != INVITE && msg->header(resip::h_StatusLine).statusCode() == 408;

   mResponseContext.processResponse(*msg);

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
      else if(nit408)
      {
         InfoLog(<<"In RequestContext, after processing a NIT/408, all"
               << " transactions are terminated. In this case, we do not send a"
               << " final response.");
      }
      else
      {
         ErrLog(<<"In RequestContext, after processing "
         << "a sip response (_not_ a NIT/408): all transactions are terminated,"
         << " but we have not sent a final response. (What happened here?) ");

         // Send best response if there is one - otherwise send 500
         if(mResponseContext.mBestResponse.isResponse())
         {
            mResponseContext.forwardBestResponse();
         }
         else
         {
            resip::SipMessage response;
            Helper::makeResponse(response, *mOriginalRequest, 500); 
            sendResponse(response);
         }
      }
   }
}

void
RequestContext::process(std::auto_ptr<ApplicationMessage> app)
{
   InfoLog (<< "RequestContext::process(ApplicationMessage) " << *app);

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

   ProcessorMessage* proc=dynamic_cast<ProcessorMessage*>(mCurrentEvent);
   
   if(proc)
   {
      Processor::ChainType type = proc->chainType();
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
                        // .bwc. Whoops. We may have just forwarded garbage upstream.
                        // ?bwc? TODO is it appropriate to try to CANCEL here?
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
RequestContext::handleSelfAimedStrayAck(SipMessage* sip)
{
   InfoLog(<<"Stray ACK aimed at us that routes back to us. Dropping it...");  
}

void
RequestContext::cancelClientTransaction(const resip::Data& tid)
{
   getProxy().getStack().cancelClientInviteTransaction(tid);
}

void
RequestContext::forwardAck200(const resip::SipMessage& ack)
{
   if(!mAck200ToRetransmit)
   {
      mAck200ToRetransmit = new SipMessage(ack);
      mAck200ToRetransmit->header(h_MaxForwards).value()--;
      Helper::processStrictRoute(*mAck200ToRetransmit);
      
      mAck200ToRetransmit->header(h_Vias).push_front(Via());

      // .bwc. Check for flow-token
      if(!mTopRoute.uri().user().empty())
      {
         resip::Tuple dest(Tuple::makeTupleFromBinaryToken(mTopRoute.uri().user().base64decode(), Proxy::FlowTokenSalt));
         if(!(dest==resip::Tuple()))
         {
            // valid flow token
            mAck200ToRetransmit->setDestination(dest);
         }
      }
   }

   send(*mAck200ToRetransmit);
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
   if(mOriginalRequest->mIsBadAck200)
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
RequestContext::postAck200Done()
{
   resip_assert(mOriginalRequest->method()==ACK);
   DebugLog(<<"Posting Ack200DoneMessage");
   // .bwc. This needs to have a timer attached to it. (We need to
   // wait until all potential retransmissions of the ACK/200 have
   // stopped. However, we must be mindful that we may receive a new,
   // non-ACK transaction with the same tid during this time, and make
   // sure we don't explode violently when this happens.)
   mProxy.postMS(
      std::auto_ptr<ApplicationMessage>(new Ack200DoneMessage(getTransactionId())),
      64*resip::Timer::T1);
}

void
RequestContext::send(SipMessage& msg)
{
   mProxy.send(msg);
}

void
RequestContext::sendResponse(SipMessage& msg)
{
   resip_assert (msg.isResponse());
   
   // We can't respond to an ACK request - so just drop it and generate an Ack200DoneMessage so the request context
   // gets cleaned up properly
   if(mOriginalRequest->method() == ACK)
   {
      ErrLog(<<"Posting Ack200DoneMessage: due to sendResponse(). This is probably a bug.");
      postAck200Done();
   }
   else
   {
      DebugLog(<< "tid of orig req: " << mOriginalRequest->getTransactionId());
      resip::Data tid;
      try
      {
         tid=msg.getTransactionId();
      }
      catch(BaseException&) // Could be SipMessage::Exception or ParseException
      {
         InfoLog(<< "Bad tid in response. Trying to replace with 2543 tid "
                  "from orig request.");
         tid=mOriginalRequest->getRFC2543TransactionId();
         // .bwc. If the original request didn't have a proper transaction-
         // id, the response will not either. We need to set the tid in the 
         // response in order to make sure that this response hits the 
         // correct transaction down in the stack.
         msg.setRFC2543TransactionId(tid);
      }
      
      if(tid!=mOriginalRequest->getTransactionId())
      {
         InfoLog(<<"Someone messed with the Via stack in a response. This "
                        "is not only bad behavior, but potentially malicious. "
                        "Response came from: " << msg.getSource() <<
                        " Request came from: " << 
                        mOriginalRequest->getSource() << 
                        " Via after modification (in response): " <<
                        msg.header(h_Vias).front() <<
                        " Via before modification (in orig request): " <<
                        mOriginalRequest->header(h_Vias).front());
         // .bwc. Compensate for malicous/broken UAS fiddling with Via stack.
         msg.header(h_Vias).front()=mOriginalRequest->header(h_Vias).front();
      }

      DebugLog(<<"Ensuring orig tid matches tid of response: " <<
               msg.getTransactionId() << " == " <<
               mOriginalRequest->getTransactionId());
      resip_assert(msg.getTransactionId()==mOriginalRequest->getTransactionId());
      
      // .bwc. Provisionals are not final responses, and CANCEL/200 is not a final
      //response in this context.
      if (msg.header(h_StatusLine).statusCode()>199 && msg.method()!=CANCEL)
      {
         DebugLog(<<"Sending final response.");
         mHaveSentFinalResponse=true;
      }
      const resip::Data& serverText = mProxy.getServerText();
      if (!serverText.empty() && !msg.exists(h_Server) ) 
      {
         msg.header(h_Server).value() = serverText;
      }
      if(mSessionCreatedEventSent && !mSessionEstablishedEventSent)
      {
         getProxy().doSessionAccounting(msg, false /* received */, *this);
      }
      send(msg);
   }
}

// This function assumes that if ;lr shows up in the RURI, that it's a URI 
// we put in a Record-Route header earlier. It will do the wrong thing if 
// some other malbehaving implementation lobs something at us with
// ;lr in the RURI and it wasn't us. (from Section 16.4 of RFC 3261)
void
RequestContext::fixStrictRouterDamage()
{
   if(mOriginalRequest->header(h_RequestLine).uri().exists(p_lr))
   {
      if (mOriginalRequest->exists(h_Routes)
          && !mOriginalRequest->header(h_Routes).empty())
      {
         mOriginalRequest->header(h_RequestLine).uri() = mOriginalRequest->header(h_Routes).back().uri();
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
   if(mOriginalRequest->exists(h_Routes)
      && !mOriginalRequest->header(h_Routes).empty()
      &&  mProxy.isMyUri(mOriginalRequest->header(h_Routes).front().uri()))
   {
      // save the top-most Route header field so monkeys can check it later
      mTopRoute = mOriginalRequest->header(h_Routes).front();

      mOriginalRequest->header(h_Routes).pop_front();

      static ExtensionParameter p_drr("drr");
      if(mTopRoute.uri().exists(p_drr))
      {
         if(!mOriginalRequest->header(h_Routes).empty()
              &&  mProxy.isMyUri(mOriginalRequest->header(h_Routes).front().uri()))
         {
            // .bwc. Do double-record routing logic
            mTopRoute = mOriginalRequest->header(h_Routes).front();
      
            mOriginalRequest->header(h_Routes).pop_front();
         }
         else
         {
            // ?bwc? Somebody messed with our record-routes. Just ignore? Or 
            // should we reject?
         }
      }
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

const resip::Data&
RequestContext::getDigestRealm()
{
   // (1) Check Preferred Identity
   if (mOriginalRequest->exists(h_PPreferredIdentities))
   {
      // !abr! Add this when we get a chance
      // find the fist sip or sips P-Preferred-Identity header
      // for (;;)
      // {
      //    if ((i->uri().scheme() == Symbols::SIP) || (i->uri().scheme() == Symbols::SIPS))
      //    {
      //       return i->uri().host();
      //    }
      // }
   }

   // (2) Check From domain
   if (mProxy.isMyDomain(mOriginalRequest->header(h_From).uri().host()))
   {
      return mOriginalRequest->header(h_From).uri().host();
   }

   // (3) Check Top Route Header
   if (mOriginalRequest->exists(h_Routes) &&
         mOriginalRequest->header(h_Routes).size()!=0 &&
         mOriginalRequest->header(h_Routes).front().isWellFormed())
   {
      // !abr! Add this when we get a chance
   }

   // (4) Punt: Use Request URI
   return mOriginalRequest->header(h_RequestLine).uri().host();
}

EncodeStream&
repro::operator<<(EncodeStream& strm, const RequestContext& rc)
{
   strm << "numtrans=" << rc.mTransactionCount
        << " final=" << rc.mHaveSentFinalResponse;
   if(!rc.mDigestIdentity.empty()) strm << " identity=" << rc.mDigestIdentity;
   if (rc.mOriginalRequest) strm << " req=" << rc.mOriginalRequest->brief();
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
