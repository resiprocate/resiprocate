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
