#ifndef Headers_hxx
#define Headers_hxx

#include "resiprocate/supported.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/HeaderTypes.hxx"

namespace Vocal2
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
      typedef Resolver::Type UnknownReturn;

#define MultiUnusedChecking(_enum)                                              \
      typedef TypeIf<Headers::_enum != Headers::UNKNOWN> TypeIfT;               \
      typedef TypeIfT::Resolve< ParserContainer<Type> > Resolver;               \
      typedef Resolver::Type UnknownReturn;

#else

#define UnusedChecking(_enum)
#define MultiUnusedChecking(_enum)

#endif

class HeaderBase
{
   public:
      virtual Headers::Type getTypeNum() const = 0;
};

#define defineHeader(_enum, _name, _type)                       \
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

#define defineMultiHeader(_enum, _name, _type)                                          \
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

defineHeader(ContentDisposition, "Content-Disposition", Token);
defineHeader(ContentEncoding, "Content-Encoding", Token);
defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory);
defineHeader(MIMEVersion, "Mime-Version", Token);
defineHeader(Priority, "Priority", Token);
defineHeader(Event, "Event", Token);
defineMultiHeader(AllowEvents, "Allow-Events", Token);
// explicitly declare to avoid h_AllowEventss, ugh
extern AllowEvents_MultiHeader h_AllowEvents;

defineMultiHeader(AcceptEncoding, "Accept-Encoding", Token);
defineMultiHeader(AcceptLanguage, "Accept-Language", Token);
defineMultiHeader(Allow, "Allow", Token);
defineMultiHeader(ContentLanguage, "Content-Language", Token);
defineMultiHeader(ProxyRequire, "Proxy-Require", Token);
defineMultiHeader(Require, "Require", Token);
defineMultiHeader(Supported, "Supported", Token);
defineMultiHeader(SubscriptionState, "Subscription-State", Token);
defineMultiHeader(Unsupported, "Unsupported", Token);
defineMultiHeader(SecurityClient, "Security-Client", Token);
defineMultiHeader(SecurityServer, "Security-Server", Token);
defineMultiHeader(SecurityVerify, "Security-Verify", Token);
// explicitly declare to avoid h_AllowEventss, ugh
extern SecurityVerify_MultiHeader h_SecurityVerifies;

//====================
// Mime
//====================
typedef ParserContainer<Mime> Mimes;

defineMultiHeader(Accept, "Accept", Mime);
defineHeader(ContentType, "Content-Type", Mime);

//====================
// GenericURIs:
//====================
typedef ParserContainer<GenericURI> GenericURIs;
defineMultiHeader(CallInfo, "Call-Info", GenericURI);
defineMultiHeader(AlertInfo, "Alert-Info", GenericURI);
defineMultiHeader(ErrorInfo, "Error-Info", GenericURI);

//====================
// NameAddr:
//====================
typedef ParserContainer<NameAddr> NameAddrs;

defineMultiHeader(RecordRoute, "Record-Route", NameAddr);
defineMultiHeader(Route, "Route", NameAddr);
defineMultiHeader(Contact, "Contact", NameAddr);
defineHeader(From, "From", NameAddr);
defineHeader(To, "To", NameAddr);
defineHeader(ReplyTo, "Reply-To", NameAddr);
defineHeader(ReferTo, "Refer-To", NameAddr);
defineHeader(ReferredBy, "Referred-By", NameAddr);

//====================
// String:
//====================
typedef ParserContainer<StringCategory> StringCategories;

defineHeader(Organization, "Organization", StringCategory);
defineHeader(Server, "Server", StringCategory);
defineHeader(Subject, "Subject", StringCategory);
defineHeader(UserAgent, "User-Agent", StringCategory);
defineHeader(Timestamp, "Timestamp", StringCategory);

//====================
// Integer:
//====================
typedef ParserContainer<IntegerCategory> IntegerCategories;

// !dlb! not clear this needs to be exposed
defineHeader(ContentLength, "Content-Length", IntegerCategory);
defineHeader(MaxForwards, "Max-Forwards", IntegerCategory);
defineHeader(MinExpires, "Min-Expires", IntegerCategory);

// !dlb! this one is not quite right -- can have (comment) after field value
defineHeader(RetryAfter, "Retry-After", IntegerCategory);
defineHeader(Expires, "Expires", ExpiresCategory);

//====================
// CallId:
//====================
defineHeader(CallId, "Call-ID", CallId);
defineHeader(Replaces, "Replaces", CallId);
defineHeader(InReplyTo, "In-Reply-To", CallId);

//====================
// Auth:
//====================
typedef ParserContainer<Auth> Auths;
defineHeader(AuthenticationInfo, "Authentication-Info", Auth);
defineMultiHeader(Authorization, "Authorization", Auth);
defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth);
defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth);
defineMultiHeader(WWWAuthenticate, "Www-Authenticate", Auth);

//====================
// CSeqCategory:
//====================
defineHeader(CSeq, "CSeq", CSeqCategory);

//====================
// DateCategory:
//====================
defineHeader(Date, "Date", DateCategory);

//====================
// WarningCategory:
//====================
defineHeader(Warning, "Warning", WarningCategory);

//====================
// Via
//====================
typedef ParserContainer<Via> Vias;
defineMultiHeader(Via, "Via", Via);

class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;
 
}

#undef defineHeader
#undef defineMultiHeader

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
