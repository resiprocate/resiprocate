/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 570, duplicates = 0 */

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
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571,   0, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571,   0,  55,  20,
        0,   0,  75, 100,   5,  10,   0,  35,  15,  45,
        0,   0,  80,   0,  50,  30,  40,   0,  60,   5,
       80,  65,  25, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571, 571, 571, 571, 571,
      571, 571, 571, 571, 571, 571
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
      TOTAL_KEYWORDS = 93,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 570
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"i", Headers::CallID},
      {"join", Headers::Join},
      {"l", Headers::ContentLength},
      {"hide", Headers::UNKNOWN},
      {"c", Headers::ContentType},
      {"s", Headers::Subject},
      {"k", Headers::Supported},
      {"allow", Headers::Allow},
      {"t", Headers::To},
      {"to", Headers::To},
      {"date", Headers::Date},
      {"m", Headers::Contact},
      {"r", Headers::ReferTo},
      {"cseq", Headers::CSeq},
      {"b", Headers::ReferredBy},
      {"v", Headers::Via},
      {"y", Headers::Identity},
      {"call-id", Headers::CallID},
      {"via", Headers::Via},
      {"f", Headers::From},
      {"x", Headers::SessionExpires},
      {"rseq", Headers::RSeq},
      {"reason", Headers::Reason},
      {"min-se", Headers::MinSE},
      {"route", Headers::Route},
      {"event", Headers::Event},
      {"rack", Headers::RAck},
      {"require", Headers::Require},
      {"content-id", Headers::ContentId},
      {"contact", Headers::Contact},
      {"path", Headers::Path},
      {"call-info", Headers::CallInfo},
      {"subject", Headers::Subject},
      {"accept", Headers::Accept},
      {"warning", Headers::Warning},
      {"identity", Headers::Identity},
      {"from", Headers::From},
      {"allow-events", Headers::AllowEvents},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"authorization", Headers::Authorization},
      {"server", Headers::Server},
      {"alert-info",Headers::AlertInfo},
      {"replaces",Headers::Replaces},
      {"record-route", Headers::RecordRoute},
      {"refer-to",Headers::ReferTo},
      {"user-agent", Headers::UserAgent},
      {"reject-contact", Headers::RejectContact},
      {"error-info", Headers::ErrorInfo},
      {"content-encoding", Headers::ContentEncoding},
      {"organization", Headers::Organization},
      {"expires", Headers::Expires},
      {"reply-to", Headers::ReplyTo},
      {"mime-version", Headers::MIMEVersion},
      {"identity-info", Headers::IdentityInfo},
      {"sip-etag", Headers::SIPETag},
      {"authentication-info", Headers::AuthenticationInfo},
      {"in-reply-to", Headers::InReplyTo},
      {"service-route", Headers::ServiceRoute},
      {"content-length", Headers::ContentLength},
      {"encryption", Headers::UNKNOWN},
      {"p-associated-uri", Headers::PAssociatedUri},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"privacy", Headers::Privacy},
      {"accept-contact", Headers::AcceptContact},
      {"content-type", Headers::ContentType},
      {"timestamp", Headers::Timestamp},
      {"response-key", Headers::UNKNOWN},
      {"accept-encoding", Headers::AcceptEncoding},
      {"priority", Headers::Priority},
      {"security-client", Headers::SecurityClient},
      {"min-expires", Headers::MinExpires},
      {"sip-if-match", Headers::SIPIfMatch},
      {"content-disposition", Headers::ContentDisposition},
      {"content-language", Headers::ContentLanguage},
      {"p-media-authorization", Headers::PMediaAuthorization},
      {"max-forwards", Headers::MaxForwards},
      {"request-disposition", Headers::RequestDisposition},
      {"referred-by",Headers::ReferredBy},
      {"session-expires", Headers::SessionExpires},
      {"retry-after", Headers::RetryAfter},
      {"accept-language", Headers::AcceptLanguage},
      {"p-called-party-id", Headers::PCalledPartyId},
      {"proxy-require", Headers::ProxyRequire},
      {"p-asserted-identity", Headers::PAssertedIdentity},
      {"security-server", Headers::SecurityServer},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"subscription-state",Headers::SubscriptionState},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"security-verify", Headers::SecurityVerify},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"p-preferred-identity", Headers::PPreferredIdentity}
    };

  static signed char lookup[] =
    {
      -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1,
       2, -1,  3, -1, -1,  4, -1,  5, -1, -1, -1, -1, -1, -1,
      -1, -1, -1,  6, -1, -1, -1, -1,  7, -1, -1, -1,  8,  9,
      10, -1, 11, -1, 12, -1, -1, -1, -1, 13, -1, -1, 14, -1,
      15, -1, -1, -1, -1, 16, -1, -1, -1, -1, 17, 18, -1, -1,
      -1, -1, -1, 19, -1, -1, 20, -1, -1, -1, -1, 21, -1, -1,
      22, -1, 23, -1, -1, -1, -1, 24, -1, -1, -1, 25, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 26, -1, -1, -1, 27, -1, -1,
      -1, -1, -1, -1, -1, 28, -1, -1, 29, -1, -1, -1, -1, -1,
      -1, 30, -1, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, 32, -1, -1, -1, -1, -1, -1, -1, 33, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 34, -1,
      -1, -1, -1, -1, 35, 36, 37, -1, -1, 38, -1, -1, -1, -1,
      -1, -1, -1, -1, 39, -1, -1, -1, -1, -1, -1, 40, -1, -1,
      41, -1, -1, -1, 42, -1, -1, 43, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 44, 45,
      -1, -1, -1, -1, -1, -1, 46, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, 47, 48, 49, 50, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 51, 52, -1, -1, -1, 53, 54, -1, -1,
      -1, -1, 55, 56, -1, 57, -1, 58, 59, 60, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, 61, -1, -1, 62, -1, 63, 64, -1,
      65, -1, -1, 66, -1, 67, -1, -1, 68, -1, -1, 69, -1, -1,
      -1, -1, -1, -1, -1, 70, -1, 71, 72, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 73, -1, 74, -1, 75, -1, -1, -1, -1,
      76, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 77, -1, 78,
      -1, -1, -1, -1, -1, -1, 79, -1, -1, -1, -1, -1, -1, -1,
      -1, 80, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, 81, -1, -1, -1, -1, -1, -1, -1, -1, 82, -1,
      83, -1, -1, -1, -1, -1, 84, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 85, -1, -1, -1, -1, -1,
      86, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      87, -1, -1, -1, -1, 88, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 89, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      90, -1, -1, -1, -1, -1, -1, -1, -1, -1, 91, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 92
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
