#include <string.h>


#include "sip2/sipstack/Helper.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/Preparse.hxx"

#include "sip2/util/RandomHex.hxx"

using namespace Vocal2;

const int Helper::tagSize = 4;

SipMessage*
Helper::makeRequest(const NameAddr& target, const NameAddr& from, const NameAddr& contact, MethodTypes method)
{
   SipMessage* request = new SipMessage;
   RequestLine rLine(method);
   rLine.uri() = target.uri();
   request->header(h_To) = target;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = method;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = from;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_Contacts).push_front(contact);
   request->header(h_CallId).value() = Helper::computeCallId();
   request->header(h_ContentLength).value() = 0;
   
   Via via;
   request->header(h_Vias).push_front(via);
   
   return request;
}
// MOVEME !ah! this goes in a test_support package
SipMessage*
Helper::makeMessage(const Data& data, bool isExternal )
{
   SipMessage* msg = new SipMessage(isExternal);
   int size = data.size();
   char *buffer = new char[size];

#if 0 // !ah!
   memcpy(buffer,data.data(),size);
   
   PreparseState::TransportAction status = PreparseState::NONE;
   
   Preparse pre;

   int used = 0;
   
   pre.process(*msg, buffer, size, used, status);
   if (status == PreparseState::preparseError ||
       status == PreparseState::fragment)
   {
      delete msg;
      msg = 0;
   }
   else
   {
      // no pp error
      if (status  PreparseState::headersComplete &&
          used < size)
      {
         // body is present .. add it up.
         // NB. The Sip Message uses an overlay (again)
         // for the body. It ALSO expects that the body
         // will be contiguous (of course).
         // it doesn't need a new buffer in UDP b/c there
         // will only be one datagram per buffer. (1:1 strict)
         
         msg->setBody(buffer+used,size-used);
      }
   }
#endif
   return msg;
}

SipMessage*
Helper::makeRegister(const NameAddr& registrar,
                     const NameAddr& aor)
{

   SipMessage* request = new SipMessage;
   RequestLine rLine(REGISTER);
   rLine.uri() = registrar.uri();

   request->header(h_To) = aor;
   request->header(h_RequestLine) = rLine;
   request->header(h_MaxForwards).value() = 70;
   request->header(h_CSeq).method() = REGISTER;
   request->header(h_CSeq).sequence() = 1;
   request->header(h_From) = aor;
   request->header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   request->header(h_CallId).value() = Helper::computeCallId();
   request->header(h_ContentLength).value() = 0;

   Via via;
   request->header(h_Vias).push_front(via);
   
   return request;
}

SipMessage*
Helper::makeInvite(const NameAddr& target, const NameAddr& from, const NameAddr& contact)
{
   return Helper::makeRequest(target, from, contact, INVITE);
}


SipMessage*
Helper::makeResponse(const SipMessage& request, int responseCode, const Data& reason)
{
   SipMessage* response = new SipMessage;
   response->header(h_StatusLine).responseCode() = responseCode;
   response->header(h_From) = request.header(h_From);
   response->header(h_To) = request.header(h_To);
   response->header(h_CallId) = request.header(h_CallId);
   response->header(h_CSeq) = request.header(h_CSeq);
   response->header(h_Vias) = request.header(h_Vias);
   response->header(h_ContentLength).value() = 0;
   
   if (request.exists(h_RecordRoutes))
   {
      response->header(h_RecordRoutes) = request.header(h_RecordRoutes);
   }

   if (request.isExternal())
   {
       response->setFromTU();
   }
   else
   {
       response->setFromExternal();
   }

   if (reason.size())
   {
      response->header(h_StatusLine).reason() = reason;
   }
   else
   {
      switch (responseCode)
      {
         case 100: response->header(h_StatusLine).reason() = "Trying"; break;
         case 180: response->header(h_StatusLine).reason() = "Ringing"; break;
         case 181: response->header(h_StatusLine).reason() = "Call Is Being Forwarded"; break;
         case 182: response->header(h_StatusLine).reason() = "Queued"; break;
         case 183: response->header(h_StatusLine).reason() = "Session Progress"; break;
         case 200: response->header(h_StatusLine).reason() = "OK"; break;
         case 300: response->header(h_StatusLine).reason() = "Multiple Choices"; break;
         case 301: response->header(h_StatusLine).reason() = "Moved Permanently"; break;
         case 302: response->header(h_StatusLine).reason() = "Moved Temporarily"; break;
         case 305: response->header(h_StatusLine).reason() = "Use Proxy"; break;
         case 380: response->header(h_StatusLine).reason() = "Alternative Service"; break;
         case 400: response->header(h_StatusLine).reason() = "Bad Request"; break;
         case 401: response->header(h_StatusLine).reason() = "Unauthorized"; break;
         case 402: response->header(h_StatusLine).reason() = "Payment Required"; break;
         case 403: response->header(h_StatusLine).reason() = "Forbidden"; break;
         case 404: response->header(h_StatusLine).reason() = "Not Found"; break;
         case 405: response->header(h_StatusLine).reason() = "Method Not Allowed"; break;
         case 406: response->header(h_StatusLine).reason() = "Not Acceptable"; break;
         case 407: response->header(h_StatusLine).reason() = "Proxy Authentication Required"; break;
         case 408: response->header(h_StatusLine).reason() = "Request Timeout"; break;
         case 410: response->header(h_StatusLine).reason() = "Gone"; break;
         case 413: response->header(h_StatusLine).reason() = "Request Entity Too Large"; break;
         case 414: response->header(h_StatusLine).reason() = "Request-URI Too Long"; break;
         case 415: response->header(h_StatusLine).reason() = "Unsupported Media Type"; break;
         case 416: response->header(h_StatusLine).reason() = "Unsupported URI Scheme"; break;
         case 420: response->header(h_StatusLine).reason() = "Bad Extension"; break;
         case 421: response->header(h_StatusLine).reason() = "Extension Required"; break;
         case 423: response->header(h_StatusLine).reason() = "Interval Too Brief"; break;
         case 480: response->header(h_StatusLine).reason() = "Temporarily Unavailable"; break;
         case 481: response->header(h_StatusLine).reason() = "Call/Transaction Does Not Exist"; break;
         case 482: response->header(h_StatusLine).reason() = "Loop Detected"; break;
         case 483: response->header(h_StatusLine).reason() = "Too Many Hops"; break;
         case 484: response->header(h_StatusLine).reason() = "Address Incomplete"; break;
         case 485: response->header(h_StatusLine).reason() = "Ambiguous"; break;
         case 486: response->header(h_StatusLine).reason() = "Busy Here"; break;
         case 487: response->header(h_StatusLine).reason() = "Request Terminated"; break;
         case 488: response->header(h_StatusLine).reason() = "Not Acceptable Here"; break;
         case 491: response->header(h_StatusLine).reason() = "Request Pending"; break;
         case 493: response->header(h_StatusLine).reason() = "Undecipherable"; break;
         case 500: response->header(h_StatusLine).reason() = "Server Internal Error"; break;
         case 501: response->header(h_StatusLine).reason() = "Not Implemented"; break;
         case 502: response->header(h_StatusLine).reason() = "Bad Gateway"; break;
         case 503: response->header(h_StatusLine).reason() = "Service Unavailable"; break;
         case 504: response->header(h_StatusLine).reason() = "Server Time-out"; break;
         case 505: response->header(h_StatusLine).reason() = "Version Not Supported"; break;
         case 513: response->header(h_StatusLine).reason() = "Message Too Large"; break;
         case 600: response->header(h_StatusLine).reason() = "Busy Everywhere"; break;
         case 603: response->header(h_StatusLine).reason() = "Decline"; break;
         case 604: response->header(h_StatusLine).reason() = "Does Not Exist Anywhere"; break;
         case 606: response->header(h_StatusLine).reason() = "Not Acceptable"; break;
      }
   }

   return response;
}


SipMessage*
Helper::makeResponse(const SipMessage& request, int responseCode, const NameAddr& contact,  const Data& reason)
{

   SipMessage* response = Helper::makeResponse(request, responseCode, reason);
   response->header(h_Contacts).push_front(contact);
   return response;
}


//to, requestLine& cseq method set
SipMessage*
Helper::makeRequest(const NameAddr& target, MethodTypes method)
{
   assert(0);
   SipMessage* junk=0;
   return junk;
}


// This interface should be used by the stack (TransactionState) to create an
// AckMsg to a failure response
// See RFC3261 section 17.1.1.3
// Note that the branch in this ACK needs to be the 
// For TU generated ACK, see Dialog::makeAck(...)
SipMessage*
Helper::makeFailureAck(const SipMessage& request, const SipMessage& response)
{
   assert (request.header(h_Vias).size() >= 1);
   assert (request.header(h_RequestLine).getMethod() == INVITE);
   
   SipMessage* ack = new SipMessage;
   ack->header(h_CallId) = request.header(h_CallId);
   ack->header(h_From) = request.header(h_From);
   ack->header(h_RequestLine) = request.header(h_RequestLine);
   ack->header(h_To) = response.header(h_To); // to get to-tag
   ack->header(h_Vias).push_back(request.header(h_Vias).front());
   ack->header(h_CSeq) = request.header(h_CSeq);
   ack->header(h_CSeq).method() = ACK;
   ack->header(h_Routes) = request.header(h_Routes);
   ack->header(h_ContentLength).value() = 0;
   
   return ack;
}


Data 
Helper::computeUniqueBranch()
{
   Data result("z9hG4bK"); // magic cookie per rfc2543bis-09    
   result += RandomHex::get(8);
   return result;
}


Data
Helper::computeCallId()
{
   // !jf! need to include host as well (should cache it)
   return RandomHex::get(8);
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
