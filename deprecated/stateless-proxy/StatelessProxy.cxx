#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Tuple.hxx"
#include "StatelessProxy.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

StatelessProxy::StatelessProxy(const char* proxyHost, int proxyPort, 
                               const char* targetHost, int targetPort, const char* proto) 
   : SipStack(false, 0, true),
     mLoose(false),
     mUseTarget(targetHost != 0)
{
   assert(proto == 0 || proto == Symbols::UDP || proto == Symbols::TCP);
   
   // used for record-route
   mProxyUrl.host() = proxyHost;
   mProxyUrl.port() = proxyPort;

   // where to proxy the requests
   if (mUseTarget)
   {
      mTarget.host() = targetHost;
      mTarget.port() = targetPort;
      mTarget.param(p_protocol) = proto;
   }

   // transports to use
   addTransport(UDP, proxyPort, V4);
   addTransport(TCP, proxyPort, V4);
}

bool
StatelessProxy::onForwardRequest(SipMessage* request)
{
   assert(request->isRequest());
   return true;
}

bool
StatelessProxy::onForwardResponse(SipMessage* request)
{
   assert(request->isResponse());
   return true;
}

void
StatelessProxy::thread()
{
   while ( !isShutdown() )
   {
      FdSet fdset;
      buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(getTimeTillNextProcessMS());
      assert (err != -1);
      process(fdset);

      SipMessage* received = receive();
      if (received)
      {
         InfoLog (<< "stack1 got: " << received->brief() 
                  << " from user: " << received->header(h_From).uri().user() );
         

         if (requestValidation(received)) // 16.3
         {
            preprocess(received); // 16.4 
            forwardRequest(received); // 16.6
         }
         handleResponse(received);
         delete received;
      }
   }
}

bool
StatelessProxy::requestValidation(SipMessage* request)
{
   if (!request->isRequest())
   {
      return true;
   }
   
   if (!request->exists(h_To) || 
       !request->exists(h_From) || 
       !request->exists(h_CSeq) || 
       !request->exists(h_CallId) || 
       !request->exists(h_Vias))
   {
      InfoLog(<< "Missing mandatory header fields (To, From, CSeq, Call-Id or Via)");
      sendResponse( request, 400, "Missing mandatory fields");
      return false;
   }
   else if (request->header(h_CallId).value().size() > 250)
   {
      InfoLog(<< "Call-Id field was too long");
      sendResponse( request, 400, "Call-Id header was too long");
      return false;
   }
   
   // !jf! check for scheme support here - just pass everything along since we are dumb
   
   // !jf! what will this return if there is no Max-Forwards field
   // should pass in this case. 
   if (request->exists(h_MaxForwards) && request->header(h_MaxForwards).value() == 0)
   {
      InfoLog(<< "Sending 483 (Too many hops)");
      sendResponse( request, 483 );
      return false;
   }
      
   // !jf! loop detection here

   // !jf! extension support here

   return true;
}

void
StatelessProxy::preprocess(SipMessage* msg)
{
   // See section 16.4 Route Information Pre-processing bis09
   // !jf! for now, assume it is a strict router
   if (mLoose && 
       msg->exists(resip::h_Routes) &&
       !msg->header(resip::h_Routes).empty())
   {
      Uri& uri = msg->header(h_RequestLine).uri();
      if (isMyDomain(uri.host(), uri.port()))
      {
         DebugLog (<< "Message came from a strict router " << msg->brief());
         msg->header(h_RequestLine).uri() = msg->header(h_Routes).back().uri();
         msg->header(h_Routes).pop_back();
      }
   }
   
   // maddr processing here !jf!
   
   if (msg->exists(h_Routes) && !msg->header(h_Routes).empty())
   {
      Uri& uri = msg->header(h_Routes).front().uri();
      if (isMyDomain(uri.host(), uri.port()))
      {
         DebugLog (<< "Removing self " << uri.host() << ":" << uri.port() << " from route " << msg->brief());
         msg->header(h_Routes).pop_front();
      }
      else
      {
         DebugLog (<< "Doing nothing '" << uri.host() << "' : '" << uri.port() << "'");
      }
   }
}

void
StatelessProxy::forwardRequest(SipMessage* request)
{
   if (!request->isRequest())
   {
      return;
   }
   
   // 16.6 section 2
   // muck with request-uri here if you want to
   //request->header(h_RequestLine).uri() = ???;

   // 16.6 section 3 
   if (request->exists(h_MaxForwards))
   {
      request->header(h_MaxForwards).value()--;
   }
   else
   {
      request->header(h_MaxForwards).value() = 70;
   }
   

   // 16.6 section 4
   NameAddr proxy;
   proxy.uri() = mProxyUrl;
   request->header(h_RecordRoutes).push_front(proxy);
   
   // may want to indicate that our proxy has been visited.  !jf!
   // 16.6 section 5  - custom headers
   
   // 16.6 section 6
   // handle strict routers here, determine where to send it. 
   if (mLoose)
   {
      if (request->exists(h_Routes) && !request->header(h_Routes).empty())
      {
         if (!request->header(h_Routes).front().uri().exists(p_lr)) 
         {
            DebugLog (<< "request came from a strict router: postprocess it " << request->brief());
            resip::Helper::processStrictRoute(*request);
         }
      }
   }


   // see section 16.11
   Via via;
   via.param(p_branch).reset(request->getTransactionId().md5());
   //via.param(p_stid) = request->getTransactionId();
   request->header(h_Vias).push_front(via);
   
   DebugLog (<< "proxy request : " 
             << via.param(p_branch).getTransactionId()
             << " : " << request->brief());
   if (onForwardRequest(request))
   {
      if (mUseTarget)
      {
         sendTo(*request, mTarget);
      }
      else
      {
         send(*request);
      }
   }
}
   
void 
StatelessProxy::handleResponse(SipMessage* response)
{
   if (response->isResponse())
   {
      if (!response->header(h_Vias).empty())
      {
         if (onForwardResponse(response)) 
         {
            if (response->header(h_Vias).front().sentHost() == mProxyUrl.host())
            {
               response->header(h_Vias).pop_front();
               assert(!response->header(h_Vias).empty());
               
               static Data unknown("UNKNOWN");
               response->setRFC2543TransactionId(unknown);
            
               DebugLog (<< "Proxying response: " << response->brief());
               send(*response);
            }
         }
      }
   }
}

void
StatelessProxy::sendResponse(SipMessage* request, int code, const Data& reason) 
{
   assert(request->isRequest());
   SipMessage* response = resip::Helper::makeResponse(*request, code, reason);
   // !jf! may need to copy tid here
   send(*response);
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
 * This software consists of voluntary contributions made by PurpleComm,
 * Inc. and many individuals on behalf of PurpleComm, Inc. Inc.
 *
 */
