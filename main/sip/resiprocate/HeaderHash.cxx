/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf --enum -E -D -L C++ -t -k '*' -Z --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/sipstack/HeaderTypes.hxx"

namespace Vocal2 {
using namespace std;
using namespace Vocal2;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 504, duplicates = 0 */

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
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505,   0, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505,   0,   5,  20,
        0,   0,  25,  35,  15,  10,   0,   0,  45,   5,
        0,   0,   0,   5,   0, 112,  35,   0, 110,  10,
        0,   0,  10, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505, 505, 505, 505, 505,
      505, 505, 505, 505, 505, 505
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
      MAX_HASH_VALUE = 504
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"m", Headers::Contact},
      {"i", Headers::CallId},
      {"c", Headers::ContentType},
      {"require", Headers::Require},
      {"rack", Headers::UNKNOWN},
      {"f", Headers::From},
      {"proxy-require", Headers::ProxyRequire},
      {"hide", Headers::UNKNOWN},
      {"from", Headers::From},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"referred-by",Headers::ReferredBy},
      {"error-info", Headers::ErrorInfo},
      {"l", Headers::ContentLength},
      {"path", Headers::UNKNOWN},
      {"warning", Headers::Warning},
      {"priority", Headers::Priority},
      {"record-route", Headers::RecordRoute},
      {"refer-to",Headers::ReferTo},
      {"encryption", Headers::UNKNOWN},
      {"accept", Headers::Accept},
      {"reply-to", Headers::ReplyTo},
      {"in-reply-to", Headers::InReplyTo},
      {"allow", Headers::Allow},
      {"retry-after", Headers::RetryAfter},
      {"v", Headers::Via},
      {"organization", Headers::Organization},
      {"s", Headers::Subject},
      {"contact", Headers::Contact},
      {"reason", Headers::UNKNOWN},
      {"rseq", Headers::UNKNOWN},
      {"via", Headers::Via},
      {"alert-info",Headers::AlertInfo},
      {"call-id", Headers::CallId},
      {"authorization", Headers::Authorization},
      {"expires", Headers::Expires},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"content-type", Headers::ContentType},
      {"cseq", Headers::CSeq},
      {"privacy", Headers::UNKNOWN},
      {"min-expires", Headers::MinExpires},
      {"event", Headers::Event},
      {"p-media-authorization", Headers::UNKNOWN},
      {"call-info", Headers::CallInfo},
      {"accept-encoding", Headers::AcceptEncoding},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"max-forwards", Headers::MaxForwards},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"content-encoding", Headers::ContentEncoding},
      {"subject", Headers::Subject},
      {"replaces",Headers::Replaces},
      {"user-agent", Headers::UserAgent},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"accept-language", Headers::AcceptLanguage},
      {"timestamp", Headers::Timestamp},
      {"authentication-info", Headers::AuthenticationInfo},
      {"content-language", Headers::ContentLanguage},
      {"server", Headers::Server},
      {"content-length", Headers::ContentLength},
      {"response-key", Headers::UNKNOWN},
      {"mime-version", Headers::MIMEVersion},
      {"security-client", Headers::SecurityClient},
      {"security-verify", Headers::SecurityVerify},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"allow-events", Headers::AllowEvents},
      {"content-disposition", Headers::ContentDisposition},
      {"security-server", Headers::SecurityServer},
      {"subscription-state",Headers::SubscriptionState}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1,  1, -1, -1, -1, -1,  2, -1, -1,
      -1, -1, -1, -1, -1, -1, -1,  3,  4, -1,  5, -1,  6, -1,
       7,  8, -1, -1, -1, -1,  9, -1, 10, 11, -1, 12, 13, 14,
      -1, -1, -1, 15, 16, -1, -1, -1, -1, -1, -1, -1, 17, -1,
      -1, -1, -1, -1, -1, -1, 18, 19, -1, -1, -1, 20, 21, -1,
      -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1, 23, -1, -1,
      -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, 25, -1, -1, -1, 26, 27, -1, -1, -1, -1, 28,
      29, 30, -1, -1, -1, 31, 32, -1, -1, 33, -1, 34, -1, 35,
      -1, 36, 37, 38, -1, -1, -1, -1, 39, 40, -1, 41, -1, -1,
      -1, 42, -1, -1, -1, -1, -1, 43, 44, -1, 45, 46, -1, -1,
      47, 48, 49, -1, 50, -1, -1, -1, -1, -1, 51, -1, -1, -1,
      52, -1, -1, 53, -1, -1, -1, -1, -1, -1, -1, 54, -1, -1,
      -1, -1, -1, 55, -1, -1, -1, -1, -1, -1, 56, -1, -1, -1,
      57, -1, -1, -1, -1, -1, -1, -1, -1, 58, -1, -1, -1, -1,
      -1, 59, -1, -1, 60, -1, -1, -1, -1, -1, -1, 61, -1, -1,
      -1, -1, -1, -1, 62, -1, -1, -1, -1, -1, 63, -1, 64, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 65, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 66, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 67, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 68, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 69, 70, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, 71, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 72, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      73
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
}

