#include "sip2/util/Data.hxx"
#include "sip2/sipstack/Headers.hxx"
#include "sip2/sipstack/Symbols.hxx"

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

RequestLineType Vocal2::h_RequestLine;
StatusLineType Vocal2::h_StatusLine;

Content_Disposition_Header Vocal2::h_ContentDisposition;
Content_Encoding_Header Vocal2::h_ContentEncoding;
MIME_Version_Header Vocal2::h_MimeVersion;
Priority_Header Vocal2::h_Priority;
Allow_Events_MultiHeader Vocal2::h_AllowEvents;
Accept_Encoding_MultiHeader Vocal2::h_AcceptEncodings;
Accept_Language_MultiHeader Vocal2::h_AcceptLanguages;
Allow_MultiHeader Vocal2::h_Allows;
Content_Language_MultiHeader Vocal2::h_ContentLanguages;
Proxy_Require_MultiHeader Vocal2::h_ProxyRequires;
Require_MultiHeader Vocal2::h_Requires;
Supported_MultiHeader Vocal2::h_Supporteds;
Timestamp_Header Vocal2::h_Timestamp;
Unsupported_MultiHeader Vocal2::h_Unsupporteds;
Accept_MultiHeader Vocal2::h_Accepts;
Content_Type_Header Vocal2::h_ContentType;
Alert_Info_MultiHeader Vocal2::h_AlertInfos;
Call_Info_MultiHeader Vocal2::h_CallInfos;
Error_Info_MultiHeader Vocal2::h_ErrorInfos;
Record_Route_MultiHeader Vocal2::h_RecordRoutes;
Route_MultiHeader Vocal2::h_Routes;
Contact_MultiHeader Vocal2::h_Contacts;
From_Header Vocal2::h_From;
Reply_To_Header Vocal2::h_ReplyTo;
Refer_To_Header Vocal2::h_ReferTo;
Referred_By_Header Vocal2::h_ReferredBy;
To_Header Vocal2::h_To;
Organization_Header Vocal2::h_Organization;
Server_Header Vocal2::h_Server;
Subject_Header Vocal2::h_Subject;
User_Agent_Header Vocal2::h_UserAgent;
Content_Length_Header Vocal2::h_ContentLength;
Expires_Header Vocal2::h_Expires;
Max_Forwards_Header Vocal2::h_MaxForwards;
Min_Expires_Header Vocal2::h_MinExpires;
Retry_After_Header Vocal2::h_RetryAfter;
Call_ID_Header Vocal2::h_CallId;
In_Reply_To_Header Vocal2::h_InReplyTo;
Authentication_Info_Header Vocal2::h_AuthenticationInfo;
Authorization_MultiHeader Vocal2::h_Authorizations;
Proxy_Authenticate_MultiHeader Vocal2::h_ProxyAuthenticates;
Proxy_Authorization_MultiHeader Vocal2::h_ProxyAuthorizations;
WWW_Authenticate_MultiHeader Vocal2::h_WWWAuthenticates;
CSeq_Header Vocal2::h_CSeq;
Date_Header Vocal2::h_Date;
Warning_Header Vocal2::h_Warning;
Via_MultiHeader Vocal2::h_Vias;
Subscription_State_MultiHeader Vocal2::h_SubscriptionStates;
Replaces_Header Vocal2::h_Replaces;
Event_Header Vocal2::h_Event;
Security_Client_MultiHeader Vocal2::h_SecurityClients;
Security_Server_MultiHeader Vocal2::h_SecurityServers;
Security_Verify_MultiHeader Vocal2::h_SecurityVerifies;
// to generate the perfect hash:
// gperf -L ANSI-C -t -k '*' headers.gperf > bar
// call tolower() on instances of the source string
// change strcmp to strncasecmp and pass len-1
// will NOT work for non alphanum chars 
/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' headers.gperf  */
struct headers { char *name; Headers::Type type; };

#define TOTAL_KEYWORDS 64
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 21
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 232
/* maximum key range = 231, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233,   0, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233,   0,   0,  54,
       20,   0,  15,  35,  10,   0,  15,   0,  30,  55,
        0,   0,   0,  45,   0,   0,   0,   0,  10,  20,
       30,   0,  50, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
      233, 233, 233, 233, 233, 233
    };
  register int hval = len;

  switch (hval)
    {
      default:
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

#ifdef __GNUC__
__inline
#endif

static struct headers *
in_word_set (register const char *str, register unsigned int len)
{
  static struct headers wordlist[] =
    {
      {""}, {""},
      {"to", Headers::To},
      {""}, {""},
      {"route", Headers::Route},
      {"reason", Headers::UNKNOWN},
      {""},
      {"priority", Headers::Priority},
      {""}, {""}, {""},
      {"response-key", Headers::UNKNOWN},
      {"via", Headers::Via},
      {"path", Headers::UNKNOWN},
      {"event", Headers::Event},
      {"server", Headers::Server},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"refer-to",Headers::Refer_To},
      {"date", Headers::Date},
      {"error-info", Headers::Error_Info},
      {"retry-after", Headers::Retry_After},
      {""}, {""},
      {"supported", Headers::Supported},
      {""},
      {"unsupported", Headers::Unsupported},
      {""}, {""},
      {"hide", Headers::UNKNOWN},
      {""}, {""},
      {"expires", Headers::Expires},
      {"reply-to", Headers::Reply_To},
      {""}, {""},
      {"in-reply-to", Headers::In_Reply_To},
      {""}, {""}, {""},
      {"user-agent", Headers::User_Agent},
      {"referred-by",Headers::Referred_By},
      {""}, {""},
      {"rseq", Headers::UNKNOWN},
      {""}, {""},
      {"require", Headers::Require},
      {""}, {""},
      {"alert-info",Headers::Alert_Info},
      {""}, {""},
      {"rack", Headers::UNKNOWN},
      {"p-asserted-identity", Headers::UNKNOWN},
      {""}, {""},
      {"warning", Headers::Warning},
      {""},
      {"encryption", Headers::UNKNOWN},
      {""},
      {"content-type", Headers::Content_Type},
      {""}, {""}, {""}, {""},
      {"privacy", Headers::UNKNOWN},
      {"subscription-state",Headers::Subscription_State},
      {"authorization", Headers::Authorization},
      {"from", Headers::From},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"subject", Headers::Subject},
      {""}, {""},
      {"security-server", Headers::Security_Server},
      {""}, {""}, {""}, {""}, {""},
      {"allow", Headers::Allow},
      {"record-route", Headers::Record_Route},
      {""},
      {"proxy-require", Headers::Proxy_Require},
      {""}, {""}, {""},
      {"replaces",Headers::Replaces},
      {"content-disposition", Headers::Content_Disposition},
      {"security-verify", Headers::Security_Verify},
      {""},
      {"min-expires", Headers::Min_Expires},
      {"organization", Headers::Organization},
      {"authentication-info", Headers::Authentication_Info},
      {""}, {""}, {""},
      {"allow-events", Headers::Allow_Events},
      {"cseq", Headers::CSeq},
      {""}, {""}, {""}, {""}, {""},
      {"proxy-authorization", Headers::Proxy_Authorization},
      {""}, {""},
      {"proxy-authenticate", Headers::Proxy_Authenticate},
      {""},
      {"accept", Headers::Accept},
      {"contact", Headers::Contact},
      {""}, {""}, {""},
      {"timestamp", Headers::Timestamp},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
      {"mime-version", Headers::MIME_Version},
      {""}, {""}, {""}, {""}, {""},
      {"call-info", Headers::Call_Info},
      {""},
      {"www-authenticate",Headers::WWW_Authenticate},
      {"call-id", Headers::Call_ID},
      {""},
      {"content-length", Headers::Content_Length},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"max-forwards", Headers::Max_Forwards},
      {"security-client", Headers::Security_Client},
      {""}, {""},
      {"p-media-authorization", Headers::UNKNOWN},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
      {"content-language", Headers::Content_Language},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"content-encoding", Headers::Content_Encoding},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"accept-language", Headers::Accept_Language},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"accept-encoding", Headers::Accept_Encoding}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
            return &wordlist[key];
        }
    }
  return 0;
}

Headers::Type
Headers::getType(const char* name, int len)
{
   struct headers* p;
   p = in_word_set(name, len);
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
