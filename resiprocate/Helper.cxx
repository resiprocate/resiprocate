#include <sipstack/Helper.hxx>
#include <sipstack/Uri.hxx>
#include <util/RandomHex.hxx>

using namespace Vocal2;

const int Helper::tagSize = 4;

SipMessage 
Helper::makeRequest(const NameAddr& target, 
                    const NameAddr& from,
                    const NameAddr& contact,
                    MethodTypes method)
{
   SipMessage request;
   RequestLine rLine(method);
   rLine.uri() = target.uri();
   request.header(h_To) = target;
   request.header(h_RequestLine) = rLine;
   request.header(h_MaxForwards).value() = 70;
   request.header(h_CSeq).method() = method;
   request.header(h_CSeq).sequence() = 1;
   request.header(h_From) = from;
   request.header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request.header(h_Contacts).push_front(contact);
   request.header(h_CallId).value() = Helper::computeCallId();
   return request;
}

SipMessage 
Helper::makeInvite(const NameAddr& target,
                   const NameAddr& from,
                   const NameAddr& contact)
{
   return Helper::makeRequest(target, from, contact, INVITE);
}

SipMessage 
Helper::makeResponse(const SipMessage& request, int responseCode)
{
   SipMessage response;
   response.header(h_StatusLine).responseCode() = responseCode;
   response.header(h_From) = request.header(h_From);
   response.header(h_To) = request.header(h_To);
   response.header(h_CallId) = request.header(h_CallId);
   response.header(h_CSeq) = request.header(h_CSeq);
   response.header(h_Vias) = request.header(h_Vias);

   if (responseCode > 100 && responseCode < 500)
   {
      if (!response.header(h_To).exists(p_tag))
      {
         response.header(h_To).param(p_tag) = Helper::computeTag(Helper::tagSize);
      }
   }
   return response;
}

SipMessage 
Helper::makeResponse(const SipMessage& request, int responseCode, const NameAddr& contact)
{
   SipMessage response = Helper::makeResponse(request, responseCode);
   response.header(h_Contacts).push_front(contact);
   return response;
}


//to, requestLine& cseq method set
SipMessage 
Helper::makeRequest(const NameAddr& target, MethodTypes method)
{
   assert(0);
}




SipMessage 
Helper::makeAck(const SipMessage& request, const SipMessage& response)
{
   assert(0);
}


Data 
Helper::computeUniqueBranch()
{
   assert(0);
}

Data 
Helper::computeProxyBranch()
{
   assert(0);
}

Data
Helper::computeCallId()
{
   // !jf! need to include host as well (should cache it)
   return RandomHex::get(4);
}


Data
Helper::computeTag(int numBytes)
{
   return RandomHex::get(4);
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
