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
/* maximum key range = 305, duplicates = 0 */

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
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306,   0, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306,   0,  25,  20,
        0,   0,  55,  25,  15,  10,   0,  60,  15,   5,
        0,   0,   0,   0,   0,  30,  40,   0,  70,  15,
        0,   0,  15, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306, 306, 306, 306, 306,
      306, 306, 306, 306, 306, 306
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
      TOTAL_KEYWORDS = 75,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 305
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"m", Headers::Contact},
      {"i", Headers::CallId},
      {"l", Headers::ContentLength},
      {"require", Headers::Require},
      {"c", Headers::ContentType},
      {"proxy-require", Headers::ProxyRequire},
      {"hide", Headers::UNKNOWN},
      {"s", Headers::Subject},
      {"rseq", Headers::UNKNOWN},
      {"reason", Headers::UNKNOWN},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"expires", Headers::Expires},
      {"allow", Headers::Allow},
      {"cseq", Headers::CSeq},
      {"f", Headers::From},
      {"warning", Headers::Warning},
      {"path", Headers::UNKNOWN},
      {"k", Headers::Supported},
      {"reply-to", Headers::ReplyTo},
      {"from", Headers::From},
      {"min-expires", Headers::MinExpires},
      {"call-id", Headers::CallId},
      {"priority", Headers::Priority},
      {"v", Headers::Via},
      {"record-route", Headers::RecordRoute},
      {"replaces",Headers::Replaces},
      {"error-info", Headers::ErrorInfo},
      {"in-reply-to", Headers::InReplyTo},
      {"supported", Headers::Supported},
      {"encryption", Headers::UNKNOWN},
      {"unsupported", Headers::Unsupported},
      {"via", Headers::Via},
      {"rack", Headers::UNKNOWN},
      {"accept", Headers::Accept},
      {"referred-by",Headers::ReferredBy},
      {"refer-to",Headers::ReferTo},
      {"user-agent", Headers::UserAgent},
      {"server", Headers::Server},
      {"privacy", Headers::UNKNOWN},
      {"organization", Headers::Organization},
      {"event", Headers::Event},
      {"max-forwards", Headers::MaxForwards},
      {"subject", Headers::Subject},
      {"call-info", Headers::CallInfo},
      {"contact", Headers::Contact},
      {"alert-info",Headers::AlertInfo},
      {"response-key", Headers::UNKNOWN},
      {"timestamp", Headers::Timestamp},
      {"mime-version", Headers::MIMEVersion},
      {"authorization", Headers::Authorization},
      {"retry-after", Headers::RetryAfter},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"accept-encoding", Headers::AcceptEncoding},
      {"content-type", Headers::ContentType},
      {"accept-language", Headers::AcceptLanguage},
      {"p-media-authorization", Headers::UNKNOWN},
      {"content-encoding", Headers::ContentEncoding},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"content-language", Headers::ContentLanguage},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"allow-events", Headers::AllowEvents},
      {"security-client", Headers::SecurityClient},
      {"content-length", Headers::ContentLength},
      {"security-server", Headers::SecurityServer},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"content-disposition", Headers::ContentDisposition},
      {"security-verify", Headers::SecurityVerify},
      {"authentication-info", Headers::AuthenticationInfo},
      {"subscription-state",Headers::SubscriptionState},
      {"content-transfer-encoding", Headers::ContentTransferEncoding}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1,  1, -1, -1, -1, -1,  2, -1, -1,
      -1, -1,  3,  4, -1, -1, -1,  5, -1,  6, -1, -1, -1, -1,
      -1,  7, -1,  8, -1, -1,  9, -1, 10, -1, -1, -1, -1, 11,
      12, -1, 13, 14, -1, 15, -1, -1, 16, -1, -1, -1, 17, -1,
      18, 19, -1, 20, -1, 21, -1, 22, 23, -1, 24, 25, 26, -1,
      -1, 27, 28, 29, -1, 30, 31, -1, -1, 32, 33, 34, -1, 35,
      36, -1, 37, -1, -1, -1, -1, 38, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 39, -1, 40, 41, 42, -1, -1, -1, -1,
      43, -1, -1, 44, -1, 45, -1, -1, -1, -1, 46, -1, 47, -1,
      -1, 48, -1, -1, 49, -1, 50, -1, -1, -1, -1, -1, -1, 51,
      -1, -1, 52, 53, -1, -1, 54, -1, -1, 55, 56, -1, 57, -1,
      -1, -1, -1, -1, -1, -1, 58, -1, -1, -1, -1, -1, 59, -1,
      -1, -1, -1, 60, -1, -1, -1, 61, -1, -1, -1, -1, -1, 62,
      -1, 63, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 64, -1, -1, 65, -1, -1, -1, -1, -1, -1, -1, -1, 66,
      -1, -1, -1, -1, -1, 67, -1, -1, -1, 68, -1, -1, -1, -1,
      -1, -1, 69, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 70, 71, -1,
      -1, -1, -1, -1, -1, -1, -1, 72, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 73,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74
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

               if (tolower(*str) == *s && !strncasecmp( str + 1, s + 1, len - 1 ))
                return &wordlist[index];
            }
        }
    }
  return 0;
}
}
