#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "repro/ProcessorChain.hxx"
#include "repro/Proxy.hxx"
#include "repro/Ack200DoneMessage.hxx"
#include "repro/UserStore.hxx"
#include "repro/Dispatcher.hxx"

#include "resip/stack/TransactionTerminated.hxx"
#include "resip/stack/ApplicationMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


Proxy::Proxy(SipStack& stack, 
             const Uri& recordRoute,
             bool enableRecordRoute,
             ProcessorChain& requestP, 
             ProcessorChain& responseP, 
             ProcessorChain& targetP, 
             UserStore& userStore,
             int timerC) 
   : TransactionUser(TransactionUser::RegisterForTransactionTermination),
     mStack(stack), 
     mRecordRoute(recordRoute),
     mRecordRouteEnabled(enableRecordRoute),
     mRequestProcessorChain(requestP), 
     mResponseProcessorChain(responseP),
     mTargetProcessorChain(targetP),
     mUserStore(userStore)
{
   mTimerC=timerC;
   if (!mRecordRoute.uri().host().empty())
   {
      mRecordRoute.uri().param(p_lr);
   }
}


Proxy::~Proxy()
{
   shutdown();
   join();
   InfoLog (<< "Proxy::thread shutdown with " << mServerRequestContexts.size() << " ServerRequestContexts and " << mClientRequestContexts.size() << " ClientRequestContexts.");
}


bool
Proxy::isShutDown() const
{
  return false;
}


UserStore&
Proxy::getUserStore()
{
   return mUserStore;
}


void
Proxy::thread()
{
   InfoLog (<< "Proxy::thread start");

   while (!isShutdown())
   {
      Message* msg=0;
      //DebugLog (<< "TransactionUser::postToTransactionUser " << " &=" << &mFifo << " size=" << mFifo.size());

      try
      {
         if ((msg = mFifo.getNext(100)) != 0)
         {
            DebugLog (<< "Got: " << *msg);
         
            SipMessage* sip = dynamic_cast<SipMessage*>(msg);
            ApplicationMessage* app = dynamic_cast<ApplicationMessage*>(msg);
            TransactionTerminated* term = dynamic_cast<TransactionTerminated*>(msg);
         
            if (sip)
            {
               if (sip->isRequest())
               {
                  // Verify that the request has all the mandatory headers
                  // (To, From, Call-ID, CSeq)  Via is already checked by stack.  
                  // See RFC 3261 Section 16.3 Step 1
                  if (!sip->exists(h_To)     ||
                      !sip->exists(h_From)   ||
                      !sip->exists(h_CallID) ||
                      !sip->exists(h_CSeq)     )
                  {
                     // skip this message and move on to the next one
                     delete sip;
                     continue;  
                  }

                  // The TU selector already checks the URI scheme for us (Sect 16.3, Step 2)

                  // check the MaxForwards isn't too low
                  if (!sip->exists(h_MaxForwards))
                  {
                     // .bwc. Add Max-Forwards header if not found.
                     sip->header(h_MaxForwards).value()=20;
                  }
                  
                  if(!sip->header(h_MaxForwards).isWellFormed())
                  {
                     //Malformed Max-Forwards! (Maybe we can be lenient and set
                     // it to 70...)
                     std::auto_ptr<SipMessage> response(Helper::makeResponse(*sip,400));
                     response->header(h_StatusLine).reason()="Malformed Max-Forwards";
                     mStack.send(*response,this);
                     delete sip;
                     continue;                     
                  }
                  
                  // .bwc. Unacceptable values for Max-Forwards
                  // !bwc! TODO make this ceiling configurable
                  if(sip->header(h_MaxForwards).value() > 255)
                  {
                     sip->header(h_MaxForwards).value()=20;                     
                  }
                  else if(sip->header(h_MaxForwards).value() <=0)
                  {
                     if (sip->header(h_RequestLine).method() != OPTIONS)
                     {
                     std::auto_ptr<SipMessage> response(Helper::makeResponse(*sip, 483));
                     mStack.send(*response, this);
                     }
                     else  // If the request is an OPTIONS, send an appropriate response
                     {
                        std::auto_ptr<SipMessage> response(Helper::makeResponse(*sip, 200));
                        mStack.send(*response, this);                        
                     }
                     // in either case get rid of the request and process the next one
                     delete sip;
                     continue;
                  }

                  // [TODO] !rwm! Need to check Proxy-Require header field values
               
                  if (sip->method() == CANCEL)
                  {
                     HashMap<Data,RequestContext*>::iterator i = mServerRequestContexts.find(sip->getTransactionId());

                     if(i == mServerRequestContexts.end())
                     {
                        SipMessage response;
                        Helper::makeResponse(response,*sip,481);
                        mStack.send(response,this);
                        delete sip;
                     }
                     else
                     {
                        try
                        {
                           i->second->process(std::auto_ptr<resip::SipMessage>(sip));
                        }
                        catch(resip::BaseException& e)
                        {
                           // .bwc. Some sort of unhandled error in process.
                           // This is very bad; we cannot form a response 
                           // at this point because we do not know
                           // whether the original request still exists.
                           ErrLog(<<"Uncaught exception in process on a CANCEL request: " << e
                                          << std::endl << "This has a high probability of leaking memory!");
                        }
                     }
                  }
                  else if (sip->method() == ACK)
                  {
                     Data tid = sip->getTransactionId();

                     // .bwc. This is going to be treated as a new transaction.
                     // The stack is maintaining no state whatsoever for this.
                     // We should treat this exactly like a new transaction.
                     if(sip->mIsBadAck200)
                     {
                        static Data ack("ack");
                        tid+=ack;
                     }
                     
                     RequestContext* context=0;

                     HashMap<Data,RequestContext*>::iterator i = mServerRequestContexts.find(tid);
                     
                     // .bwc. This might be an ACK/200, or a stray ACK/failure
                     if(i == mServerRequestContexts.end())
                     {
                        context = new RequestContext(*this, 
                                                     mRequestProcessorChain, 
                                                     mResponseProcessorChain, 
                                                     mTargetProcessorChain);
                        mServerRequestContexts[tid] = context;
                     }
                     else // .bwc. ACK/failure
                     {
                        context = i->second;
                     }

                     // The stack will send TransactionTerminated messages for
                     // client and server transaction which will clean up this
                     // RequestContext 
                     try
                     {
                        context->process(std::auto_ptr<resip::SipMessage>(sip));
                     }
                     catch(resip::BaseException& e)
                     {
                        // .bwc. Some sort of unhandled error in process.
                        ErrLog(<<"Uncaught exception in process on an ACK request: " << e
                                       << std::endl << "This has a high probability of leaking memory!");
                     }
                  }
                  else
                  {
                     // This is a new request, so create a Request Context for it
                     InfoLog (<< "New RequestContext tid=" << sip->getTransactionId() << " : " << sip->brief());
                     

                     if(mServerRequestContexts.count(sip->getTransactionId()) == 0)
                     {
                        RequestContext* context = new RequestContext(*this,
                                                                     mRequestProcessorChain, 
                                                                     mResponseProcessorChain, 
                                                                     mTargetProcessorChain);
                        InfoLog (<< "Inserting new RequestContext tid=" << sip->getTransactionId() << " -> " << *context);
                        mServerRequestContexts[sip->getTransactionId()] = context;
                        DebugLog (<< "RequestContexts: " << Inserter(mServerRequestContexts));
                        try
                        {
                           context->process(std::auto_ptr<resip::SipMessage>(sip));
                        }
                        catch(resip::BaseException& e)
                        {
                           // .bwc. Some sort of unhandled error in process.
                           // This is very bad; we cannot form a response 
                           // at this point because we do not know
                           // whether the original request still exists.
                           ErrLog(<<"Uncaught exception in process on a new request: " << e
                                          << std::endl << "This has a high probability of leaking memory!");
                        }
                     }
                     else
                     {
                        InfoLog(<<"Got a new non-ACK request "
                        "with an already existing transaction ID. This can "
                        "happen if a new request collides with a previously "
                        "received ACK/200.");
                        SipMessage response;
                        Helper::makeResponse(response,*sip,400,"Transaction-id "
                                                         "collision");
                        mStack.send(response,this);
                        delete sip;
                     }
                  }
               }
               else if (sip->isResponse())
               {
                  InfoLog (<< "Looking up RequestContext tid=" << sip->getTransactionId());
               
                  // TODO  is there a problem with a stray 200?
                  HashMap<Data,RequestContext*>::iterator i = mClientRequestContexts.find(sip->getTransactionId());
                  if (i != mClientRequestContexts.end())
                  {
                     try
                     {
                        i->second->process(std::auto_ptr<resip::SipMessage>(sip));
                     }
                     catch(resip::BaseException& e)
                     {
                        // .bwc. Some sort of unhandled error in process.
                        ErrLog(<<"Uncaught exception in process on a response: " << e);
                     }
                  }
                  else
                  {
                     // throw away stray responses
                     InfoLog (<< "Unmatched response (stray?) : " << endl << *msg);
                     delete sip;  
                  }
               }
            }
            else if (app)
            {
               DebugLog(<< "Trying to dispatch : " << *app );
               HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(app->getTransactionId());
               // the underlying RequestContext may not exist
               if (i != mServerRequestContexts.end())
               {
                  DebugLog(<< "Sending " << *app << " to " << *(i->second));
                  // This goes in as a Message and not an ApplicationMessage
                  // so that we have one peice of code doing dispatch to Monkeys
                  // (the intent is that Monkeys may eventually handle non-SIP
                  //  application messages).
                  bool eraseThisTid =  (dynamic_cast<Ack200DoneMessage*>(app)!=0);
                  try
                  {
                     i->second->process(std::auto_ptr<resip::ApplicationMessage>(app));
                  }
                  catch(resip::BaseException& e)
                  {
                     ErrLog(<<"Uncaught exception in process: " << e);
                  }
                  
                  if (eraseThisTid)
                  {
                     mServerRequestContexts.erase(i);
                  }
               }
               else
               {
                  InfoLog (<< "No matching request context...ignoring " << *app);
                  delete app;
               }
            }
            else if (term)
            {
               if (term->isClientTransaction())
               {
                  HashMap<Data,RequestContext*>::iterator i=mClientRequestContexts.find(term->getTransactionId());
                  if (i != mClientRequestContexts.end())
                  {
                     try
                     {
                        i->second->process(*term);
                     }
                     catch(resip::BaseException& e)
                     {
                        ErrLog(<<"Uncaught exception in process: " << e);
                     }
                     mClientRequestContexts.erase(i);
                  }
               }
               else 
               {
                  HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(term->getTransactionId());
                  if (i != mServerRequestContexts.end())
                  {
                     try
                     {
                        i->second->process(*term);
                     }
                     catch(resip::BaseException& e)
                     {
                        ErrLog(<<"Uncaught exception in process: " << e);
                     }
                     mServerRequestContexts.erase(i);
                  }
               }
               delete term;
            }
         }
      }
      catch (BaseException& e)
      {
         ErrLog (<< "Caught: " << e);
      }
      catch (...)
      {
         ErrLog (<< "Caught unknown exception");
      }
   }
   InfoLog (<< "Proxy::thread exit");
}

void
Proxy::send(const SipMessage& msg) 
{
   mStack.send(msg, this);
}

void
Proxy::addClientTransaction(const Data& transactionId, RequestContext* rc)
{
   if(mClientRequestContexts.count(transactionId) == 0)
   {
      InfoLog (<< "add client transaction tid=" << transactionId << " " << rc);
      mClientRequestContexts[transactionId] = rc;
   }
   else
   {
      ErrLog(<< "Received a client request context whose transaction id matches that of an existing request context. Ignoring.");
   }
}

void
Proxy::postTimerC(std::auto_ptr<TimerCMessage> tc)
{
   if(mTimerC > 0)
   {
      InfoLog(<<"Posting timer C");
      mStack.post(*tc,mTimerC,this);
   }
}


void
Proxy::postMS(std::auto_ptr<resip::ApplicationMessage> msg, int msec)
{
   mStack.postMS(*msg,msec,this);
}


const Data& 
Proxy::name() const
{
   static Data n("Proxy");
   return n;
}

bool 
Proxy::isMyUri(const Uri& uri)
{
   bool ret = isMyDomain(uri.host());
   if(ret) 
   {
      // check if we are listening on the specified port
      // .slg. this is not perfect, but it will allow us to operate in most environments
      //       where the repro proxy and a UA are running on the same machine.
      //       Note:  There is a scenario that we cannot correctly handle - when a UA and 
      //       repro are running on the same machine, and they are using the same port but on 
      //       different transports types or interfaces.  In this case we cannot tell, by looking
      //       at a requestUri or From header if the uri is ours or the UA's, and things will break.
      if(uri.port() != 0)
      {
         ret = mStack.isMyPort(uri.port());
      }
   }
   DebugLog( << "Proxy::isMyUri " << uri << " " << ret);
   return ret;
}

const resip::NameAddr& 
Proxy::getRecordRoute() const
{
   return mRecordRoute;
}

bool
Proxy::getRecordRouteEnabled() const
{
   return mRecordRouteEnabled;
}

void
Proxy::decorateMessage(resip::SipMessage &request,
                        const resip::Tuple &source,
                        const resip::Tuple &destination)
{
   DebugLog(<<"Proxy::decorateMessage called.");
   NameAddr rt;
   
   if(destination.onlyUseExistingConnection 
      || resip::InteropHelper::getRRTokenHackEnabled())
   {
      rt=getRecordRoute();
      // .bwc. If our target has an outbound flow to us, we need to put a flow
      // token in a Record-Route.
      Helper::massageRoute(request,rt);
      resip::Data binaryFlowToken;
      Tuple::writeBinaryToken(destination,binaryFlowToken);
      
      // !bwc! TODO encrypt this binary token to self.
      rt.uri().user()=binaryFlowToken.base64encode();
   }
   else if(!request.empty(h_RecordRoutes) 
            && isMyUri(request.header(h_RecordRoutes).front().uri())
            && !request.header(h_RecordRoutes).front().uri().user().empty())
   {
      // .bwc. If we Record-Routed earlier with a flow-token, we need to
      // add a second Record-Route (to make in-dialog stuff work both ways)
      rt = getRecordRoute();
      Helper::massageRoute(request,rt);
   }
   
   // This pushes the Record-Route that represents the interface from
   // which the request is being sent
   //
   // .bwc. This shouldn't duplicate the previous Record-Route, since rt only
   // gets defined if we need to double-record-route. (ie, the source or the
   // target had an outbound flow to us). The only way these could end up the
   // same is if the target and source were the same entity.
   if (!rt.uri().host().empty())
   {
      request.header(h_RecordRoutes).push_front(rt);
      InfoLog (<< "Added outbound Record-Route: " << rt);
   }
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
 */
