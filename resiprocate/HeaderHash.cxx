/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 368, duplicates = 0 */

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
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369,   0, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369,  20, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369,   0,   0, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369,   0,   5,  20,
        0,   0,  65,  45,  70,  10,  10,  45,  15,   5,
        0,   0,   0,   0,   0,  30,  40,   0,  57,   0,
       75,  55,   0, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369
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
      TOTAL_KEYWORDS = 87,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 368
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"RSeq", Headers::RSeq},
      {"m", Headers::Contact},
      {"i", Headers::CallID},
      {"l", Headers::ContentLength},
      {"require", Headers::Require},
      {"c", Headers::ContentType},
      {"s", Headers::Subject},
      {"rseq", Headers::RSeq},
      {"allow", Headers::Allow},
      {"reason", Headers::UNKNOWN},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"k", Headers::Supported},
      {"min-se", Headers::MinSE},
      {"cseq", Headers::CSeq},
      {"y", Headers::Identity},
      {"v", Headers::Via},
      {"warning", Headers::Warning},
      {"f", Headers::From},
      {"call-id", Headers::CallID},
      {"rack", Headers::RAck},
      {"via", Headers::Via},
      {"record-route", Headers::RecordRoute},
      {"replaces",Headers::Replaces},
      {"from", Headers::From},
      {"x", Headers::SessionExpires},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"hide", Headers::UNKNOWN},
      {"error-info", Headers::ErrorInfo},
      {"accept", Headers::Accept},
      {"RAck", Headers::RAck},
      {"server", Headers::Server},
      {"event", Headers::Event},
      {"content", Headers::ContentId},
      {"subject", Headers::Subject},
      {"refer-to",Headers::ReferTo},
      {"path", Headers::UNKNOWN},
      {"organization", Headers::Organization},
      {"reply-to", Headers::ReplyTo},
      {"content-id", Headers::ContentId},
      {"expires", Headers::Expires},
      {"priority", Headers::Priority},
      {"user-agent", Headers::UserAgent},
      {"contact", Headers::Contact},
      {"mime-version", Headers::MIMEVersion},
      {"in-reply-to", Headers::InReplyTo},
      {"sip-etag", Headers::SIPETag},
      {"call-info", Headers::CallInfo},
      {"encryption", Headers::UNKNOWN},
      {"referred-by",Headers::ReferredBy},
      {"timestamp", Headers::Timestamp},
      {"alert-info",Headers::AlertInfo},
      {"min-expires", Headers::MinExpires},
      {"privacy", Headers::UNKNOWN},
      {"proxy-require", Headers::ProxyRequire},
      {"identity", Headers::Identity},
      {"allow-events", Headers::AllowEvents},
      {"accept-encoding", Headers::AcceptEncoding},
      {"response-key", Headers::UNKNOWN},
      {"authorization", Headers::Authorization},
      {"max-forwards", Headers::MaxForwards},
      {"content-encoding", Headers::ContentEncoding},
      {"accept-language", Headers::AcceptLanguage},
      {"p-media-authorization", Headers::UNKNOWN},
      {"content-type", Headers::ContentType},
      {"retry-after", Headers::RetryAfter},
      {"content-language", Headers::ContentLanguage},
      {"session-expires", Headers::SessionExpires},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"identity-info", Headers::IdentityInfo},
      {"content-disposition", Headers::ContentDisposition},
      {"security-client", Headers::SecurityClient},
      {"security-server", Headers::SecurityServer},
      {"sip-if-match", Headers::SIPIfMatch},
      {"subscription-state",Headers::SubscriptionState},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"content-length", Headers::ContentLength},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"authentication-info", Headers::AuthenticationInfo},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"security-verify", Headers::SecurityVerify},
      {"proxy-authenticate", Headers::ProxyAuthenticate}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1,  1, -1,  2, -1, -1, -1, -1,  3, -1, -1,
      -1, -1,  4,  5, -1, -1, -1,  6, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  7, -1, -1,  8,  9, 10, -1, -1, -1, -1, 11,
      12, -1, 13, 14, 15, -1, -1, -1, -1, 16, -1, -1, 17, -1,
      18, -1, 19, -1, -1, -1, 20, -1, -1, -1, 21, 22, -1, 23,
      24, -1, 25, 26, 27, -1, 28, -1, -1, 29, -1, 30, -1, -1,
      31, 32, 33, -1, -1, 34, -1, -1, -1, 35, -1, -1, -1, -1,
      -1, -1, -1, -1, 36, -1, -1, -1, -1, 37, -1, -1, -1, -1,
      38, 39, 40, -1, -1, 41, 42, -1, 43, -1, 44, 45, -1, 46,
      -1, 47, -1, 48, -1, 49, -1, 50, 51, 52, 53, -1, -1, 54,
      55, 56, -1, -1, -1, -1, -1, -1, -1, 57, -1, -1, -1, 58,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 59, -1, -1, -1, -1,
      -1, 60, 61, -1, 62, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 63, -1, -1, -1, 64, -1, -1, -1, 65, -1, -1, -1, -1,
      -1, -1, -1, -1, 66, -1, -1, -1, -1, -1, 67, 68, -1, -1,
      -1, 69, -1, -1, -1, -1, -1, -1, -1, -1, -1, 70, -1, -1,
      -1, -1, -1, -1, -1, -1, 71, -1, -1, -1, -1, -1, 72, -1,
      -1, -1, 73, -1, -1, 74, -1, -1, -1, -1, -1, 75, -1, -1,
      -1, -1, -1, 76, -1, 77, -1, -1, -1, -1, 78, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 79, 80, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 81, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 82, -1, -1,
      -1, -1, 83, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 84,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 85, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 86
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
