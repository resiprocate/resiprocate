#if defined(HAVE_CONFIG_H)
#include "config.h"
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
#include "rutil/Random.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

// Initialize statics
Data Proxy::FlowTokenSalt;

// Use Static fn with static local object to ensure KeyValueStoreKeyAllocator is created before
// static calls to allocate new keys in the monkey classes
KeyValueStore::KeyValueStoreKeyAllocator* Proxy::getGlobalKeyValueStoreKeyAllocator()
{
   static KeyValueStore::KeyValueStoreKeyAllocator* globalAllocator = new KeyValueStore::KeyValueStoreKeyAllocator();
   return globalAllocator;
}

KeyValueStore::KeyValueStoreKeyAllocator* Proxy::getRequestKeyValueStoreKeyAllocator()
{
   static KeyValueStore::KeyValueStoreKeyAllocator* requestAllocator = new KeyValueStore::KeyValueStoreKeyAllocator();
   return requestAllocator;
}

KeyValueStore::KeyValueStoreKeyAllocator* Proxy::getTargetKeyValueStoreKeyAllocator()
{
   static KeyValueStore::KeyValueStoreKeyAllocator* targetAllocator = new KeyValueStore::KeyValueStoreKeyAllocator();
   return targetAllocator;
}

KeyValueStore::Key Proxy::allocateGlobalKeyValueStoreKey()
{
   return getGlobalKeyValueStoreKeyAllocator()->allocateNewKey();
}

KeyValueStore::Key Proxy::allocateRequestKeyValueStoreKey()
{
   return getRequestKeyValueStoreKeyAllocator()->allocateNewKey();
}

KeyValueStore::Key Proxy::allocateTargetKeyValueStoreKey()
{
   return getTargetKeyValueStoreKeyAllocator()->allocateNewKey();
}

RequestContext* 
RequestContextFactory::createRequestContext(Proxy& proxy,
                                            ProcessorChain& requestP,  // monkeys
                                            ProcessorChain& responseP, // lemurs
                                            ProcessorChain& targetP)   // baboons
{
   return new RequestContext(proxy, requestP, responseP, targetP); 
}

Proxy::Proxy(SipStack& stack, 
             ProxyConfig& config,
             ProcessorChain& requestP, 
             ProcessorChain& responseP, 
             ProcessorChain& targetP) 
   : TransactionUser(TransactionUser::RegisterForTransactionTermination),
     mStack(stack), 
     mConfig(config),
     mRecordRoute(config.getConfigUri("RecordRouteUri", Uri())),
     mRecordRouteForced(config.getConfigBool("ForceRecordRouting", false)),
     mAssumePath(config.getConfigBool("AssumePath", false)),
     mPAssertedIdentityProcessing(config.getConfigBool("EnablePAssertedIdentityProcessing", false)),
     mNeverStripProxyAuthorizationHeaders(config.getConfigBool("NeverStripProxyAuthorizationHeaders", false)),
#ifdef PACKAGE_VERSION
     mServerText(config.getConfigData("ServerText", "repro " PACKAGE_VERSION)),
#else
     mServerText(config.getConfigData("ServerText", "")),
#endif
     mTimerC(config.getConfigInt("TimerC", 180)),
     mKeyValueStore(*Proxy::getGlobalKeyValueStoreKeyAllocator()),
     mRequestProcessorChain(requestP), 
     mResponseProcessorChain(responseP),
     mTargetProcessorChain(targetP),
     mUserStore(config.getDataStore()->mUserStore),
     mOptionsHandler(0),
     mRequestContextFactory(new RequestContextFactory),
     mSessionAccountingEnabled(config.getConfigBool("SessionAccountingEnabled", false)),
     mRegistrationAccountingEnabled(config.getConfigBool("RegistrationAccountingEnabled", false)),
     mAccountingCollector(0)
{
   FlowTokenSalt = Random::getCryptoRandom(20);   // 20-octet Crypto Random Key for Salting Flow Token HMACs

   mFifo.setDescription("Proxy::mFifo");

   if(InteropHelper::getOutboundSupported())
   {
      addSupportedOption("outbound");
   }

   // Create Accounting Collector if enabled
   if(mSessionAccountingEnabled || mRegistrationAccountingEnabled)
   {
      mAccountingCollector = new AccountingCollector(config);
   }
}

Proxy::~Proxy()
{
   shutdown();
   join();
   delete mAccountingCollector;
   InfoLog (<< "Proxy::thread shutdown with " << mServerRequestContexts.size() << " ServerRequestContexts and " << mClientRequestContexts.size() << " ClientRequestContexts.");
}

void 
Proxy::setOptionsHandler(OptionsHandler* handler)
{
   mOptionsHandler = handler;
}
 
void 
Proxy::setRequestContextFactory(std::auto_ptr<RequestContextFactory> requestContextFactory)
{
   mRequestContextFactory = requestContextFactory;
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
               Data tid(sip->getTransactionId());
               tid.lowercase();
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
                  if(sip->method()==OPTIONS && 
                     isMyUri(sip->header(h_RequestLine).uri()))
                  {
                     if(mOptionsHandler)
                     {
                        std::auto_ptr<SipMessage> resp(new SipMessage);
                        Helper::makeResponse(*resp,*sip,200);
                        if(mOptionsHandler->onOptionsRequest(*sip, *resp))
                        {
                           mStack.send(*resp,this);
                           delete sip;
                           continue;
                        }
                     }
                     else if(sip->header(h_RequestLine).uri().user().empty())
                     {
                        std::auto_ptr<SipMessage> resp(new SipMessage);
                        Helper::makeResponse(*resp,*sip,200);

                        if(resip::InteropHelper::getOutboundSupported())
                        {
                           resp->header(h_Supporteds).push_back(Token(Symbols::Outbound));
                        }
                        mStack.send(*resp,this);
                        delete sip;
                        continue;
                     }
                  }

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
                     sip->header(h_MaxForwards).value() = 20;                     
                  }
                  else if(sip->header(h_MaxForwards).value() <= 0)
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

                  if(!sip->empty(h_ProxyRequires))
                  {
                     std::auto_ptr<SipMessage> response(0);

                     for(Tokens::iterator i=sip->header(h_ProxyRequires).begin();
                           i!=sip->header(h_ProxyRequires).end();
                           ++i)
                     {
                        if(!i->isWellFormed() || 
                           !mSupportedOptions.count(i->value()) )
                        {
                           if(!response.get())
                           {
                              response.reset(Helper::makeResponse(*sip, 420, "Bad extension"));
                           }
                           response->header(h_Unsupporteds).push_back(*i);
                        }
                     }

                     if(response.get())
                     {
                        mStack.send(*response, this);
                        delete sip;
                        continue;
                     }
                  }
                  
                  
                  if (sip->method() == CANCEL)
                  {
                     HashMap<Data,RequestContext*>::iterator i = mServerRequestContexts.find(tid);

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
                           ErrLog(<<"Uncaught exception in process on a CANCEL "
                                    "request: " << e);
                           mStack.abandonServerTransaction(tid);
                        }
                     }
                  }
                  else if (sip->method() == ACK)
                  {
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
                        context = mRequestContextFactory->createRequestContext(*this, 
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
                        ErrLog(<<"Uncaught exception in process on an ACK "
                                 "request: " << e);
                     }
                  }
                  else
                  {
                     // This is a new request, so create a Request Context for it
                     InfoLog (<< "New RequestContext tid=" << tid << " : " << sip->brief());
                     

                     if(mServerRequestContexts.count(tid) == 0)
                     {
                        RequestContext* context = mRequestContextFactory->createRequestContext(*this,
                                                                     mRequestProcessorChain, 
                                                                     mResponseProcessorChain, 
                                                                     mTargetProcessorChain);
                        InfoLog (<< "Inserting new RequestContext tid=" << tid
                                  << " -> " << *context);
                        mServerRequestContexts[tid] = context;
                        //DebugLog (<< "RequestContexts: " << InserterP(mServerRequestContexts));  For a busy proxy - this generates a HUGE log statement!
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
                           ErrLog(<<"Uncaught exception in process on a new "
                                    "request: " << e);
                           mStack.abandonServerTransaction(tid);
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
                  InfoLog (<< "Looking up RequestContext tid=" << tid);
               
                  // TODO  is there a problem with a stray 200?
                  HashMap<Data,RequestContext*>::iterator i = mClientRequestContexts.find(tid);
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
               Data tid(app->getTransactionId());
               tid.lowercase();
               DebugLog(<< "Trying to dispatch : " << *app );
               HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(tid);
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
               Data tid(term->getTransactionId());
               tid.lowercase();
               if (term->isClientTransaction())
               {
                  HashMap<Data,RequestContext*>::iterator i=mClientRequestContexts.find(tid);
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
                  else
                  {
                     InfoLog (<< "No matching request context...ignoring " << *term);
                  }
               }
               else 
               {
                  HashMap<Data,RequestContext*>::iterator i=mServerRequestContexts.find(tid);
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
                  else
                  {
                     InfoLog (<< "No matching request context...ignoring " << *term);
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
Proxy::isMyUri(const Uri& uri) const
{
   bool ret = mStack.isMyDomain(uri.host(), uri.port());
   if(!ret)
   {
      ret = isMyDomain(uri.host());

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
   }
   DebugLog( << "Proxy::isMyUri " << uri << " " << ret);
   return ret;
}

void 
Proxy::addTransportRecordRoute(unsigned int transportKey, const resip::NameAddr& recordRoute)
{
   Lock lock(mTransportRecordRouteMutex);
   mTransportRecordRoutes[transportKey] = recordRoute;
}

void Proxy::removeTransportRecordRoute(unsigned int transportKey)
{
   Lock lock(mTransportRecordRouteMutex);
   mTransportRecordRoutes.erase(transportKey);
}

const resip::NameAddr& 
Proxy::getRecordRoute(unsigned int transportKey) const
{
   Lock lock(mTransportRecordRouteMutex);
   TransportRecordRouteMap::const_iterator it = mTransportRecordRoutes.find(transportKey);
   if(it != mTransportRecordRoutes.end())
   {
      // Transport specific record-route found
      return it->second;
   }
   return mRecordRoute;
}

bool 
Proxy::compressionEnabled() const
{
   return mStack.getCompression().getAlgorithm() != resip::Compression::NONE;
}

void 
Proxy::addSupportedOption(const resip::Data& option)
{
   mSupportedOptions.insert(option);
}

void 
Proxy::removeSupportedOption(const resip::Data& option)
{
   mSupportedOptions.erase(option);
}

void 
Proxy::doSessionAccounting(const resip::SipMessage& sip, bool received, RequestContext& context)
{
   if(mSessionAccountingEnabled)
   {
      resip_assert(mAccountingCollector);
      mAccountingCollector->doSessionAccounting(sip, received, context);
   }
}

void 
Proxy::doRegistrationAccounting(AccountingCollector::RegistrationEvent regEvent, const resip::SipMessage& sip)
{
   if(mRegistrationAccountingEnabled)
   {
      resip_assert(mAccountingCollector);
      mAccountingCollector->doRegistrationAccounting(regEvent, sip);
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
