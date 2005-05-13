#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <iostream>

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "repro/Proxy.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/RequestContext.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

ResponseContext::ResponseContext(RequestContext& context) : 
   mRequestContext(context),
   mForwardedFinalResponse(false),
   mBestPriority(50),
   mSecure(false) //context.getOriginalRequest().header(h_RequestLine).uri().scheme() == Symbols::Sips)
{
}

void 
ResponseContext::sendRequest(const resip::SipMessage& request)
{
   assert (request.isRequest());
   mRequestContext.mProxy.send(request);
   if (request.header(h_RequestLine).method() != CANCEL && 
       request.header(h_RequestLine).method() != ACK)
   {
      mRequestContext.getProxy().addClientTransaction(request.getTransactionId(), &mRequestContext);
      mRequestContext.mTransactionCount++;
   }
}

void
ResponseContext::processCandidates()
{
   bool added=false;
   while (!mRequestContext.getCandidates().empty())
   {
      // purpose of this code is to copy the Uri from the candidate and to only
      // take the q value parameter from the candidate. 
      
      NameAddr& candidate = mRequestContext.getCandidates().back();
      InfoLog (<< "Considering " << candidate);
      Uri target(candidate.uri());
      // make sure each target is only inserted once
      if (mSecure && target.scheme() == Symbols::Sips || !mSecure)
      {
         if (mTargetSet.insert(target).second)
         {
            added = true;
            NameAddr pending(target);
            pending.param(p_q) = candidate.param(p_q);
         
            mPendingTargetSet.insert(pending);
            InfoLog (<< "Added " << pending);
         }
      }
      mRequestContext.getCandidates().pop_back();
   }

   if (added)
   {
      //InfoLog (<< *this);
      processPendingTargets();
   }
}

void
ResponseContext::processPendingTargets()
{
   for (PendingTargetSet::iterator i=mPendingTargetSet.begin(); i != mPendingTargetSet.end(); ++i)
   {
      // see rfc 3261 section 16.6
      SipMessage request(mRequestContext.getOriginalRequest());

      Branch branch;
      branch.status = Trying;
      branch.uri = i->uri();
      
      request.header(h_RequestLine).uri() = branch.uri; 

      if (request.exists(h_MaxForwards))
      {
         if (request.header(h_MaxForwards).value() <= 20)
         {
            request.header(h_MaxForwards).value()--;
         }
         else
         {
            request.header(h_MaxForwards).value() = 20; // !jf! use Proxy to retrieve this
         }
      }
      else
      {
         request.header(h_MaxForwards).value() = 20; // !jf! use Proxy to retrieve this
      }
      
      // Record-Route addition only for dialogs
      if (request.header(h_RequestLine).method() == INVITE ||
          request.header(h_RequestLine).method() == SUBSCRIBE)
      {
         NameAddr rt;
         // !jf! could put unique id for this instance of the proxy in user portion

         if (request.exists(h_Routes) && request.header(h_Routes).size() != 0)
         {
            rt.uri().scheme() == request.header(h_Routes).front().uri().scheme();
         }
         else
         {
            rt.uri().scheme() = request.header(h_RequestLine).uri().scheme();
         }
         
         const Data& sentTransport = request.header(h_Vias).front().transport();
         if (sentTransport != Symbols::UDP)
         {
            rt.uri().param(p_transport) = sentTransport;
         }

         // !jf! This should be the domain of "this" proxy instead of local
         // hostname. 
         rt.uri().host() = DnsUtil::getLocalHostName();
         rt.uri().param(p_lr);
         request.header(h_RecordRoutes).push_front(rt);
      }
      
      // !jf! unleash the baboons here
      // a baboon might adorn the message, record call logs or CDRs, might
      // insert loose routes on the way to the next hop
      
      Helper::processStrictRoute(request);
      request.header(h_Vias).push_front(branch.via);
      
      // add a Timer C if one hasn't already been added
      
      // the rest of 16.6 is implemented by the transaction layer of resip
      // - determining the next hop (tuple)
      // - adding a content-length if needed
      // - sending the request
      mClientTransactions[request.getTransactionId()] = branch;
      InfoLog (<< "Creating new client transaction " << request.getTransactionId() << " -> " << branch.uri);
      sendRequest(request); 
   }
}

void
ResponseContext::processCancel(const SipMessage& request)
{
   assert(request.isRequest());
   assert(request.header(h_RequestLine).method() == CANCEL);

   std::auto_ptr<SipMessage> ok(Helper::makeResponse(request, 200));   
   mRequestContext.sendResponse(*ok);

   if (!mForwardedFinalResponse)
   {
      cancelProceedingClientTransactions();
   }
}

void
ResponseContext::processResponse(SipMessage& response)
{
   InfoLog (<< "processResponse: " << endl << response);

   // store this before we pop the via and lose the branch tag
   const Data transactionId = response.getTransactionId();
   
   // for provisional responses, 
   assert(response.isResponse());
   assert (response.exists(h_Vias) && !response.header(h_Vias).empty());
   response.header(h_Vias).pop_front();

   const Via& via = response.header(h_Vias).front();
   if (!via.exists(p_branch) || !via.param(p_branch).hasMagicCookie())
   {
      response.setRFC2543TransactionId(mRequestContext.mOriginalRequest->getTransactionId());
   }

   if (response.header(h_Vias).empty())
   {
      // ignore CANCEL/200
      return;
   }
   
   int code = response.header(h_StatusLine).statusCode();

   switch (code / 100)
   {
      case 1:
         // update Timer C
         if  (code > 100 && !mForwardedFinalResponse)
         {
            mRequestContext.sendResponse(response);
         }
         
         {
            InfoLog (<< "Search for " << transactionId << " in " << Inserter(mClientTransactions));
            
            TransactionMap::iterator i = mClientTransactions.find(transactionId);
            assert (i != mClientTransactions.end());
            if (i->second.status == WaitingToCancel)
            {
               cancelClientTransaction(i->second);
               mClientTransactions.erase(i);
            }
            else
            {
               i->second.status = Proceeding;
            }
         }
         break;
         
      case 2:
         terminateClientTransaction(transactionId);
         if (response.header(h_CSeq).method() == INVITE)
         {
            cancelProceedingClientTransactions();
            mForwardedFinalResponse = true;
            mRequestContext.sendResponse(response);
         }
         else if (!mForwardedFinalResponse)
         {
            mForwardedFinalResponse = true;
            mRequestContext.sendResponse(response);            
         }
         break;
         
      case 3:
      case 4:
      case 5:
         DebugLog (<< "forwardedFinal=" << mForwardedFinalResponse 
                   << " outstanding client transactions: " << Inserter(mClientTransactions));
         terminateClientTransaction(transactionId);
         if (!mForwardedFinalResponse)
         {
            int priority = getPriority(response);
            if (priority == mBestPriority)
            {
               if (code == 401 || code == 407)
               {
                  if (response.exists(h_WWWAuthenticates))
                  {
                     for ( Auths::iterator i=response.header(h_WWWAuthenticates).begin(); 
                           i != response.header(h_WWWAuthenticates).end() ; ++i)
                     {                     
                        mBestResponse.header(h_WWWAuthenticates).push_back(*i);
                     }
                  }
                  
                  if (response.exists(h_ProxyAuthenticates))
                  {
                     for ( Auths::iterator i=response.header(h_ProxyAuthenticates).begin(); 
                           i != response.header(h_ProxyAuthenticates).end() ; ++i)
                     {                     
                        mBestResponse.header(h_ProxyAuthenticates).push_back(*i);
                     }
                     mBestResponse.header(h_StatusLine).statusCode() = 407;
                  }
               }
               else if (code / 100 == 3) // merge 3xx
               {
                  for (NameAddrs::iterator i=response.header(h_Contacts).begin(); 
                       i != response.header(h_Contacts).end(); ++i)
                  {
                     if (!i->isAllContacts())
                     {
                        mBestResponse.header(h_Contacts).push_back(*i);
                     }
                  }
                  mBestResponse.header(h_StatusLine).statusCode() = 300;
               }
            }
            else if (priority < mBestPriority)
            {
               mBestPriority = priority;
               mBestResponse = response;
            }
            
            if (areAllTransactionsTerminated())
            {
               InfoLog (<< "Forwarding best response: " << response.brief());
               
               mForwardedFinalResponse = true;
               // don't forward 408 to NIT
               if (mBestResponse.header(h_StatusLine).statusCode() != 408 ||
                   response.header(h_CSeq).method() == INVITE)
               {
                  mRequestContext.sendResponse(mBestResponse);
               }
            }
         }
         break;
         
      case 6:
         terminateClientTransaction(transactionId);
         if (!mForwardedFinalResponse)
         {
            if (mBestResponse.header(h_StatusLine).statusCode() / 100 != 6)
            {
               mBestResponse = response;
               if (response.header(h_CSeq).method() == INVITE)
               {
                  // CANCEL INVITE branches
                  cancelProceedingClientTransactions();
               }
            }
            
            if (mClientTransactions.empty())
            {
               mForwardedFinalResponse = true;
               mRequestContext.sendResponse(mBestResponse);
            }
         }
         break;
         
      default:
         assert(0);
         break;
   }
}

void
ResponseContext::cancelClientTransaction(const Branch& branch)
{
   InfoLog (<< "Cancel client transactions: " << branch);
   
   SipMessage request(mRequestContext.getOriginalRequest());
   request.header(h_Vias).push_front(branch.via);
   
   std::auto_ptr<SipMessage> cancel(Helper::makeCancel(request));
   sendRequest(*cancel);
}

void
ResponseContext::cancelProceedingClientTransactions()
{
   InfoLog (<< "Cancel all proceeding client transactions: " << mClientTransactions.size());

   // CANCEL INVITE branches
   for (TransactionMap::iterator i = mClientTransactions.begin(); 
        i != mClientTransactions.end(); ++i)
   {
      if (i->second.status == Proceeding)
      {
         cancelClientTransaction(i->second);
         i->second.status = Terminated;
      }
      else if (i->second.status == Trying)
      {
         i->second.status = WaitingToCancel;
      }
   }
}

void
ResponseContext::terminateClientTransaction(const Data& transactionId)
{
   for (TransactionMap::iterator i = mClientTransactions.begin(); i != mClientTransactions.end(); i++)
   {
      if (i->first == transactionId) 
      {
         i->second.status = Terminated;
      }
   }
   InfoLog (<< "Terminating client transaction: " << transactionId << " all = " << areAllTransactionsTerminated());
   InfoLog (<< "client transactions: " << Inserter(mClientTransactions));
}

bool
ResponseContext::areAllTransactionsTerminated()
{
   for (TransactionMap::iterator i = mClientTransactions.begin(); i != mClientTransactions.end(); i++)
   {
      if (i->second.status != Terminated) return false;
   }
   return true;
}

bool
ResponseContext::removeClientTransaction(const Data& transactionId)
{
   TransactionMap::iterator i = mClientTransactions.find(transactionId);
   if (i != mClientTransactions.end())
   {
      mClientTransactions.erase(i);
      return true;
   }
   else
   {
      return false;
   }
}

int
ResponseContext::getPriority(const resip::SipMessage& msg)
{
   int responseCode = msg.header(h_StatusLine).statusCode();
   int p = 0;  // "p" is the relative priority of the response

	assert(responseCode >= 300 && responseCode <= 599);
	if (responseCode <= 399)  // 3xx response
	{ 
		return 5;  // response priority is 5
	}
	if (responseCode >= 500)
	{
		switch(responseCode)
		{
			case 501:	// these three have different priorities
			case 503:   // which are addressed in the case statement
			case 580:	// below (with the 4xx responses)
				break;
			default:
				return 42; // response priority of other 5xx is 42
		}
	}

	switch(responseCode)
	{
		// Easy to Repair Responses: 412, 484, 422, 423, 407, 401, 300..399, 402
		case 412:		// Publish ETag was stale
           return 1;
		case 484:		// overlap dialing
           return 2;
		case 422:		// Session-Timer duration too long
		case 423:		// Expires too short
           return 3;
		case 407:		// Proxy-Auth
		case 401:		// UA Digest challenge
           return 4;
			
		// 3xx responses have p = 5
		case 402:		// Payment required
           return 6;

		// Responses used for negotiation: 493, 429, 420, 406, 415, 488
		case 493:		// Undecipherable, try again unencrypted 
           return 10;

		case 420:		// Required Extension not supported, try again without
           return 12;

		case 406:		// Not Acceptable
		case 415:		// Unsupported Media Type
		case 488:		// Not Acceptable Here
           return 13;
			
		// Possibly useful for negotiation, but less likely: 421, 416, 417, 494, 580, 485, 405, 501, 413, 414
		
		case 416:		// Unsupported scheme
		case 417:		// Unknown Resource-Priority
           return 20;

		case 405:		// Method not allowed (both used for negotiating REFER, PUBLISH, etc..
		case 501:		// Usually used when the method is not OK
           return 21;

		case 580:		// Preconditions failure
           return 22;

		case 485:		// Ambiguous userpart.  A bit better than 404?
           return 23;

		case 428:		// Use Identity header
		case 429:		// Provide Referrer Identity 
		case 494:		// Use the sec-agree mechanism
           return 24;

		case 413:		// Request too big
		case 414:		// URI too big
           return 25;

		case 421:		// An extension required by the server was not in the Supported header
           return 26;
		
		// The request isn't repairable, but at least we can try to provide some 
		// useful information: 486, 480, 410, 404, 403, 487
		
		case 486:		// Busy Here
           return 30;

		case 480:		// Temporarily unavailable
           return 31;

		case 410:		// Gone
           return 32;

		case 436:		// Bad Identity-Info 
		case 437:		// Unsupported Certificate
           return 33;

		case 403:		// Forbidden
           return 34;

		case 404:		// Not Found
           return 35;

		case 487:		// Some branches were cancelled, if the UAC sent a CANCEL this is good news
           return 36;

		// We are hosed: 503, 483, 482, 481, other 5xx, 400, 491, 408  // totally useless

		case 503:	// bad news, but maybe we got a 
           return 40;

		case 483:	// loops, encountered
		case 482:
           return 41;
			
		// other 5xx   p = 42

		// UAS is seriously confused: p = 43
		// case 481:	
		// case 400:
		// case 491:
		// default:
		
		case 408:	// very, very bad  (even worse than the remaining 4xx responses)
           return 49;
		
		default:
           return 43;
	}
    return p;
}

bool 
ResponseContext::CompareQ::operator()(const resip::NameAddr& lhs, const resip::NameAddr& rhs) const
{
   return lhs.param(p_q) < rhs.param(p_q);
}

bool 
ResponseContext::CompareStatus::operator()(const resip::SipMessage& lhs, const resip::SipMessage& rhs) const
{
   assert(lhs.isResponse());
   assert(rhs.isResponse());
   
   
   // !rwm! replace with correct thingy here
   return lhs.header(h_StatusLine).statusCode() < rhs.header(h_StatusLine).statusCode();
}


std::ostream& 
repro::operator<<(std::ostream& strm, const ResponseContext::Branch& b)
{
   strm << "Branch: " << b.uri << " " <<" status=" << b.status;
   return strm;
}

std::ostream&
repro::operator<<(std::ostream& strm, const ResponseContext& rc)
{
   strm << "ResponseContext: "
        << " identity=" << rc.mRequestContext.getDigestIdentity()
        << " best=" << rc.mBestPriority << " " << rc.mBestResponse.brief()
        << " forwarded=" << rc.mForwardedFinalResponse
        << " pending=" << Inserter(rc.mPendingTargetSet);
      //<< " targets=" << Inserter(rc.mTargetSet)
      //<< " clients=" << Inserter(rc.mClientTransactions);

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
