#if !defined(RESIP_HEADERTYPES_HXX)
#define RESIP_HEADERTYPES_HXX 

#include "rutil/Data.hxx"

//****************************************************************************
//
// !dlb! until automated, must ensure that this set is consistent with
//
// ************ PLEASE RUN testParserCategories AFTER CHANGING ***************
//
// HeaderTypes.hxx
// Headers.hxx
// HeaderHash.gperf
// Headers.cxx
// SipMessage.hxx
// SipMessage.cxx
//
//****************************************************************************

// eventually use these macros to automate Headers.hxx, Headers.cxx+gperf
#define UNUSED_defineHeader(_enum, _name, _type, _rfc) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define UNUSED_defineMultiHeader(_enum, _name, _type, _rfc) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1
#define defineHeader(_enum, _name, _type, _rfc) _enum
#define defineMultiHeader(_enum, _name, _type, _rfc) _enum

namespace resip
{

/**
   Maps from header name to derived ParserCategory. Determines whether the
   header is single or multiple valued.

   The Type enum controls the order of output of the headers in the encoded
   SipMessage.  Put headers that you want to appear early in the message early
   in this list.
*/
class Headers
{
   public:
      enum Type
      {
         UNKNOWN = -1,
         defineMultiHeader(Via, "Via", Via, "RFC 3261"), // rjs says must be first
         defineHeader(MaxForwards, "Max-Forwards", UInt32Category, "RFC 3261"),
         defineMultiHeader(Route, "Route", NameAddr, "RFC 3261"),
         defineMultiHeader(RecordRoute, "Record-Route", NameAddr, "RFC 3261"),
         defineMultiHeader(Path, "Path", NameAddr, "RFC 3327"),
         defineMultiHeader(ServiceRoute, "Service-Route", NameAddr, "RFC 3608"),
         defineMultiHeader(ProxyRequire, "Proxy-Require", Token, "RFC 3261"),
         defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth, "RFC 3261"),
         defineHeader(Identity, "Identity", StringCategory, "RFC 4474"),
         defineHeader(IdentityInfo, "Identity-Info", GenericUri, "RFC 4474"),
         defineMultiHeader(Require, "Require", Token, "RFC 3261"),
         defineMultiHeader(Contact, "Contact", NameAddr, "RFC 3261"),

         defineHeader(To, "To", NameAddr, "RFC 3261"), 
         defineHeader(From, "From", NameAddr, "RFC 3261"),
         defineHeader(CallID, "Call-ID", CallId, "RFC 3261"),
         defineHeader(CSeq, "CSeq", CSeqCategory, "RFC 3261"),
         defineHeader(Subject, "Subject", StringCategory, "RFC 3261"),
         defineHeader(Expires, "Expires", ExpiresCategory, "RFC 3261"),
         defineHeader(SessionExpires, "Session-Expires", ExpiresCategory, "RFC 4028"),
         defineHeader(MinSE, "Min-SE", ExpiresCategory, "RFC 4028"),
         defineMultiHeader(Accept, "Accept", Mime, "RFC 3261"),
         defineMultiHeader(AcceptEncoding, "Accept-Encoding", Token, "RFC 3261"),
         defineMultiHeader(AcceptLanguage, "Accept-Language", Token, "RFC 3261"),
         defineMultiHeader(AlertInfo, "Alert-Info", GenericUri, "RFC 3261"),
         defineMultiHeader(Allow, "Allow", Token, "RFC 3261"),
         defineHeader(AuthenticationInfo, "Authentication-Info", Auth, "RFC 3261"),
         defineMultiHeader(CallInfo, "Call-Info", GenericUri, "RFC 3261"),
         defineHeader(ContentDisposition, "Content-Disposition", Token, "RFC ?"),
         defineHeader(ContentEncoding, "Content-Encoding", Token, "RFC ?"),
         defineHeader(ContentId, "Content-ID", Token, "RFC 2045"),
         defineMultiHeader(ContentLanguage, "Content-Language", Token, "RFC ?"),
// i really think that Content-Transfer-Encoding should be a Token   !rwm
         defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory, "RFC ?"), // !dlb! defineMultiHeader
         defineHeader(ContentType, "Content-Type", Mime, "RFC 3261"),
         defineHeader(Date, "Date", DateCategory, "RFC 3261"),
         defineMultiHeader(ErrorInfo, "Error-Info", GenericUri, "RFC 3261"),
         defineHeader(InReplyTo, "In-Reply-To", CallId, "RFC 3261"),
         defineHeader(MinExpires, "Min-Expires", UInt32Category, "RFC 3261"),
         defineHeader(MIMEVersion, "MIME-Version", Token, "RFC 3261"),
         defineHeader(Organization, "Organization", StringCategory, "RFC 3261"),
         defineHeader(SecWebSocketKey, "Sec-WebSocket-Key", StringCategory, "RFC 6455"),
         defineHeader(SecWebSocketKey1, "Sec-WebSocket-Key1", StringCategory, "draft-hixie- thewebsocketprotocol-76"),
         defineHeader(SecWebSocketKey2, "Sec-WebSocket-Key2", StringCategory, "draft-hixie- thewebsocketprotocol-76"),
         defineHeader(Origin, "Origin", StringCategory, "draft-hixie- thewebsocketprotocol-76"),
         defineHeader(Host, "Host", StringCategory, "draft-hixie- thewebsocketprotocol-76"),
         defineHeader(SecWebSocketAccept, "Sec-WebSocket-Accept", StringCategory, "RFC 6455"),
         defineMultiHeader(Cookie, "Cookie", StringCategory, "RFC 6265"),
         defineHeader(Priority, "Priority", Token, "RFC 3261"),
         defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth, "RFC 3261"),
         defineHeader(ReplyTo, "Reply-To", NameAddr, "RFC 3261"),
         defineHeader(RetryAfter, "Retry-After", UInt32Category, "RFC 3261"),
         defineHeader(FlowTimer, "Flow-Timer", UInt32Category, "RFC 5626"),
         defineHeader(Server, "Server", StringCategory, "RFC 3261"),
         defineHeader(SIPETag, "SIP-ETag", Token, "RFC 3903"),
         defineHeader(SIPIfMatch, "SIP-If-Match", Token, "RFC 3903"),
         defineMultiHeader(Supported, "Supported", Token, "RFC 3261"),
         defineHeader(Timestamp, "Timestamp", StringCategory, "RFC 3261"),
         defineMultiHeader(Unsupported, "Unsupported", Token, "RFC 3261"),
         defineHeader(UserAgent, "User-Agent", StringCategory, "RFC 3261"),
         defineMultiHeader(Warning, "Warning", WarningCategory, "RFC 3261"),
         defineMultiHeader(WWWAuthenticate, "WWW-Authenticate", Auth, "RFC 3261"),
         defineHeader(SubscriptionState, "Subscription-State", Token, "RFC 3265"),
         defineHeader(ReferTo, "Refer-To", NameAddr, "RFC 3515"),
         defineHeader(ReferredBy, "Referred-By", NameAddr, "RFC 3892"),
         defineMultiHeader(Authorization, "Authorization", Auth, "RFC 3261"),
         defineHeader(Replaces, "Replaces", CallId, "RFC 3891"),
         defineHeader(Event, "Event", Token, "RFC 3265"),
         defineMultiHeader(AllowEvents, "Allow-Events", Token, "RFC 3265"),
         defineMultiHeader(SecurityClient, "Security-Client", Token, "RFC 3329"),
         defineMultiHeader(SecurityServer, "Security-Server", Token, "RFC 3329"),
         defineMultiHeader(SecurityVerify, "Security-Verify", Token, "RFC 3329"),
         defineHeader(RSeq, "RSeq", UInt32Category, "RFC 3262"),
         defineHeader(RAck, "RAck", RAckCategory, "RFC 3262"),

         defineMultiHeader(Reason, "Reason", Token, "RFC 3326"),
         defineMultiHeader(Privacy, "Privacy", PrivacyCategory, "RFC 3323"),
         defineMultiHeader(RequestDisposition, "Request-Disposition", Token, "RFC 3841"),
         defineMultiHeader(PMediaAuthorization, "P-Media-Authorization", Token, "RFC 3313"),
         defineHeader(Join, "Join", CallId, "RFC 3911"),
         defineHeader(TargetDialog, "Target-Dialog", CallId, "RFC 4538"),
         defineMultiHeader(PAssertedIdentity, "P-Asserted-Identity", NameAddr, "RFC 3325"),
         defineMultiHeader(PPreferredIdentity, "P-Preferred-Identity", NameAddr, "RFC 3325"),
         defineMultiHeader(AcceptContact, "Accept-Contact", NameAddr, "RFC 3841"),
         defineMultiHeader(RejectContact, "Reject-Contact", NameAddr, "RFC 3841"),
         defineHeader(PCalledPartyId, "P-Called-Party-ID", NameAddr, "RFC 3455"),
         defineMultiHeader(PAssociatedUri, "P-Associated-URI", NameAddr, "RFC 3455"),

         defineHeader(ContentLength, "Content-Length", UInt32Category, "RFC 3261"),
         defineHeader(ReferSub, "Refer-Sub", Token, "RFC 4488"),
         defineHeader(AnswerMode, "Answer-Mode", Token, "RFC 5373"),
         defineHeader(PrivAnswerMode, "Priv-Answer-Mode", Token, "RFC 5373"),
         defineMultiHeader(RemotePartyId, "Remote-Party-ID", NameAddr, "draft-ietf-sip-privacy-04"), // ?bwc? Not in 3323, should we keep?
         defineMultiHeader(HistoryInfo, "History-Info", NameAddr, "RFC 4244"),

         defineHeader(PAccessNetworkInfo, "P-Access-Network-Info", Token, "RFC 3455"),
         defineHeader(PChargingVector, "P-Charging-Vector", Token, "RFC 3455"),
         defineHeader(PChargingFunctionAddresses, "P-Charging-Function-Addresses", Token, "RFC 3455"),
         defineMultiHeader(PVisitedNetworkID, "P-Visited-Network-ID", TokenOrQuotedStringCategory, "RFC 3455"),

         defineMultiHeader(UserToUser, "User-to-User", TokenOrQuotedStringCategory, "draft-ietf-cuss-sip-uui-17"),

         defineMultiHeader(RESIP_DO_NOT_USE, "ShouldNotSeeThis", StringCategory, "N/A"),
         MAX_HEADERS,
         NONE
      };

      // get enum from header name
      static Type getType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
      static bool isCommaEncoding(Type type);
      static const Data& getHeaderName(int);
      static bool isMulti(Type type);

      // treat as private
      static bool CommaTokenizing[MAX_HEADERS+1];
      static bool CommaEncoding[MAX_HEADERS+1];
      static Data HeaderNames[MAX_HEADERS+1];
      static bool Multi[MAX_HEADERS+1];
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
