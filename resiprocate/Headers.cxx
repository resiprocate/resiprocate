#include "sip2/util/Data.hxx"
#include "sip2/sipstack/Headers.hxx"
#include "sip2/sipstack/Symbols.hxx"

#include <iostream>
using namespace std;

//int strcasecmp(const char*, const char*);
//int strncasecmp(const char*, const char*, int len);

using namespace Vocal2;

Data Headers::HeaderNames[MAX_HEADERS];
bool Headers::CommaTokenizing[] = {false};

bool 
Headers::isCommaTokenizing(Type type)
{
   return CommaTokenizing[type];
}

#define defineHeader(_enum, _name, _type)                               \
Headers::Type                                                           \
_enum##_Header::getTypeNum() const {return Headers::_enum;}             \
_enum##_Header::_enum##_Header()                                        \
{                                                                       \
   Headers::CommaTokenizing[Headers::_enum] = Type::isCommaTokenizing;  \
   Headers::HeaderNames[Headers::_enum] = _name;                        \
}                                                                       \
                                                                        \
_type&                                                                  \
_enum##_Header::knownReturn(ParserContainerBase* container)             \
{                                                                       \
   return dynamic_cast<ParserContainer<_type>*>(container)->front();    \
}                                                                       \
                                                                        \
_enum##_Header Vocal2::h_##_enum

#define defineMultiHeader(_enum, _name, _type)                          \
Headers::Type                                                           \
_enum##_MultiHeader::getTypeNum() const {return Headers::_enum;}        \
_enum##_MultiHeader::_enum##_MultiHeader()                              \
{                                                                       \
   Headers::CommaTokenizing[Headers::_enum] = Type::isCommaTokenizing;  \
   Headers::HeaderNames[Headers::_enum] = _name;                        \
}                                                                       \
                                                                        \
ParserContainer<_type>&                                                 \
_enum##_MultiHeader::knownReturn(ParserContainerBase* container)        \
{                                                                       \
   return *dynamic_cast<ParserContainer<_type>*>(container);            \
}                                                                       \
                                                                        \
_enum##_MultiHeader Vocal2::h_##_enum##s

defineHeader(ContentDisposition, "Content-Disposition", Token);
defineHeader(ContentEncoding, "Content-Encoding", Token);
defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory);
defineHeader(MIMEVersion, "Mime-Version", Token);
defineHeader(Priority, "Priority", Token);
defineHeader(Event, "Event", Token);
defineMultiHeader(AllowEvents, "Allow-Events", Token);
// explicitly declare to avoid h_AllowEventss, ugh
AllowEvents_MultiHeader Vocal2::h_AllowEvents;

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
// explicitly declare to avoid h_SecurityVerifys, ugh
SecurityVerify_MultiHeader Vocal2::h_SecurityVerifies;

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

defineMultiHeader(Via, "Via", Via);

RequestLineType Vocal2::h_RequestLine;
StatusLineType Vocal2::h_StatusLine;

/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -g -Z HeaderHash -E -L C++ -t -k '*' -D headers.gperf  */
struct headers { char *name; Headers::Type type; };
/* maximum key range = 494, duplicates = 0 */

class HeaderHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct headers *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
HeaderHash::hash (register const char *str, register unsigned int len)
{
  static unsigned short asso_values[] =
    {
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495,   0, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495,   0,   0, 102,
        0,   0,  60,  20,  15,  10,   0,   0,  15,  50,
        0,  20,   0,  10,   0,  30,  35,   0,  45,   0,
       60,   0,   0, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 25:
        hval += asso_values[(unsigned char)tolower(str[24])];
      case 24:
        hval += asso_values[(unsigned char)tolower(str[23])];
      case 23:
        hval += asso_values[(unsigned char)tolower(str[22])];
      case 22:
        hval += asso_values[(unsigned char)tolower(str[21])];
      case 21:
        hval += asso_values[(unsigned char)tolower(str[20])];
      case 20:
        hval += asso_values[(unsigned char)tolower(str[19])];
      case 19:
        hval += asso_values[(unsigned char)tolower(str[18])];
      case 18:
        hval += asso_values[(unsigned char)tolower(str[17])];
      case 17:
        hval += asso_values[(unsigned char)tolower(str[16])];
      case 16:
        hval += asso_values[(unsigned char)tolower(str[15])];
      case 15:
        hval += asso_values[(unsigned char)tolower(str[14])];
      case 14:
        hval += asso_values[(unsigned char)tolower(str[13])];
      case 13:
        hval += asso_values[(unsigned char)tolower(str[12])];
      case 12:
        hval += asso_values[(unsigned char)tolower(str[11])];
      case 11:
        hval += asso_values[(unsigned char)tolower(str[10])];
      case 10:
        hval += asso_values[(unsigned char)tolower(str[9])];
      case 9:
        hval += asso_values[(unsigned char)tolower(str[8])];
      case 8:
        hval += asso_values[(unsigned char)tolower(str[7])];
      case 7:
        hval += asso_values[(unsigned char)tolower(str[6])];
      case 6:
        hval += asso_values[(unsigned char)tolower(str[5])];
      case 5:
        hval += asso_values[(unsigned char)tolower(str[4])];
      case 4:
        hval += asso_values[(unsigned char)tolower(str[3])];
      case 3:
        hval += asso_values[(unsigned char)tolower(str[2])];
      case 2:
        hval += asso_values[(unsigned char)tolower(str[1])];
      case 1:
        hval += asso_values[(unsigned char)tolower(str[0])];
        break;
    }
  return hval;
}

struct headers *
HeaderHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 74,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 494
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"i", Headers::CallId},
      {"l", Headers::ContentLength},
      {"o", Headers::ContentType},
      {"require", Headers::Require},
      {"hide", Headers::UNKNOWN},
      {"s", Headers::Subject},
      {"t", Headers::To},
      {"warning", Headers::Warning},
      {"date", Headers::Date},
      {"rseq", Headers::UNKNOWN},
      {"v", Headers::Via},
      {"m", Headers::Contact},
      {"path", Headers::UNKNOWN},
      {"allow", Headers::Allow},
      {"reason", Headers::UNKNOWN},
      {"to", Headers::To},
      {"via", Headers::Via},
      {"route", Headers::Route},
      {"f", Headers::From},
      {"referred-by",Headers::ReferredBy},
      {"reply-to", Headers::ReplyTo},
      {"server", Headers::Server},
      {"priority", Headers::Priority},
      {"event", Headers::Event},
      {"in-reply-to", Headers::InReplyTo},
      {"response-key", Headers::UNKNOWN},
      {"supported", Headers::Supported},
      {"user-agent", Headers::UserAgent},
      {"unsupported", Headers::Unsupported},
      {"rack", Headers::UNKNOWN},
      {"expires", Headers::Expires},
      {"proxy-require", Headers::ProxyRequire},
      {"error-info", Headers::ErrorInfo},
      {"refer-to",Headers::ReferTo},
      {"organization", Headers::Organization},
      {"from", Headers::From},
      {"retry-after", Headers::RetryAfter},
      {"cseq", Headers::CSeq},
      {"call-id", Headers::CallId},
      {"alert-info",Headers::AlertInfo},
      {"replaces",Headers::Replaces},
      {"authorization", Headers::Authorization},
      {"privacy", Headers::UNKNOWN},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"min-expires", Headers::MinExpires},
      {"allow-events", Headers::AllowEvents},
      {"subject", Headers::Subject},
      {"encryption", Headers::UNKNOWN},
      {"record-route", Headers::RecordRoute},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"timestamp", Headers::Timestamp},
      {"p-media-authorization", Headers::UNKNOWN},
      {"mime-version", Headers::MIMEVersion},
      {"call-info", Headers::CallInfo},
      {"max-forwards", Headers::MaxForwards},
      {"content-type", Headers::ContentType},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"accept", Headers::Accept},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"content-language", Headers::ContentLanguage},
      {"security-server", Headers::SecurityServer},
      {"content-length", Headers::ContentLength},
      {"contact", Headers::Contact},
      {"security-verify", Headers::SecurityVerify},
      {"accept-language", Headers::AcceptLanguage},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"security-client", Headers::SecurityClient},
      {"subscription-state",Headers::SubscriptionState},
      {"content-encoding", Headers::ContentEncoding},
      {"authentication-info", Headers::AuthenticationInfo},
      {"content-disposition", Headers::ContentDisposition},
      {"accept-encoding", Headers::AcceptEncoding},
      {"content-transfer-encoding", Headers::ContentTransferEncoding}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1,
      -1, -1,  2, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1,  4,
      -1,  5, -1,  6, -1, -1, -1, -1,  7,  8, -1,  9, -1, -1,
      -1, -1, 10, -1, 11, -1, -1, -1, -1, 12, -1, -1, 13, 14,
      15, 16, 17, -1, 18, 19, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 20, -1, -1, -1, -1, -1, -1, 21, -1, -1, 22, -1, 23,
      -1, 24, -1, -1, -1, -1, -1, 25, 26, -1, 27, 28, 29, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 30, 31, -1, -1, -1, -1,
      -1, 32, -1, -1, -1, -1, -1, -1, 33, -1, -1, 34, -1, -1,
      -1, 35, -1, -1, -1, -1, -1, -1, 36, -1, -1, -1, -1, -1,
      -1, 37, -1, -1, -1, -1, 38, -1, -1, 39, 40, -1, -1, -1,
      -1, 41, -1, -1, 42, -1, -1, -1, -1, -1, 43, -1, -1, -1,
      -1, -1, 44, 45, 46, -1, 47, -1, -1, 48, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 49, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 50, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1,
      -1, -1, 52, 53, -1, -1, -1, 54, 55, -1, -1, -1, -1, -1,
      -1, 56, -1, -1, -1, -1, 57, 58, -1, -1, 59, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 60, -1, -1,
      -1, 61, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 63, -1, -1, -1, -1, -1, 64,
      -1, 65, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 66, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 67, 68, -1, -1, -1, -1, 69, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 70, -1, -1, -1, -1, 71, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      72, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 73
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;
              if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
                return &wordlist[index];
            }
        }
    }
  return 0;
}

Headers::Type
Headers::getType(const char* name, int len)
{
   struct headers* p;
   p = HeaderHash::in_word_set(name, len);
   return p ? Headers::Type(p->type) : Headers::UNKNOWN;
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
