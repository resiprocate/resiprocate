#include <util/Data.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <sipstack/Symbols.hxx>

int strcasecmp(const char*, const char*);
int strncasecmp(const char*, const char*, int len);

using namespace Vocal2;

Data Headers::HeaderNames[MAX_HEADERS];
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
Header<Headers::Timestamp> Vocal2::h_Timestamp;
MultiHeader<Headers::Unsupported> Vocal2::h_Unsupporteds;
MultiHeader<Headers::Accept> Vocal2::h_Accepts;
Header<Headers::Content_Type> Vocal2::h_ContentType;
MultiHeader<Headers::Alert_Info> Vocal2::h_AlertInfos;
MultiHeader<Headers::Call_Info> Vocal2::h_CallInfos;
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
Header<Headers::Replaces> Vocal2::h_Replaces;

bool
Headers::isCommaTokenizing(Type type)
{
   return CommaTokenizing[type];
}


#ifndef WIN32
// !ah! We might not want this
Headers::Type& Vocal2::operator++(Headers::Type& t)
{
   t = static_cast<Headers::Type>(t + 1);
   return t;
}
#endif

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '3,$' headers.gperf  */
struct params { char *name; Headers::Type type; };

#define TOTAL_KEYWORDS 48
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 19
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 96
/* maximum key range = 95, duplicates = 0 */

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
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 20, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97,  0, 20,  0,
      25,  0,  0, 25,  0, 25, 97, 97,  0, 30,
      30,  0, 20, 16, 45, 46, 20, 20, 97,  0,
      10,  0, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97, 97, 97, 97, 97,
      97, 97, 97, 97, 97, 97
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 3:
        hval += asso_values[tolower((unsigned char)str[2])];
      case 2:
        break;
    }
  return hval + asso_values[tolower((unsigned char)str[len - 1])];
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
      {"to", Headers::To},
      {"via", Headers::Via},
      {""},
      {"allow", Headers::Allow},
      {""}, {""},
      {"refer-to",Headers::Refer_To},
      {"call-info", Headers::Call_Info},
      {"alert-info",Headers::Alert_Info},
      {"referred-by",Headers::Referred_By},
      {"record-route", Headers::Record_Route},
      {"proxy-require", Headers::Proxy_Require},
      {""},
      {"accept-language", Headers::Accept_Language},
      {"www-authenticate",Headers::WWW_Authenticate},
      {""},
      {"proxy-authenticate", Headers::Proxy_Authenticate},
      {""},
      {"cseq", Headers::CSeq},
      {""}, {""},
      {"require", Headers::Require},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"accept", Headers::Accept},
      {""},
      {"reply-to", Headers::Reply_To},
      {""},
      {"user-agent", Headers::User_Agent},
      {"in-reply-to", Headers::In_Reply_To},
      {"call-id", Headers::Call_ID},
      {"priority", Headers::Priority},
      {"from", Headers::From},
      {""}, {""}, {""},
      {"subscription-state",Headers::Subscription_State},
      {"authentication-info", Headers::Authentication_Info},
      {"accept-encoding", Headers::Accept_Encoding},
      {""},
      {"content-type", Headers::Content_Type},
      {""},
      {"content-length", Headers::Content_Length},
      {""},
      {"content-language", Headers::Content_Language},
      {"subject", Headers::Subject},
      {""},
      {"proxy-authorization", Headers::Proxy_Authorization},
      {""}, {""}, {""}, {""},
      {"supported", Headers::Supported},
      {"error-info", Headers::Error_Info},
      {""},
      {"contact", Headers::Contact},
      {""},
      {"timestamp", Headers::Timestamp},
      {""}, {""}, {""},
      {"authorization", Headers::Authorization},
      {""}, {""}, {""},
      {"organization", Headers::Organization},
      {"max-forwards", Headers::Max_Forwards},
      {""}, {""},
      {"content-encoding", Headers::Content_Encoding},
      {"mime-version", Headers::MIME_Version},
      {"expires", Headers::Expires},
      {"replaces",Headers::Replaces},
      {""},
      {"retry-after", Headers::Retry_After},
      {"warning", Headers::Warning},
      {""},
      {"content-disposition", Headers::Content_Disposition},
      {""}, {""},
      {"unsupported", Headers::Unsupported},
      {""}, {""}, {""}, {""},
      {"min-expires", Headers::Min_Expires},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"server", Headers::Server}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (tolower(*str) == *s && !strcasecmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}

// to generate the perfect hash:
// gperf -L ANSI-C -t -k '*' headers.gperf > bar
// also needed to call tolower() on instances of the source string
// chars. Also change strcmp to strcasecmp
// will NOT work for non alpha chars 
// 
Headers::Type
Headers::getHeaderType(const char* name, int len)
{
   struct params* p;
   p = in_word_set(name, len);
   return p ? Headers::Type(p->type) : Headers::UNKNOWN;
}


