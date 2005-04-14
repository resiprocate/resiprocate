/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 490, duplicates = 0 */

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
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491,   0, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491,   0, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491,   0,   0, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491,   0,  55,  20,
        0,   0,  75,  55,  35,  10,  10,  35,  15,  25,
        0,   0,  40,   0,  50,  30,  40,   0,  45,  15,
       60,  65,   0, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491, 491, 491, 491, 491,
      491, 491, 491, 491, 491, 491
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
      TOTAL_KEYWORDS = 89,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 490
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"RSeq", Headers::RSeq},
      {"i", Headers::CallID},
      {"l", Headers::ContentLength},
      {"c", Headers::ContentType},
      {"m", Headers::Contact},
      {"s", Headers::Subject},
      {"k", Headers::Supported},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"v", Headers::Via},
      {"hide", Headers::UNKNOWN},
      {"allow", Headers::Allow},
      {"r", Headers::ReferTo},
      {"cseq", Headers::CSeq},
      {"b", Headers::ReferredBy},
      {"via", Headers::Via},
      {"RAck", Headers::RAck},
      {"x", Headers::SessionExpires},
      {"y", Headers::Identity},
      {"call-id", Headers::CallID},
      {"min-se", Headers::MinSE},
      {"f", Headers::From},
      {"rseq", Headers::RSeq},
      {"reason", Headers::UNKNOWN},
      {"event", Headers::Event},
      {"route", Headers::Route},
      {"content", Headers::ContentId},
      {"rack", Headers::RAck},
      {"require", Headers::Require},
      {"path", Headers::UNKNOWN},
      {"content-id", Headers::ContentId},
      {"accept", Headers::Accept},
      {"contact", Headers::Contact},
      {"warning", Headers::Warning},
      {"call-info", Headers::CallInfo},
      {"from", Headers::From},
      {"subject", Headers::Subject},
      {"replaces",Headers::Replaces},
      {"allow-events", Headers::AllowEvents},
      {"identity", Headers::Identity},
      {"organization", Headers::Organization},
      {"server", Headers::Server},
      {"sip-etag", Headers::SIPETag},
      {"user-agent", Headers::UserAgent},
      {"expires", Headers::Expires},
      {"authorization", Headers::Authorization},
      {"alert-info",Headers::AlertInfo},
      {"content-encoding", Headers::ContentEncoding},
      {"mime-version", Headers::MIMEVersion},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"reply-to", Headers::ReplyTo},
      {"timestamp", Headers::Timestamp},
      {"accept-encoding", Headers::AcceptEncoding},
      {"record-route", Headers::RecordRoute},
      {"refer-to",Headers::ReferTo},
      {"in-reply-to", Headers::InReplyTo},
      {"encryption", Headers::UNKNOWN},
      {"min-expires", Headers::MinExpires},
      {"privacy", Headers::UNKNOWN},
      {"content-language", Headers::ContentLanguage},
      {"error-info", Headers::ErrorInfo},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"content-type", Headers::ContentType},
      {"content-length", Headers::ContentLength},
      {"accept-language", Headers::AcceptLanguage},
      {"response-key", Headers::UNKNOWN},
      {"identity-info", Headers::IdentityInfo},
      {"priority", Headers::Priority},
      {"p-media-authorization", Headers::UNKNOWN},
      {"content-disposition", Headers::ContentDisposition},
      {"sip-if-match", Headers::SIPIfMatch},
      {"authentication-info", Headers::AuthenticationInfo},
      {"session-expires", Headers::SessionExpires},
      {"security-client", Headers::SecurityClient},
      {"max-forwards", Headers::MaxForwards},
      {"proxy-require", Headers::ProxyRequire},
      {"referred-by",Headers::ReferredBy},
      {"p-asserted-identity", Headers::UNKNOWN},
      {"retry-after", Headers::RetryAfter},
      {"security-server", Headers::SecurityServer},
      {"subscription-state",Headers::SubscriptionState},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"security-verify", Headers::SecurityVerify},
      {"p-preferred-identity", Headers::UNKNOWN}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1,  1, -1, -1, -1, -1, -1, -1,  2, -1, -1,
      -1, -1,  3, -1, -1, -1, -1,  4, -1, -1, -1, -1,  5, -1,
      -1, -1, -1,  6, -1, -1, -1, -1,  7, -1, -1, -1, -1,  8,
       9, -1, 10, -1, 11, -1, -1, 12, 13, 14, -1, -1, 15, -1,
      16, -1, 17, 18, -1, 19, -1, -1, -1, -1, 20, 21, -1, -1,
      -1, 22, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1,
      24, -1, 25, -1, -1, -1, 26, -1, -1, -1, -1, 27, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 28, -1, 29, -1, -1,
      -1, -1, -1, -1, -1, 30, -1, 31, 32, -1, -1, -1, -1, -1,
      33, 34, -1, -1, -1, -1, -1, -1, -1, -1, -1, 35, -1, -1,
      -1, -1, -1, -1, 36, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      37, -1, -1, -1, -1, -1, -1, -1, 38, 39, -1, -1, -1, -1,
      -1, -1, -1, -1, 40, 41, -1, -1, -1, 42, -1, -1, -1, 43,
      -1, 44, -1, 45, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 46, 47, -1, 48, 49, -1, -1, -1, -1, -1, 50, -1, 51,
      -1, 52, -1, -1, -1, -1, -1, -1, 53, 54, 55, -1, 56, 57,
      -1, -1, -1, -1, -1, -1, -1, 58, -1, -1, -1, 59, 60, 61,
      -1, -1, -1, 62, -1, -1, -1, 63, 64, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 65, -1, 66, 67, -1, 68, 69, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 70, -1, -1, -1, -1, -1, -1,
      -1, 71, -1, -1, -1, -1, -1, -1, -1, 72, -1, -1, -1, -1,
      -1, -1, -1, 73, -1, 74, -1, -1, -1, -1, -1, 75, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 76, -1, 77, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 78, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, 79, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 80, -1, -1, -1,
      -1, -1, -1, 81, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 82,
      -1, -1, -1, -1, -1, -1, -1, 83, -1, -1, -1, -1, 84, 85,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 86, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 87,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      88
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
