#ifndef HeaderTypes_hxx
#define HeaderTypes_hxx

#include "resiprocate/supported.hxx"
#include "resiprocate/os/Data.hxx"

// eventually use these macros to automate Headers.hxx, Headers.cxx+gperf
#define UNUSEDsingle(_enum, _category) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define UNUSEDmulti(_enum, _category) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define single(_enum, _category) _enum
#define multi(_enum, _category) _enum

namespace resip
{

class Headers
{
   public:
      // put headers that you want to appear early in the message early in
      // this set

      // !dlb! until automated, must ensure that this set is consistent with
      // Headers.hxx and gperf in Headers.cxx
      enum Type
      {
         UNKNOWN = -1,
         single(To, NameAddr), 
         single(From, NameAddr),
         multi(Via, Via),
         single(CallId, CallId),
         single(CSeq, CSeqCategory),
         multi(Route, NameAddr),
         multi(RecordRoute, NameAddr),
         multi(Contact, NameAddr),
         single(Subject, StringCategory),
         single(Expires, IntegerCategory),
         single(MaxForwards, IntegerCategory),
         multi(Accept, Mime),
         multi(AcceptEncoding, AcceptEncoding),
         multi(AcceptLanguage, Token),
         multi(AlertInfo, GenericURI),
         multi(Allow, Token),
         single(AuthenticationInfo, Auth),
         multi(CallInfo, GenericURI),
         single(ContentDisposition, Token),
         single(ContentEncoding, Token),
         multi(ContentLanguage, Token),
         single(ContentTransferEncoding, Token), // !dlb! multi
         single(ContentType, Mime),
         single(Date, DateCategory),
         UNUSEDmulti(ErrorInfo, GenericURI),
         single(InReplyTo, CallId),
         single(MinExpires, IntegerCategory),
         single(MIMEVersion, Token),
         single(Organization, StringCategory),
         single(Priority, Token),
         multi(ProxyAuthenticate, Auth),
         multi(ProxyAuthorization, Auth),
         multi(ProxyRequire, Token),
         single(ReplyTo, NameAddr),
         multi(Require, Token),
         single(RetryAfter, IntegerCategory),
         single(Server, StringCategory),
         multi(Supported, Token),
         single(Timestamp, StringCategory),
         multi(Unsupported, Token),
         single(UserAgent, StringCategory),
         single(Warning, WarningCategory),
         multi(WWWAuthenticate, Auth),
         multi(SubscriptionState,Token),
         single(ReferTo, NameAddr),
         single(ReferredBy, NameAddr),
         multi(Authorization, header),
         single(Replaces, CallId),
         single(Event, Token),
         multi(AllowEvents, Token),
         multi(SecurityClient, Token),
         multi(SecurityServer, Token),
         multi(SecurityVerify, Token),
         single(ContentLength, Token),

         MAX_HEADERS,
         NONE
      };

      // get enum from header name
      static Type getType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
      static const Data& getHeaderName(int);

      // treat as private
      static bool CommaTokenizing[MAX_HEADERS+1];
      static Data HeaderNames[MAX_HEADERS+1];
};
 
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

#endif
