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
/* maximum key range = 383, duplicates = 0 */

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
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384,   0, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384,  20, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384,   0,   0, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384,   0,   5,  20,
        0,   0,  65,  45,  70,  10,   5,  45,  15,   5,
        0,   0,   0,   0,   0,  30,  40,   0,  75,   0,
       90,  55,   0, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384
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
      TOTAL_KEYWORDS = 85,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 383
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
      {"warning", Headers::Warning},
      {"f", Headers::From},
      {"call-id", Headers::CallID},
      {"rack", Headers::RAck},
      {"record-route", Headers::RecordRoute},
      {"replaces",Headers::Replaces},
      {"from", Headers::From},
      {"v", Headers::Via},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"hide", Headers::UNKNOWN},
      {"error-info", Headers::ErrorInfo},
      {"accept", Headers::Accept},
      {"via", Headers::Via},
      {"RAck", Headers::RAck},
      {"x", Headers::SessionExpires},
      {"subject", Headers::Subject},
      {"server", Headers::Server},
      {"refer-to",Headers::ReferTo},
      {"path", Headers::UNKNOWN},
      {"organization", Headers::Organization},
      {"reply-to", Headers::ReplyTo},
      {"event", Headers::Event},
      {"priority", Headers::Priority},
      {"user-agent", Headers::UserAgent},
      {"contact", Headers::Contact},
      {"in-reply-to", Headers::InReplyTo},
      {"sip-etag", Headers::SIPETag},
      {"call-info", Headers::CallInfo},
      {"encryption", Headers::UNKNOWN},
      {"referred-by",Headers::ReferredBy},
      {"expires", Headers::Expires},
      {"timestamp", Headers::Timestamp},
      {"alert-info",Headers::AlertInfo},
      {"mime-version", Headers::MIMEVersion},
      {"min-expires", Headers::MinExpires},
      {"identity", Headers::Identity},
      {"privacy", Headers::UNKNOWN},
      {"proxy-require", Headers::ProxyRequire},
      {"accept-encoding", Headers::AcceptEncoding},
      {"response-key", Headers::UNKNOWN},
      {"authorization", Headers::Authorization},
      {"allow-events", Headers::AllowEvents},
      {"content-encoding", Headers::ContentEncoding},
      {"accept-language", Headers::AcceptLanguage},
      {"max-forwards", Headers::MaxForwards},
      {"p-media-authorization", Headers::UNKNOWN},
      {"content-type", Headers::ContentType},
      {"retry-after", Headers::RetryAfter},
      {"content-language", Headers::ContentLanguage},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"identity-info", Headers::IdentityInfo},
      {"session-expires", Headers::SessionExpires},
      {"content-disposition", Headers::ContentDisposition},
      {"security-client", Headers::SecurityClient},
      {"sip-if-match", Headers::SIPIfMatch},
      {"subscription-state",Headers::SubscriptionState},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"security-server", Headers::SecurityServer},
      {"content-length", Headers::ContentLength},
      {"authentication-info", Headers::AuthenticationInfo},
      {"proxy-authorization", Headers::ProxyAuthorization},
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
      18, -1, -1, -1, -1, -1, 19, -1, -1, -1, 20, 21, -1, 22,
      -1, -1, 23, 24, 25, -1, 26, -1, -1, 27, -1, 28, -1, -1,
      29, 30, 31, -1, 32, 33, -1, 34, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1, -1, -1, 36,
      -1, 37, 38, -1, -1, 39, 40, -1, 41, -1, -1, 42, -1, 43,
      -1, 44, -1, -1, -1, 45, -1, 46, 47, 48, 49, 50, -1, 51,
      52, -1, -1, -1, -1, -1, -1, 53, -1, -1, -1, -1, -1, -1,
      -1, -1, 54, -1, -1, -1, -1, -1, -1, 55, -1, -1, -1, 56,
      57, -1, 58, -1, 59, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 60, -1, -1, -1, 61, -1, -1, -1, 62, -1, -1, -1, -1,
      -1, -1, -1, -1, 63, -1, 64, -1, -1, -1, 65, 66, -1, -1,
      -1, 67, -1, -1, -1, -1, -1, -1, -1, -1, -1, 68, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 69, -1,
      -1, -1, 70, -1, -1, 71, -1, 72, -1, -1, -1, 73, -1, -1,
      -1, -1, -1, 74, -1, -1, -1, -1, -1, -1, 75, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 76, 77, 78, -1, -1, -1, -1,
      -1, -1, -1, -1, 79, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 80, -1, -1, -1, -1, -1, -1, -1, -1, -1, 81, 82,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 83, -1, -1,
      -1, -1, -1, -1, -1, 84
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
