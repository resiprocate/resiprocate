/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
using namespace resip;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 325, duplicates = 0 */

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
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326,   0, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326,  15, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326,   0,   0, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326,   0,  15,  20,
        0,   0,  65,  35,  10,  10,  10,  45,  15,   5,
        0,   0,   0,   5,   0,  30,  40,   0, 105,   0,
       55,   0,   5, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326
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
      TOTAL_KEYWORDS = 82,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 325
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"m", Headers::Contact},
      {"RSeq", Headers::RSeq},
      {"i", Headers::CallID},
      {"l", Headers::ContentLength},
      {"c", Headers::ContentType},
      {"require", Headers::Require},
      {"hide", Headers::RESIP_UNKNOWN},
      {"s", Headers::Subject},
      {"allow", Headers::Allow},
      {"reason", Headers::RESIP_UNKNOWN},
      {"rseq", Headers::RSeq},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"k", Headers::Supported},
      {"min-se", Headers::MinSE},
      {"warning", Headers::Warning},
      {"path", Headers::RESIP_UNKNOWN},
      {"x", Headers::SessionExpires},
      {"cseq", Headers::CSeq},
      {"reply-to", Headers::ReplyTo},
      {"f", Headers::From},
      {"call-id", Headers::CallID},
      {"priority", Headers::Priority},
      {"rack", Headers::RAck},
      {"record-route", Headers::RecordRoute},
      {"replaces",Headers::Replaces},
      {"from", Headers::From},
      {"in-reply-to", Headers::InReplyTo},
      {"supported", Headers::Supported},
      {"encryption", Headers::RESIP_UNKNOWN},
      {"unsupported", Headers::Unsupported},
      {"proxy-require", Headers::ProxyRequire},
      {"RAck", Headers::RAck},
      {"error-info", Headers::ErrorInfo},
      {"accept", Headers::Accept},
      {"referred-by",Headers::ReferredBy},
      {"expires", Headers::Expires},
      {"v", Headers::Via},
      {"organization", Headers::Organization},
      {"refer-to",Headers::ReferTo},
      {"user-agent", Headers::UserAgent},
      {"response-key", Headers::RESIP_UNKNOWN},
      {"via", Headers::Via},
      {"min-expires", Headers::MinExpires},
      {"subject", Headers::Subject},
      {"sip-etag", Headers::SIPETag},
      {"contact", Headers::Contact},
      {"authorization", Headers::Authorization},
      {"call-info", Headers::CallInfo},
      {"timestamp", Headers::Timestamp},
      {"alert-info",Headers::AlertInfo},
      {"server", Headers::Server},
      {"privacy", Headers::RESIP_UNKNOWN},
      {"event", Headers::Event},
      {"p-media-authorization", Headers::RESIP_UNKNOWN},
      {"content-type", Headers::ContentType},
      {"retry-after", Headers::RetryAfter},
      {"accept-encoding", Headers::AcceptEncoding},
      {"max-forwards", Headers::MaxForwards},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"mime-version", Headers::MIMEVersion},
      {"accept-language", Headers::AcceptLanguage},
      {"content-encoding", Headers::ContentEncoding},
      {"p-preferred-identity", Headers::RESIP_UNKNOWN},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"security-client", Headers::SecurityClient},
      {"content-language", Headers::ContentLanguage},
      {"sip-if-match", Headers::SIPIfMatch},
      {"session-expires", Headers::SessionExpires},
      {"content-length", Headers::ContentLength},
      {"allow-events", Headers::AllowEvents},
      {"p-asserted-identity", Headers::RESIP_UNKNOWN},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"content-disposition", Headers::ContentDisposition},
      {"security-server", Headers::SecurityServer},
      {"authentication-info", Headers::AuthenticationInfo},
      {"subscription-state",Headers::SubscriptionState},
      {"security-verify", Headers::SecurityVerify},
      {"content-transfer-encoding", Headers::ContentTransferEncoding}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1,  1, -1, -1,  2, -1,  3, -1, -1,
      -1, -1,  4, -1, -1, -1, -1,  5,  6, -1,  7, -1, -1, -1,
      -1, -1, -1,  8, -1, -1, -1,  9, 10, -1, -1, 11, -1, 12,
      13, -1, 14, 15, 16, -1, -1, -1, -1, 17, 18, -1, 19, -1,
      20, -1, -1, 21, -1, -1, -1, 22, -1, -1, 23, 24, 25, 26,
      -1, -1, 27, 28, 29, -1, 30, -1, -1, 31, 32, 33, -1, 34,
      35, 36, 37, -1, -1, -1, -1, 38, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 39, -1, -1, -1, 40, -1, -1, -1, -1, -1,
      41, 42, -1, 43, -1, 44, 45, -1, -1, 46, 47, 48, -1, -1,
      -1, 49, 50, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, 52,
      53, 54, 55, -1, -1, -1, -1, -1, -1, -1, 56, 57, 58, -1,
      -1, -1, 59, -1, -1, -1, 60, -1, -1, -1, -1, -1, -1, 61,
      -1, -1, -1, -1, -1, -1, -1, -1, 62, 63, -1, -1, 64, 65,
      -1, -1, -1, 66, -1, -1, -1, 67, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 68, 69, 70, -1, -1, -1, -1, -1, -1, -1,
      71, -1, -1, -1, 72, -1, -1, 73, -1, 74, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 75, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 76, 77, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 78, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, 79, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 80, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, 81
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

              if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
}
