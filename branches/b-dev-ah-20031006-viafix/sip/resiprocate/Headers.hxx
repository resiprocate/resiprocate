#if !defined(RESIP_HEADERS_HXX)
#define RESIP_HEADERS_HXX 

#include "resiprocate/supported.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{

//#define PARTIAL_TEMPLATE_SPECIALIZATION
#ifdef PARTIAL_TEMPLATE_SPECIALIZATION
template<bool>
class TypeIf
{
   public:
      template <class T>
      class Resolve
      {
         public:
            typedef T Type;
      };
};

class UnusedHeader
{
};

class TypeIf<false>
{
   public:
      template <class T>
      class Resolve
      {
         public:
            typedef UnusedHeader Type;
      };
};

#define UnusedChecking(_enum)                                           \
      typedef TypeIf<Headers::_enum != Headers::UNKNOWN> TypeIfT;       \
      typedef TypeIfT::Resolve<Type> Resolver;                          \
      typedef Resolver::Type UnknownReturn

#define MultiUnusedChecking(_enum)                                              \
      typedef TypeIf<Headers::_enum != Headers::UNKNOWN> TypeIfT;               \
      typedef TypeIfT::Resolve< ParserContainer<Type> > Resolver;               \
      typedef Resolver::Type UnknownReturn

#else

#define UnusedChecking(_enum) typedef int _dummy
#define MultiUnusedChecking(_enum) typedef int _dummy

#endif

class HeaderBase
{
   public:
      virtual Headers::Type getTypeNum() const = 0;
};

#define defineHeader(_enum, _name, _type, _rfc)                 \
class _enum##_Header : public HeaderBase                        \
{                                                               \
   public:                                                      \
      enum {Single = true};                                     \
      typedef _type Type;                                       \
      UnusedChecking(_enum);                                    \
      static Type& knownReturn(ParserContainerBase* container); \
      virtual Headers::Type getTypeNum() const;                 \
      _enum##_Header();                                         \
};                                                              \
extern _enum##_Header h_##_enum

#define defineMultiHeader(_enum, _name, _type, _rfc)                                    \
class _enum##_MultiHeader : public HeaderBase                                           \
{                                                                                       \
   public:                                                                              \
      enum {Single = false};                                                            \
      typedef _type Type;                                                               \
      MultiUnusedChecking(_enum);                                                       \
      static ParserContainer<Type>& knownReturn(ParserContainerBase* container);        \
      virtual Headers::Type getTypeNum() const;                                         \
      _enum##_MultiHeader();                                                            \
};                                                                                      \
extern _enum##_MultiHeader h_##_enum##s

//====================
// Token:
//====================
typedef ParserContainer<Token> Tokens;

class ContentDisposition_Header : public HeaderBase                        \
{                                                               \
   public:                                                      \
      enum {Single = true};                                     \
      typedef Token Type;                                       \
      UnusedChecking(ContentDisposition);                                    \
      static Type& knownReturn(ParserContainerBase* container); \
      virtual Headers::Type getTypeNum() const;                 \
      ContentDisposition_Header();                                         \
};                                                              \
extern ContentDisposition_Header h_ContentDisposition;

//defineHeader(ContentDisposition, "Content-Disposition", Token, "RFC ????");
defineHeader(ContentEncoding, "Content-Encoding", Token, "RFC ????");
defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory, "RFC ????");
defineHeader(MIMEVersion, "Mime-Version", Token, "RFC ????");
defineHeader(Priority, "Priority", Token, "RFC ????");
defineHeader(Event, "Event", Token, "RFC ????");
defineMultiHeader(AllowEvents, "Allow-Events", Token, "RFC ????");
// explicitly declare to avoid h_AllowEventss, ugh
extern AllowEvents_MultiHeader h_AllowEvents;

defineMultiHeader(AcceptEncoding, "Accept-Encoding", Token, "RFC ????");
defineMultiHeader(AcceptLanguage, "Accept-Language", Token, "RFC ????");
defineMultiHeader(Allow, "Allow", Token, "RFC ????");
defineMultiHeader(ContentLanguage, "Content-Language", Token, "RFC ????");
defineMultiHeader(ProxyRequire, "Proxy-Require", Token, "RFC ????");
defineMultiHeader(Require, "Require", Token, "RFC ????");
defineMultiHeader(Supported, "Supported", Token, "RFC ????");
defineMultiHeader(SubscriptionState, "Subscription-State", Token, "RFC ????");
defineMultiHeader(Unsupported, "Unsupported", Token, "RFC ????");
defineMultiHeader(SecurityClient, "Security-Client", Token, "RFC ????");
defineMultiHeader(SecurityServer, "Security-Server", Token, "RFC ????");
defineMultiHeader(SecurityVerify, "Security-Verify", Token, "RFC ????");
// explicitly declare to avoid h_SecurityVerifys, ugh
extern SecurityVerify_MultiHeader h_SecurityVerifies;

//====================
// Mime
//====================
typedef ParserContainer<Mime> Mimes;

defineMultiHeader(Accept, "Accept", Mime, "RFC ????");
defineHeader(ContentType, "Content-Type", Mime, "RFC ????");

//====================
// GenericURIs:
//====================
typedef ParserContainer<GenericURI> GenericURIs;
defineMultiHeader(CallInfo, "Call-Info", GenericURI, "RFC ????");
defineMultiHeader(AlertInfo, "Alert-Info", GenericURI, "RFC ????");
defineMultiHeader(ErrorInfo, "Error-Info", GenericURI, "RFC ????");

//====================
// NameAddr:
//====================
typedef ParserContainer<NameAddr> NameAddrs;

defineMultiHeader(RecordRoute, "Record-Route", NameAddr, "RFC ????");
defineMultiHeader(Route, "Route", NameAddr, "RFC ????");
defineMultiHeader(Contact, "Contact", NameAddr, "RFC ????");
defineHeader(From, "From", NameAddr, "RFC ????");
defineHeader(To, "To", NameAddr, "RFC ????");
defineHeader(ReplyTo, "Reply-To", NameAddr, "RFC ????");
defineHeader(ReferTo, "Refer-To", NameAddr, "RFC ????");
defineHeader(ReferredBy, "Referred-By", NameAddr, "RFC ????");

//====================
// String:
//====================
typedef ParserContainer<StringCategory> StringCategories;

defineHeader(Organization, "Organization", StringCategory, "RFC ????");
defineHeader(Server, "Server", StringCategory, "RFC ????");
defineHeader(Subject, "Subject", StringCategory, "RFC ????");
defineHeader(UserAgent, "User-Agent", StringCategory, "RFC ????");
defineHeader(Timestamp, "Timestamp", StringCategory, "RFC ????");

//====================
// Integer:
//====================
typedef ParserContainer<IntegerCategory> IntegerCategories;

// !dlb! not clear this needs to be exposed
defineHeader(ContentLength, "Content-Length", IntegerCategory, "RFC ????");
defineHeader(MaxForwards, "Max-Forwards", IntegerCategory, "RFC ????");
defineHeader(MinExpires, "Min-Expires", IntegerCategory, "RFC ????");

// !dlb! this one is not quite right -- can have (comment) after field value
defineHeader(RetryAfter, "Retry-After", IntegerCategory, "RFC ????");
defineHeader(Expires, "Expires", ExpiresCategory, "RFC ????");

//====================
// CallId:
//====================
defineHeader(CallId, "Call-ID", CallId, "RFC ????");
defineHeader(Replaces, "Replaces", CallId, "RFC ????");
defineHeader(InReplyTo, "In-Reply-To", CallId, "RFC ????");

//====================
// Auth:
//====================
typedef ParserContainer<Auth> Auths;
defineHeader(AuthenticationInfo, "Authentication-Info", Auth, "RFC ????");
defineMultiHeader(Authorization, "Authorization", Auth, "RFC ????");
defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth, "RFC ????");
defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth, "RFC ????");
defineMultiHeader(WWWAuthenticate, "Www-Authenticate", Auth, "RFC ????");

//====================
// CSeqCategory:
//====================
defineHeader(CSeq, "CSeq", CSeqCategory, "RFC ????");

//====================
// DateCategory:
//====================
defineHeader(Date, "Date", DateCategory, "RFC ????");

//====================
// WarningCategory:
//====================
defineHeader(Warning, "Warning", WarningCategory, "RFC ????");

//====================
// Via
//====================
typedef ParserContainer<Via> Vias;
defineMultiHeader(Via, "Via", Via, "RFC ????");

class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;
 
}

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
