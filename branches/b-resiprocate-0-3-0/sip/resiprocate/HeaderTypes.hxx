#if !defined(RESIP_HEADERTYPES_HXX)
#define RESIP_HEADERTYPES_HXX 

#include "resiprocate/supported.hxx"
#include "resiprocate/os/Data.hxx"

// eventually use these macros to automate Headers.hxx, Headers.cxx+gperf
#define UNUSED_defineHeader(_enum, _name, _type, _rfc) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define UNUSED_defineMultiHeader(_enum, _name, _type, _rfc) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define defineHeader(_enum, _name, _type, _rfc) _enum
#define defineMultiHeader(_enum, _name, _type, _rfc) _enum

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
         defineHeader(To, "To", NameAddr, "RFC ????"), 
         defineHeader(From, "From", NameAddr, "RFC ????"),
         defineMultiHeader(Via, "Via", Via, "RFC ????"),
         defineHeader(CallId, "Call-ID", CallId, "RFC ????"),
         defineHeader(CSeq, "CSeq", CSeqCategory, "RFC ????"),
         defineMultiHeader(Route, "Route", NameAddr, "RFC ????"),
         defineMultiHeader(RecordRoute, "Record-Route", NameAddr, "RFC ????"),
         defineMultiHeader(Contact, "Contact", NameAddr, "RFC ????"),
         defineHeader(Subject, "Subject", StringCategory, "RFC ????"),
         defineHeader(Expires, "Expires", IntegerCategory, "RFC ????"),
         defineHeader(MaxForwards, "Max-Forwards", IntegerCategory, "RFC ????"),
         defineMultiHeader(Accept, "Accept", Mime, "RFC ????"),
         defineMultiHeader(AcceptEncoding, "Accept-Encoding", AcceptEncoding, "RFC ????"),
         defineMultiHeader(AcceptLanguage, "Accept-Language", Token, "RFC ????"),
         defineMultiHeader(AlertInfo, "Alert-Info", GenericURI, "RFC ????"),
         defineMultiHeader(Allow, "Allow", Token, "RFC ????"),
         defineHeader(AuthenticationInfo, "Authentication-Info", Auth, "RFC ????"),
         defineMultiHeader(CallInfo, "Call-Info", GenericURI, "RFC ????"),
         defineHeader(ContentDisposition, "Content-Disposition", Token, "RFC ????"),
         defineHeader(ContentEncoding, "Content-Encoding", Token, "RFC ????"),
         defineMultiHeader(ContentLanguage, "Content-Language", Token, "RFC ????"),
         defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", Token, "RFC ????"), // !dlb! defineMultiHeader
         defineHeader(ContentType, "Content-Type", Mime, "RFC ????"),
         defineHeader(Date, "Date", DateCategory, "RFC ????"),
         UNUSED_defineMultiHeader(ErrorInfo, "Error-Info", GenericURI, "RFC ????"),
         defineHeader(InReplyTo, "In-Reply-To", CallId, "RFC ????"),
         defineHeader(MinExpires, "Min-Expires", IntegerCategory, "RFC ????"),
         defineHeader(MIMEVersion, "MIME-Version", Token, "RFC ????"),
         defineHeader(Organization, "Organization", StringCategory, "RFC ????"),
         defineHeader(Priority, "Priority", Token, "RFC ????"),
         defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth, "RFC ????"),
         defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth, "RFC ????"),
         defineMultiHeader(ProxyRequire, "Proxy-Require", Token, "RFC ????"),
         defineHeader(ReplyTo, "Reply-To", NameAddr, "RFC ????"),
         defineMultiHeader(Require, "Require", Token, "RFC ????"),
         defineHeader(RetryAfter, "Retry-After", IntegerCategory, "RFC ????"),
         defineHeader(Server, "Server", StringCategory, "RFC ????"),
         defineMultiHeader(Supported, "Supported", Token, "RFC ????"),
         defineHeader(Timestamp, "Timestamp", StringCategory, "RFC ????"),
         defineMultiHeader(Unsupported, "Unsupported", Token, "RFC ????"),
         defineHeader(UserAgent, "User-Agent", StringCategory, "RFC ????"),
         defineHeader(Warning, "Warning", WarningCategory, "RFC ????"),
         defineMultiHeader(WWWAuthenticate, "WWW-Authenticate", Auth, "RFC ????"),
         defineMultiHeader(SubscriptionState, "Subscription-State", Token, "RFC ????"),
         defineHeader(ReferTo, "Refer-To", NameAddr, "RFC ????"),
         defineHeader(ReferredBy, "Referred-By", NameAddr, "RFC ????"),
         defineMultiHeader(Authorization, "Authorization", header, "RFC ????"),
         defineHeader(Replaces, "Replaces", CallId, "RFC ????"),
         defineHeader(Event, "Event", Token, "RFC ????"),
         defineMultiHeader(AllowEvents, "Allow-Events", Token, "RFC ????"),
         defineMultiHeader(SecurityClient, "Security-Client", Token, "RFC ????"),
         defineMultiHeader(SecurityServer, "Security-Server", Token, "RFC ????"),
         defineMultiHeader(SecurityVerify, "Security-Verify", Token, "RFC ????"),
         defineHeader(ContentLength, "Content-Length", Token, "RFC ????"),

         MAX_HEADERS,
         NONE
      };

      // get enum from header name
      static Type getType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
      static bool isCommaEncoding(Type type);
      static const Data& getHeaderName(int);

      // treat as private
      static bool CommaTokenizing[MAX_HEADERS+1];
      static bool CommaEncoding[MAX_HEADERS+1];
      static Data HeaderNames[MAX_HEADERS+1];
};
 
}

#undef UNUSED_defineHeader
#undef UNUSED_defineMultiHeader
#undef defineHeader
#undef defineMultiHeader

#endif

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
