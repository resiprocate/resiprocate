#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "repro/RequestProcessorChain.hxx"
#include "repro/Proxy.hxx"

#include "resiprocate/TransactionTerminated.hxx"
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;


Proxy::Proxy(SipStack& stack, 
             const Uri& recordRoute,
             RequestProcessorChain& requestProcessors, 
             UserStore& userStore) 
   : mStack(stack), 
     mRecordRoute(recordRoute),
     mRequestProcessorChain(requestProcessors), 
     mUserStore(userStore)
{
   if (!mRecordRoute.uri().host().empty())
   {
      mRecordRoute.uri().param(p_lr);
   }
   
   mStack.registerForTransactionTermination();
}


Proxy::~Proxy()
{
   shutdown();
   join();
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
                  // [TODO] !rwm! need to verify that the request has required headers
                  // (To, From, Call-ID, CSeq)  Via is already checked by stack.  
                  // See RFC 3261 Section 16.3 Step 1
			   
                  // The TU selector already checks the URI scheme for us (Sect 16.3, Step 2)
			   
                  if (sip->exists(h_MaxForwards) && sip->header(h_MaxForwards).value() <= 0)
                  {
                     // [TODO] !rwm! If the request is an OPTIONS, send an appropropriate response
                     std::auto_ptr<SipMessage> response(Helper::makeResponse(*sip, 483));
                     mStack.send(*response, this);
                     break;
                  }

                  // [TODO] !rwm! Need to check Proxy-Require header field values
               
                  if (sip->header(h_RequestLine).method() == CANCEL)
                  {
                     HashMap<Data,RequestContext*>::iterator i = mServerRequestContexts.find(sip->getTransactionId());

                     // [TODO] !rwm! this should not be an assert.  log and ignore instead.
                     assert (i != mServerRequestContexts.end());
                     i->second->process(std::auto_ptr<resip::Message>(msg));
                  }
                  else if (sip->header(h_RequestLine).method() == ACK)
                  {
                     // ACK needs to create its own RequestContext based on a
                     // unique transaction id. 
                     static Data ack("ack");
                     Data tid = sip->getTransactionId() + ack;
                     //assert (mServerRequestContexts.count(tid) == 0);
                     RequestContext* context=0;
                     if (mServerRequestContexts.count(tid) == 0)
                     {
                        context = new RequestContext(*this, mRequestProcessorChain);
                        mServerRequestContexts[tid] = context;
                     }
                     else
                     {
                        context = mServerRequestContexts[tid];
                     }

                     // The stack will send TransactionTerminated messages for
                     // client and server transaction which will clean up this
                     // RequestContext 
                     context->process(std::auto_ptr<resip::Message>(msg));
                  }
                  else
                  {
                     // This is a new request, so create a Request Context for it
                     InfoLog (<< "New RequestContext tid=" << sip->getTransactionId() << " : " << sip->brief());
                     
                     assert(mServerRequestContexts.count(sip->getTransactionId()) == 0);                  
                     RequestContext* context = new RequestContext(*this, mRequestProcessorChain);
                     InfoLog (<< "Inserting new RequestContext tid=" << sip->getTransactionId() << " -> " << *context);
                     mServerRequestContexts[sip->getTransactionId()] = context;
                     DebugLog (<< "RequestContexts: " << Inserter(mServerRequestContexts));
                     context->process(std::auto_ptr<resip::Message>(msg));
                  }
               }
               else if (sip->isResponse())
               {
                  InfoLog (<< "Looking up RequestContext tid=" << sip->getTransactionId());
               
                  // is there a problem with a stray 200
                  HashMap<Data,RequestContext*>::iterator i = mClientRequestContexts.find(sip->getTransactionId());
                  if (i != mClientRequestContexts.end())
                  {
                     i->second->process(std::auto_ptr<resip::Message>(msg));
                  }
                  else
                  {
                     InfoLog (<< "Unmatched response (stray?) : " << endl << *msg);
                  }
                  
                  // [TODO] !rwm! who throws stray responses away?  does the stack do this?
               }
            }
            else if (app)
            {
               HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(app->getTransactionId());
               // the underlying RequestContext may not exist
               if (i != mServerRequestContexts.end())
               {
                  i->second->process(std::auto_ptr<Message>(msg));
               }
               else
               {
                  //InfoLog (<< "No matching request context...ignoring " << *msg);
                  // [TODO] !rwm! do we need to delete the app event message?
               }
            }
            else if (term)
            {
               if (term->isClientTransaction())
               {
                  HashMap<Data,RequestContext*>::iterator i=mClientRequestContexts.find(term->getTransactionId());
                  if (i != mClientRequestContexts.end())
                  {
                     i->second->process(*term);
                     mClientRequestContexts.erase(i);
                  }
               }
               else 
               {
                  HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(term->getTransactionId());
                  if (i != mServerRequestContexts.end())
                  {
                     i->second->process(*term);
                     mServerRequestContexts.erase(i);
                  }
               }
            }
         }
      }
      catch (BaseException& e)
      {
         WarningLog (<< "Caught: " << e);
      }
      catch (...)
      {
         WarningLog (<< "Caught unknown exception");
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
   assert(mClientRequestContexts.count(transactionId) == 0);
   InfoLog (<< "add client transaction tid=" << transactionId << " " << rc);
   mClientRequestContexts[transactionId] = rc;
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
   DebugLog( << "Proxy::isMyUri " << uri << " " << ret);
   return ret;
}

const resip::NameAddr& 
Proxy::getRecordRoute() const
{
   return mRecordRoute;
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
