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

// to generate the perfect hash:
// gperf -L ANSI-C -t -k '*' headers.gperf > bar
// call tolower() on instances of the source string
// change strcmp to strncasecmp and pass len-1
// will NOT work for non alphanum chars 

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' headers.gperf  */
struct params { char *name; Headers::Type type; };

#define TOTAL_KEYWORDS 48
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 19
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 157
/* maximum key range = 156, duplicates = 0 */

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
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158,   0, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158,   0,  25,   0,
         5,   0,  50,   0,   5,   0,  15, 158,  30,  35,
         0,   0,   0,  35,   0,   0,   0,   0,  25,  30,
         25,  15,   0, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
         158, 158, 158, 158, 158, 158
      };
   register int hval = len;

   switch (hval)
   {
      default:
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
struct params *
h_in_word_set (register const char *str, register unsigned int len)
{
   static struct params wordlist[] =
      {
         {""}, {""},
         {"to", Headers::To},
         {""}, {""},
         {"route", Headers::Route},
         {"accept", Headers::Accept},
         {"contact", Headers::Contact},
         {""},
         {"date", Headers::Date},
         {"user-agent", Headers::User_Agent},
         {""},
         {"organization", Headers::Organization},
         {""},
         {"supported", Headers::Supported},
         {""},
         {"unsupported", Headers::Unsupported},
         {"record-route", Headers::Record_Route},
         {"authorization", Headers::Authorization},
         {""},
         {"accept-encoding", Headers::Accept_Encoding},
         {"content-encoding", Headers::Content_Encoding},
         {""},
         {"priority", Headers::Priority},
         {"content-disposition", Headers::Content_Disposition},
         {""}, {""},
         {"content-type", Headers::Content_Type},
         {"via", Headers::Via},
         {""}, {""},
         {"server", Headers::Server},
         {"expires", Headers::Expires},
         {""}, {""}, {""}, {""},
         {"warning", Headers::Warning},
         {"replaces",Headers::Replaces},
         {"cseq", Headers::CSeq},
         {""}, {""},
         {"require", Headers::Require},
         {"subscription-state",Headers::Subscription_State},
         {""},
         {"accept-language", Headers::Accept_Language},
         {"content-language", Headers::Content_Language},
         {"subject", Headers::Subject},
         {""},
         {"content-length", Headers::Content_Length},
         {""}, {""}, {""},
         {"reply-to", Headers::Reply_To},
         {""}, {""},
         {"in-reply-to", Headers::In_Reply_To},
         {""},
         {"refer-to",Headers::Refer_To},
         {""},
         {"error-info", Headers::Error_Info},
         {""}, {""},
         {"proxy-authenticate", Headers::Proxy_Authenticate},
         {"proxy-authorization", Headers::Proxy_Authorization},
         {""}, {""}, {""}, {""}, {""}, {""},
         {"min-expires", Headers::Min_Expires},
         {"call-id", Headers::Call_ID},
         {""},
         {"authentication-info", Headers::Authentication_Info},
         {""},
         {"retry-after", Headers::Retry_After},
         {""}, {""},
         {"timestamp", Headers::Timestamp},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {"proxy-require", Headers::Proxy_Require},
         {"from", Headers::From},
         {"alert-info",Headers::Alert_Info},
         {""}, {""}, {""}, {""},
         {"allow", Headers::Allow},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {""},
         {"referred-by",Headers::Referred_By},
         {"mime-version", Headers::MIME_Version},
         {""}, {""}, {""},
         {"www-authenticate",Headers::WWW_Authenticate},
         {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {"call-info", Headers::Call_Info},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
         {""},
         {"max-forwards", Headers::Max_Forwards}
      };

   if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
   {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
      {
         register const char *s = wordlist[key].name;

         if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
         {
            return &wordlist[key];
         }
      }
   }
   return 0;
}

Headers::Type
Headers::getType(const char* name, int len)
{
   struct params* p;
   p = h_in_word_set(name, len);
   return p ? Headers::Type(p->type) : Headers::UNKNOWN;
}


