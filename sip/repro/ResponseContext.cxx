#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/SipMessage.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/RequestContext.hxx"

using namespace resip;
using namespace repro;
using namespace std;

ResponseContext::ResponseContext(RequestContext& context) : 
   mRequestContext(context)
{
}

void
ResponseContext::processCandidates()
{
   while (!mRequestContext.getCandidates().empty())
   {
      // purpose of this code is to copy the Uri from the candidate and to only
      // take the q value parameter from the candidate. 
      
      NameAddr& candidate = mRequestContext.getCandidates().back();
      NameAddr uri(candidate.uri());
      uri.param(p_q) = candidate.param(p_q);
      mPendingTargetSet.insert(uri);
      mRequestContext.getCandidates().pop_back();
   }

   processPendingTargets();
}

void
ResponseContext::processPendingTargets()
{
   
}

void
ResponseContext::processResponse(const SipMessage& response)
{
   // for provisional responses, 
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
