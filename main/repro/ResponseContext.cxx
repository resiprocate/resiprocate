#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <iostream>

#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "repro/Proxy.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Ack200DoneMessage.hxx"

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


bool
ResponseContext::addTarget(repro::Target& target, bool beginImmediately)
{
   //Disallow sip: if secure
   if(mSecure && target.uri().scheme() != Symbols::Sips)
   {
      return false;
   }
   
   
   //Make sure we don't have Targets with an invalid initial state.
   if(target.status() != Target::Pending)
   {
      return false;
   }
   
   std::list<resip::Uri>::const_iterator i;
   // make sure each target is only inserted once

   // !bwc! We can not optimize this by using stl, because operator
   // == does not conform to the partial-ordering established by operator
   // <  (We can very easily have a < b and a==b simultaneously).
   // [TODO] Once we have a canonicalized form, we can improve this.

   for(i=mTargetList.begin();i!=mTargetList.end();i++)
   {
      if(*i==target.uri())
      {
         return false;
      }
   }

   mTargetList.push_back(target.uri());
   
   
   if(beginImmediately)
   {
      beginClientTransaction(target);
      target.status()=Target::Trying;
      mActiveTransactionMap[target.tid()]=target;
   }
   else
   {
      mPendingTransactionMap[target.tid()]=target;
   }
   
   return true;
   
}


bool 
ResponseContext::beginClientTransactions()
{
   if(mPendingTransactionMap.empty())
   {
      return false;
   }
   
   for (TransactionMap::iterator i=mPendingTransactionMap.begin(); i != mPendingTransactionMap.end(); )
   {
      beginClientTransaction(i->second);
      // see rfc 3261 section 16.6
      //This code moves the Target from mPendingTransactionMap to mActiveTransactionMap,
      //and begins the transaction.
      mActiveTransactionMap[i->second.tid()] = i->second;
      InfoLog (<< "Creating new client transaction " << i->second.tid() << " -> " << i->second.uri());
      
      TransactionMap::iterator temp=i;
      i++;
      mPendingTransactionMap.erase(temp);
   }
   
   return true;

}


bool 
ResponseContext::beginClientTransaction(const resip::Data& tid)
{
   TransactionMap::iterator i = mPendingTransactionMap.find(tid);
   if(i==mPendingTransactionMap.end())
   {
      return false;
   }
   
   beginClientTransaction(i->second);
   mActiveTransactionMap[i->second.tid()] = i->second;
   InfoLog(<< "Creating new client transaction " << i->second.tid() << " -> " << i->second.uri());
   mPendingTransactionMap.erase(i);
   
   return true;
}


bool 
ResponseContext::cancelActiveClientTransactions()
{
   InfoLog (<< "Cancel all proceeding client transactions: " << (mPendingTransactionMap.size() + 
            mActiveTransactionMap.size()));

   if(mActiveTransactionMap.empty())
   {
      return false;
   }

   // CANCEL INVITE branches
   for (TransactionMap::iterator i = mActiveTransactionMap.begin(); 
        i != mActiveTransactionMap.end(); ++i)
   {
      cancelClientTransaction(i->second);
   }
      
   return true;

}

bool
ResponseContext::cancelAllClientTransactions()
{
   InfoLog (<< "Cancel ALL client transactions: " << (mPendingTransactionMap.size() + 
            mActiveTransactionMap.size()));

   if(mActiveTransactionMap.empty())
   {
      return false;
   }

   // CANCEL INVITE branches
   for (TransactionMap::iterator i = mActiveTransactionMap.begin(); 
        i != mActiveTransactionMap.end(); ++i)
   {
      cancelClientTransaction(i->second);
   }

   for (TransactionMap::iterator j = mPendingTransactionMap.begin(); 
        j != mPendingTransactionMap.end(); ++j)
   {
      cancelClientTransaction(j->second);
      mTerminatedTransactionMap[j->second.tid()] = j->second;
      TransactionMap::iterator temp = j;
      j++;
      mPendingTransactionMap.erase(temp);
   }
   
   return true;

}


bool 
ResponseContext::cancelClientTransaction(const resip::Data& tid)
{
   TransactionMap::iterator i = mActiveTransactionMap.find(tid);
   if(i!=mActiveTransactionMap.end())
   {
      cancelClientTransaction(i->second);      
      return true;
   }
   
   TransactionMap::iterator j = mPendingTransactionMap.find(tid);
   if(j != mPendingTransactionMap.end())
   {
      cancelClientTransaction(j->second);
      mTerminatedTransactionMap[tid] = j->second;
      mPendingTransactionMap.erase(j);
      return true;
   }
   
   return false;
}


Target::Status 
ResponseContext::getStatus(const resip::Data& tid)
{

   TransactionMap::iterator act = mActiveTransactionMap.find(tid);
   if(act != mActiveTransactionMap.end())
   {
      assert(!(act->second.status()==Target::Pending || act->second.status()==Target::Terminated));
      return act->second.status();
   }

   TransactionMap::iterator term = mTerminatedTransactionMap.find(tid);
   if(term != mTerminatedTransactionMap.end())
   {
      assert(term->second.status()==Target::Terminated);
      return Target::Terminated;
   }

   TransactionMap::iterator pend = mPendingTransactionMap.find(tid);
   if(pend != mPendingTransactionMap.end())
   {
      assert(pend->second.status()==Target::Pending);
      return Target::Pending;
   }
   

   return Target::NonExistent;
}


const ResponseContext::TransactionMap& 
ResponseContext::getPendingTransactionMap() const
{
   return mPendingTransactionMap;
}



bool 
ResponseContext::hasPendingTransactions()
{
   return !mPendingTransactionMap.empty();
}


bool 
ResponseContext::hasActiveTransactions()
{
   return !mActiveTransactionMap.empty();
}


bool 
ResponseContext::hasTerminatedTransactions()
{
   return !mTerminatedTransactionMap.empty();
}


bool 
ResponseContext::areAllTransactionsTerminated()
{
   return (mPendingTransactionMap.empty() && mActiveTransactionMap.empty());
}


const std::list<resip::Uri>&
ResponseContext::getTargetList() const
{
   return mTargetList;
}

void 
ResponseContext::removeClientTransaction(const resip::Data& transactionId)
{
   //This tid will most likely be found in the map of terminated transactions,
   //under normal circumstances.
   if(mTerminatedTransactionMap.erase(transactionId))
   {
      return;
   }

   if(mPendingTransactionMap.erase(transactionId))
   {
      return;
   }
   
   if(mActiveTransactionMap.erase(transactionId))
   {
      WarningLog(<< "Something removed an active transaction, " << transactionId
               << ". It is very likely that something is broken here. ");
      return;
   }
      
}
 

void
ResponseContext::beginClientTransaction(repro::Target& target)
{

      SipMessage request(mRequestContext.getOriginalRequest());

      
      request.header(h_RequestLine).uri() = target.uri(); 

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
      
      static ExtensionParameter p_cid("cid");
      static ExtensionParameter p_cid1("cid1");
      static ExtensionParameter p_cid2("cid2");
      
      // Record-Route addition only for dialogs
      if ( !request.header(h_To).exists(p_tag) &&  // only for dialog-creating request
           (request.header(h_RequestLine).method() == INVITE ||
            request.header(h_RequestLine).method() == SUBSCRIBE ) )
      {
         
         NameAddr rt(mRequestContext.mProxy.getRecordRoute());
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
            
            if (mRequestContext.getOriginalRequest().getSource().connectionId != 0)
            {
               rt.uri().param(p_cid1) = Data(mRequestContext.getOriginalRequest().getSource().connectionId);
            }
            
            if (request.header(h_RequestLine).uri().exists(p_cid))
            {
               rt.uri().param(p_cid2) = request.header(h_RequestLine).uri().param(p_cid);
            }
            InfoLog (<< "Added Record-Route: " << rt);
         }

         // !jf! By not specifying host in Record-Route, the TransportSelector
         //will fill it in. 
         if (!mRequestContext.mProxy.getRecordRoute().uri().host().empty())
         {
            request.header(h_RecordRoutes).push_front(rt);
         }
      }
      
      // !jf! unleash the baboons here
      // a baboon might adorn the message, record call logs or CDRs, might
      // insert loose routes on the way to the next hop
      
      Helper::processStrictRoute(request);
      
      //This is where the request acquires the tid of the Target. The tids 
      //should be the same from here on out.
      request.header(h_Vias).push_front(target.via());

      if (mRequestContext.mTargetConnectionId != 0)
      {
         request.header(h_RequestLine).uri().param(p_cid) = Data(mRequestContext.mTargetConnectionId);
         InfoLog (<< "Use an existing connection id: " << request.header(h_RequestLine).uri());
      }

      
      if(!mRequestContext.mInitialTimerCSet)
      {
         mRequestContext.mInitialTimerCSet=true;
         mRequestContext.updateTimerC();
      }
      
      // the rest of 16.6 is implemented by the transaction layer of resip
      // - determining the next hop (tuple)
      // - adding a content-length if needed
      // - sending the request
      sendRequest(request); 

      target.status() = Target::Trying;

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

   if (request.header(h_RequestLine).method() == ACK)
   {
     DebugLog(<<"Posting Ack200DoneMessage");
     static Data ack("ack");
     mRequestContext.getProxy().post(new Ack200DoneMessage(mRequestContext.getTransactionId()+ack));
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
      cancelAllClientTransactions();
   }
}

void
ResponseContext::processTimerC()
{
   if (!mForwardedFinalResponse)
   {
      InfoLog(<<"Canceling client transactions due to timer C.");
      cancelAllClientTransactions();
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
         mRequestContext.updateTimerC();

         if  (code > 100 && !mForwardedFinalResponse)
         {
            mRequestContext.sendResponse(response);
         }
         
         {
            InfoLog (<< "Search for " << transactionId << " in " << Inserter(mActiveTransactionMap));
            
            TransactionMap::iterator i = mActiveTransactionMap.find(transactionId);
            if (i == mActiveTransactionMap.end())
            {
               return;//Response isn't for an active transaction, drop on floor
            }
            if (i->second.status() == Target::WaitingToCancel)
            {
               DebugLog(<< "Canceling a transaction with uri: " 
                        << resip::Data::from(i->second.uri()) << " , to host: " 
                        << i->second.via().sentHost());
               cancelClientTransaction(i->second);
               i->second.status() = Target::Terminated;
            }
            else if(i->second.status() != Target::Terminated)
            {
               i->second.status() =  Target::Proceeding ;
            }
         }
         break;
         
      case 2:
         terminateClientTransaction(transactionId);
         if (response.header(h_CSeq).method() == INVITE)
         {
            cancelAllClientTransactions();
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
                   << " outstanding client transactions: " << Inserter(mActiveTransactionMap));
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
               mBestPriority=0; //6xx class responses take precedence over all 3xx,4xx, and 5xx
               if (response.header(h_CSeq).method() == INVITE)
               {
                  // CANCEL INVITE branches
                  cancelAllClientTransactions();
               }
            }
            
            if (areAllTransactionsTerminated())
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
ResponseContext::cancelClientTransaction(repro::Target& target)
{
   if (target.status() == Target::Proceeding)
   {
      InfoLog (<< "Cancel client transaction: " << target);
      
      SipMessage request(mRequestContext.getOriginalRequest());
      request.header(h_Vias).push_front(target.via());
      request.header(h_RequestLine).uri() = target.uri();
      
      std::auto_ptr<SipMessage> cancel(Helper::makeCancel(request));
      sendRequest(*cancel);

      DebugLog(<< "Canceling a transaction with uri: " 
               << resip::Data::from(target.uri()) << " , to host: " 
               << target.via().sentHost());
      target.status() = Target::Terminated;
   }
   else if (target.status() == Target::Trying)
   {
      target.status() = Target::WaitingToCancel;
      DebugLog(<< "Setting transaction status to "
               << "WaitingToCancel with uri: " 
               << resip::Data::from(target.uri()) << " , to host: " 
               << target.via().sentHost());

   }
   else if (target.status() == Target::Pending)
   {
      target.status() = Target::Terminated;
   }

}

void 
ResponseContext::terminateClientTransaction(const resip::Data& tid)
{

   InfoLog (<< "Terminating client transaction: " << tid << " all = " << areAllTransactionsTerminated());

   TransactionMap::iterator i = mActiveTransactionMap.find(tid);
   if(i != mActiveTransactionMap.end())
   {
      InfoLog (<< "client transactions: " << Inserter(mActiveTransactionMap));
      i->second.status() = Target::Terminated;
      mTerminatedTransactionMap[tid] = i->second;
      mActiveTransactionMap.erase(i);
      return;
   }
   
   TransactionMap::iterator j = mPendingTransactionMap.find(tid);
   if(j != mPendingTransactionMap.end())
   {
      InfoLog (<< "client transactions: " << Inserter(mPendingTransactionMap));
      j->second.status() = Target::Terminated;
      mTerminatedTransactionMap[tid] = j->second;
      mPendingTransactionMap.erase(j);
      return;   
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
repro::operator<<(std::ostream& strm, const repro::Target& t)
{
   strm << "Target: " << t.uri() << " " <<" status=" << t.status();
   return strm;
}



std::ostream&
repro::operator<<(std::ostream& strm, const ResponseContext& rc)
{
   strm << "ResponseContext: "
        << " identity=" << rc.mRequestContext.getDigestIdentity()
        << " best=" << rc.mBestPriority << " " << rc.mBestResponse.brief()
        << " forwarded=" << rc.mForwardedFinalResponse
        << " pending=" << Inserter(rc.mPendingTransactionMap)
        << " active=" << Inserter(rc.mActiveTransactionMap)
        << " terminated=" << Inserter(rc.mTerminatedTransactionMap);
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
