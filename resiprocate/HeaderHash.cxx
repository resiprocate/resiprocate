/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D -E -L C++ -t -k'*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
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
      326, 326, 326, 326, 326,  15, 326, 326, 326,   0,
      326, 326, 326, 326, 326, 326, 326,  10, 326, 326,
      326, 326,   0,   0, 326, 326, 326, 326, 326, 326,
      326, 326, 326, 326, 326, 326, 326,   0,  15,  20,
        0,   0,  65,  35,  10,  10,  10,  45,  15,   5,
        0,   0,   0,   5,   0,  30,  40,   0, 105,   0,
       50,   0,   5, 326, 326, 326, 326, 326, 326, 326,
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
      {"hide", Headers::UNKNOWN},
      {"Min-SE", Headers::MinSE},
      {"s", Headers::Subject},
      {"allow", Headers::Allow},
      {"reason", Headers::UNKNOWN},
      {"rseq", Headers::RSeq},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"route", Headers::Route},
      {"k", Headers::Supported},
      {"x", Headers::SessionExpires},
      {"warning", Headers::Warning},
      {"path", Headers::UNKNOWN},
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
      {"proxy-require", Headers::ProxyRequire},
      {"supported", Headers::Supported},
      {"encryption", Headers::UNKNOWN},
      {"unsupported", Headers::Unsupported},
      {"RAck", Headers::RAck},
      {"error-info", Headers::ErrorInfo},
      {"accept", Headers::Accept},
      {"referred-by",Headers::ReferredBy},
      {"expires", Headers::Expires},
      {"v", Headers::Via},
      {"organization", Headers::Organization},
      {"refer-to",Headers::ReferTo},
      {"user-agent", Headers::UserAgent},
      {"min-expires", Headers::MinExpires},
      {"response-key", Headers::UNKNOWN},
      {"via", Headers::Via},
      {"subject", Headers::Subject},
      {"sip-etag", Headers::SIPETag},
      {"contact", Headers::Contact},
      {"authorization", Headers::Authorization},
      {"call-info", Headers::CallInfo},
      {"timestamp", Headers::Timestamp},
      {"alert-info",Headers::AlertInfo},
      {"server", Headers::Server},
      {"privacy", Headers::UNKNOWN},
      {"event", Headers::Event},
      {"p-media-authorization", Headers::UNKNOWN},
      {"content-type", Headers::ContentType},
      {"retry-after", Headers::RetryAfter},
      {"accept-encoding", Headers::AcceptEncoding},
      {"max-forwards", Headers::MaxForwards},
      {"Session-Expires", Headers::SessionExpires},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"mime-version", Headers::MIMEVersion},
      {"accept-language", Headers::AcceptLanguage},
      {"content-encoding", Headers::ContentEncoding},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"security-client", Headers::SecurityClient},
      {"content-language", Headers::ContentLanguage},
      {"sip-if-match", Headers::SIPIfMatch},
      {"content-length", Headers::ContentLength},
      {"allow-events", Headers::AllowEvents},
      {"p-asserted-identity", Headers::UNKNOWN},
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
      -1, -1,  4, -1, -1, -1, -1,  5,  6, -1,  7, -1,  8, -1,
      -1, -1, -1,  9, -1, -1, -1, 10, 11, -1, -1, 12, -1, 13,
      14, -1, 15, 16, 17, -1, -1, -1, -1, 18, 19, -1, 20, -1,
      -1, -1, -1, 21, -1, -1, -1, 22, -1, -1, 23, 24, 25, 26,
      -1, -1, 27, 28, 29, -1, 30, -1, 31, 32, 33, 34, -1, -1,
      35, 36, 37, -1, -1, -1, -1, 38, -1, -1, -1, -1, -1, 39,
      -1, -1, -1, -1, -1, -1, -1, -1, 40, -1, -1, -1, -1, -1,
      41, 42, -1, 43, 44, 45, 46, -1, -1, -1, 47, 48, -1, -1,
      -1, 49, 50, -1, -1, -1, -1, -1, 51, -1, -1, -1, -1, 52,
      53, 54, 55, -1, -1, -1, -1, -1, -1, -1, 56, 57, 58, -1,
      -1, -1, 59, -1, -1, -1, 60, -1, 61, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 62, 63, 64, -1, -1, 65, 66,
      -1, -1, 67, 68, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 69, 70, 71, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 72, -1, -1, 73, -1, 74, -1, -1, -1, -1,
      -1, -1, -1, -1, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1,
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
