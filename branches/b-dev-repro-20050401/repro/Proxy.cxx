#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/TransactionTerminated.hxx"
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "repro/RequestProcessorChain.hxx"
#include "repro/Proxy.hxx"

using namespace resip;
using namespace repro;
using namespace std;

Proxy::Proxy(SipStack& stack, RequestProcessorChain& requestProcessors,
             UserDb &userDb) 
   : mStack(stack), mRequestProcessorChain(requestProcessors), mUserDb(userDb)
{
}

Proxy::~Proxy()
{
   shutdown();
   join();
}

bool 
Proxy::isForMe(const SipMessage& msg) const
{
   return true;
}

bool
Proxy::isShutDown() const
{
  return false;
}

UserDb &
Proxy::getUserDb()
{
   return mUserDb;
}

void
Proxy::thread()
{
   while (!isShutdown())
   {
      Message* msg;
      if ((msg = mFifo.getNext(100)) != 0)
      {
         
         SipMessage* sip = dynamic_cast<SipMessage*>(msg);
         ApplicationMessage* app = dynamic_cast<ApplicationMessage*>(msg);
         TransactionTerminated* term = dynamic_cast<TransactionTerminated*>(msg);

         if (sip)
         {
            if (sip->isRequest())
            {
               // !jf! handle ACK specially (to 200) here
               if (sip->header(h_RequestLine).method() == CANCEL)
               {
                  HashMap<Data,RequestContext*>::iterator i = mRequestContexts.find(sip->getTransactionId());
                  assert (i != mRequestContexts.end());
                  i->second->process(std::auto_ptr<resip::Message>(msg));
               }
               else
               {
                  assert(mRequestContexts.count(sip->getTransactionId()) == 0);                  
                  RequestContext* context = new RequestContext(*this, std::auto_ptr<SipMessage>(sip), mRequestProcessorChain);
                  mRequestContexts[sip->getTransactionId()] = context;
                  context->process(std::auto_ptr<resip::Message>(msg));
               }
            }
            else if (sip->isResponse())
            {
               // is there a problem with a stray 200
               HashMap<Data,RequestContext*>::iterator i = mRequestContexts.find(sip->getTransactionId());
               assert (i != mRequestContexts.end());
               i->second->process(std::auto_ptr<resip::Message>(msg));
            }
         }
         else if (app)
         {
            HashMap<Data,RequestContext*>::iterator i=mRequestContexts.find(term->getTransactionId());
            // the underlying RequestContext may not exist
            if (i != mRequestContexts.end())
            {
               i->second->process(std::auto_ptr<Message>(msg));
            }
            else
            {
               //InfoLog (<< "No matching request context...ignoring " << *msg);
            }
         }
         else if (term)
         {
            HashMap<Data,RequestContext*>::iterator i=mRequestContexts.find(term->getTransactionId());
            if (i != mRequestContexts.end())
            {
               i->second->process(*term);
               mRequestContexts.erase(i);
            }
         }
      }
   }
}

bool
Proxy::isMyDomain(resip::Uri& uri) const
{
   return mStack.isMyDomain(uri.host(),uri.port());
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
