#include <util/Data.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;

Data Headers::HeaderNames[MAX_HEADERS] = {};
bool Headers::CommaTokenizing[] = {false};


Header<Headers::Content_Disposition> Vocal2::h_ContentDisposition;
Header<Headers::Content_Encoding> Vocal2::h_ContentEncoding;
Header<Headers::MIME_Version> Vocal2::h_MimeVersion;
Header<Headers::Priority> Vocal2::h_Priority;
MultiHeader<Headers::Accept_Encoding> Vocal2::h_AcceptEncodings;
MultiHeader<Headers::Accept_Language> Vocal2::h_AcceptLanguages;
MultiHeader<Headers::Allow> Vocal2::h_Allows;
MultiHeader<Headers::Content_Language> Vocal2::h_ContentLanguages;
MultiHeader<Headers::Proxy_Require> Vocal2::h_ProxyRequires;
MultiHeader<Headers::Require> Vocal2::h_Requires;
MultiHeader<Headers::Supported> Vocal2::h_Supporteds;
MultiHeader<Headers::Unsupported> Vocal2::h_Unsupporteds;
MultiHeader<Headers::Accept> Vocal2::h_Accepts;
Header<Headers::Content_Type> Vocal2::h_ContentType;
MultiHeader<Headers::Alert_Info> Vocal2::h_AlertInfos;
MultiHeader<Headers::Error_Info> Vocal2::h_ErrorInfos;
MultiHeader<Headers::Record_Route> Vocal2::h_RecordRoutes;
MultiHeader<Headers::Route> Vocal2::h_Routes;
MultiHeader<Headers::Contact> Vocal2::h_Contacts;
Header<Headers::From> Vocal2::h_From;
Header<Headers::Reply_To> Vocal2::h_ReplyTo;
Header<Headers::Refer_To> Vocal2::h_ReferTo;
Header<Headers::Referred_By> Vocal2::h_ReferredBy;
Header<Headers::To> Vocal2::h_To;
Header<Headers::Organization> Vocal2::h_Organization;
Header<Headers::Server> Vocal2::h_Server;
Header<Headers::Subject> Vocal2::h_Subject;
Header<Headers::User_Agent> Vocal2::h_UserAgent;
Header<Headers::Content_Length> Vocal2::h_ContentLength;
Header<Headers::Expires> Vocal2::h_Expires;
Header<Headers::Max_Forwards> Vocal2::h_MaxForwards;
Header<Headers::Min_Expires> Vocal2::h_MinExpires;
Header<Headers::Retry_After> Vocal2::h_RetryAfter;
Header<Headers::Call_ID> Vocal2::h_CallId;
Header<Headers::In_Reply_To> Vocal2::h_InReplyTo;
Header<Headers::Authentication_Info> Vocal2::h_AuthenticationInfo;
Header<Headers::Authorization> Vocal2::h_Authorization;
Header<Headers::Proxy_Authenticate> Vocal2::h_ProxyAuthenticate;
Header<Headers::Proxy_Authorization> Vocal2::h_ProxyAuthorization;
Header<Headers::WWW_Authenticate> Vocal2::h_WWWAuthenticate;
Header<Headers::CSeq> Vocal2::h_CSeq;
Header<Headers::Date> Vocal2::h_Date;
Header<Headers::Warning> Vocal2::h_Warning;
MultiHeader<Headers::Via> Vocal2::h_Vias;
MultiHeader<Headers::Subscription_State> Vocal2::h_SubscriptionStates;
Vocal2::RequestLineType Vocal2::h_RequestLine;
Vocal2::StatusLineType Vocal2::h_StatusLine;
Header<Headers::Call_ID> Vocal2::h_Replaces;

bool
Headers::isCommaTokenizing(Type type)
{
   return CommaTokenizing[type];
}

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' headers.gperf  */
struct params { char *name; Headers::Type type; };

#define TOTAL_KEYWORDS 48
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 19
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 162
/* maximum key range = 161, duplicates = 0 */

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
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163,   0, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163,   0,   0,   0,   5,   0,
       60,   0,   0,   0,  15, 163,  30,  30,   0,   0,
        0,  35,   0,   0,   0,   0,  25,  30,  25,  15,
        0, 163, 163, 163, 163,  15, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163, 163, 163, 163, 163,
      163, 163, 163, 163, 163, 163
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 19:
        hval += asso_values[(unsigned char)str[18] & 0xDF ];
      case 18:
        hval += asso_values[(unsigned char)str[17] & 0xDF ];
      case 17:
        hval += asso_values[(unsigned char)str[16] & 0xDF ];
      case 16:
        hval += asso_values[(unsigned char)str[15] & 0xDF ];
      case 15:
        hval += asso_values[(unsigned char)str[14] & 0xDF ];
      case 14:
        hval += asso_values[(unsigned char)str[13] & 0xDF ];
      case 13:
        hval += asso_values[(unsigned char)str[12] & 0xDF ];
      case 12:
        hval += asso_values[(unsigned char)str[11] & 0xDF ];
      case 11:
        hval += asso_values[(unsigned char)str[10] & 0xDF ];
      case 10:
        hval += asso_values[(unsigned char)str[9] & 0xDF ];
      case 9:
        hval += asso_values[(unsigned char)str[8] & 0xDF ];
      case 8:
        hval += asso_values[(unsigned char)str[7] & 0xDF ];
      case 7:
        hval += asso_values[(unsigned char)str[6] & 0xDF ];
      case 6:
        hval += asso_values[(unsigned char)str[5] & 0xDF ];
      case 5:
        hval += asso_values[(unsigned char)str[4] & 0xDF ];
      case 4:
        hval += asso_values[(unsigned char)str[3] & 0xDF ];
      case 3:
        hval += asso_values[(unsigned char)str[2] & 0xDF ];
      case 2:
        hval += asso_values[(unsigned char)str[1] & 0xDF ];
      case 1:
        hval += asso_values[(unsigned char)str[0] & 0xDF ];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
struct params *
in_word_set (register const char *str, register unsigned int len)
{
  static struct params wordlist[] =
    {
      {""}, {""},
      {"TO", Headers::To},
      {""}, {""},
      {"ROUTE", Headers::Route},
      {"ACCEPT", Headers::Accept},
      {"CONTACT", Headers::Contact},
      {""},
      {"DATE", Headers::Date},
      {"USER-AGENT", Headers::User_Agent},
      {""},
      {"ORGANIZATION", Headers::Organization},
      {"AUTHORIZATION", Headers::Authorization},
      {"SUPPORTED", Headers::Supported},
      {""},
      {"UNSUPPORTED", Headers::Unsupported},
      {"RECORD-ROUTE", Headers::Record_Route},
      {"SUBSCRIPTION-STATE",Headers::Subscription_State},
      {""},
      {"ACCEPT-ENCODING", Headers::Accept_Encoding},
      {"CONTENT-ENCODING", Headers::Content_Encoding},
      {"SUBJECT", Headers::Subject},
      {"PRIORITY", Headers::Priority},
      {"CONTENT-DISPOSITION", Headers::Content_Disposition},
      {""}, {""},
      {"CONTENT-TYPE", Headers::Content_Type},
      {"VIA", Headers::Via},
      {""}, {""},
      {"SERVER", Headers::Server},
      {"EXPIRES", Headers::Expires},
      {""}, {""}, {""}, {""},
      {"WARNING", Headers::Warning},
      {"REPLACES",Headers::Replaces},
      {"CSEQ", Headers::CSeq},
      {""}, {""},
      {"REQUIRE", Headers::Require},
      {""},
      {"CONTENT-LENGTH", Headers::Content_Length},
      {"ACCEPT-LANGUAGE", Headers::Accept_Language},
      {"CONTENT-LANGUAGE", Headers::Content_Language},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"REPLY-TO", Headers::Reply_To},
      {""}, {""}, {""}, {""},
      {"PROXY-AUTHENTICATE", Headers::Proxy_Authenticate},
      {"PROXY-AUTHORIZATION", Headers::Proxy_Authorization},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"MIN-EXPIRES", Headers::Min_Expires},
      {""},
      {"REFER-TO",Headers::Refer_To},
      {"TIMESTAMP", Headers::Timestamp},
      {"ERROR-INFO", Headers::Error_Info},
      {"IN-REPLY_TO", Headers::In_Reply_To},
      {"CALL-ID", Headers::Call_ID},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"AUTHENTICATION-INFO", Headers::Authentication_Info},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"RETRY-AFTER", Headers::Retry_After},
      {""},
      {"PROXY-REQUIRE", Headers::Proxy_Require},
      {""}, {""},
      {"REFERRED-BY",Headers::Referred_By},
      {""}, {""},
      {"FROM", Headers::From},
      {"ALLOW", Headers::Allow},
      {""},
      {"MIME-VERSION", Headers::MIME_Version},
      {""}, {""},
      {"ALERT-INFO",Headers::Alert_Info},
      {""}, {""}, {""}, {""}, {""},
      {"WWW-AUTHENTICATE",Headers::WWW_Authenticate},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
      {"CALL-INFO", Headers::Call_Info},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"MAX-FORWARDS", Headers::Max_Forwards}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (( (*str) & 0xDF) == *s && !strcasecmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}



// to generate the perfect hash:
// gperf -L ANSI-C -t -k '*' headers.gperf > bar
// also needed to bitwise and all char comparisons with 0xDF to force to
// uppercase. All names in headers.gperf must be uppercased
// will NOT work for non alpha chars 
// 
Headers::Type
Headers::getHeaderType(const char* name, int len)
{
   struct params* p;
   p = in_word_set(name, len);
   return p ? Headers::Type(p->type) : Headers::UNKNOWN;
}


