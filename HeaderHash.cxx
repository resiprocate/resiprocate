/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "sip2/sipstack/HeaderTypes.hxx"
namespace Vocal2 {
using namespace std;
using namespace Vocal2;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 494, duplicates = 0 */

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
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495,   0, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495,   0,   0, 102,
        0,   0,  60,  20,  15,  10,   0,   0,  15,  50,
        0,  20,   0,  10,   0,  30,  35,   0,  45,   0,
       60,   0,   0, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495, 495, 495, 495, 495,
      495, 495, 495, 495, 495, 495
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
      MAX_HASH_VALUE = 494
    };

  static struct headers wordlist[] =
    {
      {""},
      {"e", Headers::ContentEncoding},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"i", Headers::CallId},
      {""}, {""}, {""}, {""},
      {"l", Headers::ContentLength},
      {""}, {""}, {""}, {""},
      {"o", Headers::ContentType},
      {""}, {""}, {""}, {""}, {""},
      {"require", Headers::Require},
      {""},
      {"hide", Headers::UNKNOWN},
      {""},
      {"s", Headers::Subject},
      {""}, {""}, {""}, {""},
      {"t", Headers::To},
      {"warning", Headers::Warning},
      {""},
      {"date", Headers::Date},
      {""}, {""}, {""}, {""},
      {"rseq", Headers::UNKNOWN},
      {""},
      {"v", Headers::Via},
      {""}, {""}, {""}, {""},
      {"m", Headers::Contact},
      {""}, {""},
      {"path", Headers::UNKNOWN},
      {"allow", Headers::Allow},
      {"reason", Headers::UNKNOWN},
      {"to", Headers::To},
      {"via", Headers::Via},
      {""},
      {"route", Headers::Route},
      {"f", Headers::From},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"referred-by",Headers::ReferredBy},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"reply-to", Headers::ReplyTo},
      {""}, {""},
      {"server", Headers::Server},
      {""},
      {"priority", Headers::Priority},
      {""},
      {"event", Headers::Event},
      {""}, {""}, {""}, {""}, {""},
      {"in-reply-to", Headers::InReplyTo},
      {"response-key", Headers::UNKNOWN},
      {""},
      {"supported", Headers::Supported},
      {"user-agent", Headers::UserAgent},
      {"unsupported", Headers::Unsupported},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"rack", Headers::UNKNOWN},
      {"expires", Headers::Expires},
      {""}, {""}, {""}, {""}, {""},
      {"proxy-require", Headers::ProxyRequire},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"error-info", Headers::ErrorInfo},
      {""}, {""},
      {"refer-to",Headers::ReferTo},
      {""}, {""}, {""},
      {"organization", Headers::Organization},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"from", Headers::From},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"retry-after", Headers::RetryAfter},
      {""}, {""}, {""}, {""},
      {"cseq", Headers::CSeq},
      {""}, {""},
      {"call-id", Headers::CallId},
      {"alert-info",Headers::AlertInfo},
      {""}, {""}, {""}, {""},
      {"replaces",Headers::Replaces},
      {""}, {""},
      {"authorization", Headers::Authorization},
      {""}, {""}, {""}, {""}, {""},
      {"privacy", Headers::UNKNOWN},
      {""}, {""}, {""}, {""}, {""},
      {"p-preferred-identity", Headers::UNKNOWN},
      {"min-expires", Headers::MinExpires},
      {"allow-events", Headers::AllowEvents},
      {""},
      {"subject", Headers::Subject},
      {""}, {""},
      {"encryption", Headers::UNKNOWN},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"record-route", Headers::RecordRoute},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"p-asserted-identity", Headers::UNKNOWN},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"timestamp", Headers::Timestamp},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"p-media-authorization", Headers::UNKNOWN},
      {"mime-version", Headers::MIMEVersion},
      {""}, {""}, {""},
      {"call-info", Headers::CallInfo},
      {"max-forwards", Headers::MaxForwards},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"content-type", Headers::ContentType},
      {""}, {""}, {""}, {""},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"accept", Headers::Accept},
      {""}, {""},
      {"www-authenticate",Headers::WWWAuthenticate},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"content-language", Headers::ContentLanguage},
      {""}, {""}, {""},
      {"security-server", Headers::SecurityServer},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"content-length", Headers::ContentLength},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"contact", Headers::Contact},
      {""}, {""}, {""}, {""}, {""},
      {"security-verify", Headers::SecurityVerify},
      {""},
      {"accept-language", Headers::AcceptLanguage},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"security-client", Headers::SecurityClient},
      {"subscription-state",Headers::SubscriptionState},
      {""}, {""}, {""}, {""},
      {"content-encoding", Headers::ContentEncoding},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"authentication-info", Headers::AuthenticationInfo},
      {""}, {""}, {""}, {""},
      {"content-disposition", Headers::ContentDisposition},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"accept-encoding", Headers::AcceptEncoding},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"content-transfer-encoding", Headers::ContentTransferEncoding}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

           if (tolower(*str) == *s && !strncasecmp( str + 1, s + 1, len - 1 ))
            return &wordlist[key];
        }
    }
  return 0;
}
}

