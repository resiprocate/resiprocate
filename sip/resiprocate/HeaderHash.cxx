/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/HeaderTypes.hxx"

namespace resip
{
using namespace std;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 540, duplicates = 4 */

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
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541,   0, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541,   0,  55,  20,
        0,   0,  75, 100,   5,  10,   0,  35,  15,  90,
        0,   0,  65,   0,  50,  30,  40,   0,  45,   5,
       95,  65,  25, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541, 541, 541, 541, 541,
      541, 541, 541, 541, 541, 541
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
      TOTAL_KEYWORDS = 98,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 540
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"i", Headers::CallID},
      {"join", Headers::Join},
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
      {"v", Headers::Via},
      {"r", Headers::ReferTo},
      {"cseq", Headers::CSeq},
      {"b", Headers::ReferredBy},
      {"via", Headers::Via},
      {"y", Headers::Identity},
      {"call-id", Headers::CallID},
      {"f", Headers::From},
      {"rseq", Headers::RSeq},
      {"reason", Headers::Reason},
      {"event", Headers::Event},
      {"m", Headers::Contact},
      {"route", Headers::Route},
      {"x", Headers::SessionExpires},
      {"rack", Headers::RAck},
      {"path", Headers::Path},
      {"require", Headers::Require},
      {"content-id", Headers::ContentId},
      {"contact", Headers::Contact},
      {"min-se", Headers::MinSE},
      {"call-info", Headers::CallInfo},
      {"accept", Headers::Accept},
      {"subject", Headers::Subject},
      {"allow-events", Headers::AllowEvents},
      {"warning", Headers::Warning},
      {"identity", Headers::Identity},
      {"server", Headers::Server},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"replaces",Headers::Replaces},
      {"authorization", Headers::Authorization},
      {"alert-info",Headers::AlertInfo},
      {"from", Headers::From},
      {"record-route", Headers::RecordRoute},
      {"refer-to",Headers::ReferTo},
      {"user-agent", Headers::UserAgent},
      {"reply-to", Headers::ReplyTo},
      {"reject-contact", Headers::RejectContact},
      {"error-info", Headers::ErrorInfo},
      {"content-encoding", Headers::ContentEncoding},
      {"organization", Headers::Organization},
      {"sip-etag", Headers::SIPETag},
      {"in-reply-to", Headers::InReplyTo},
      {"expires", Headers::Expires},
      {"service-route", Headers::ServiceRoute},
      {"service-route", Headers::ServiceRoute},
      {"supported", Headers::Supported},
      {"encryption", Headers::UNKNOWN},
      {"unsupported", Headers::Unsupported},
      {"privacy", Headers::Privacy},
      {"identity-info", Headers::IdentityInfo},
      {"authentication-info", Headers::AuthenticationInfo},
      {"p-associated-uri", Headers::PAssociatedUri},
      {"p-associated-uri", Headers::PAssociatedUri},
      {"content-length", Headers::ContentLength},
      {"accept-contact", Headers::AcceptContact},
      {"content-type", Headers::ContentType},
      {"response-key", Headers::UNKNOWN},
      {"accept-encoding", Headers::AcceptEncoding},
      {"priority", Headers::Priority},
      {"content-disposition", Headers::ContentDisposition},
      {"security-client", Headers::SecurityClient},
      {"content-language", Headers::ContentLanguage},
      {"request-disposition", Headers::RequestDisposition},
      {"mime-version", Headers::MIMEVersion},
      {"referred-by",Headers::ReferredBy},
      {"sip-if-match", Headers::SIPIfMatch},
      {"min-expires", Headers::MinExpires},
      {"p-called-party-id", Headers::PCalledPartyId},
      {"p-called-party-id", Headers::PCalledPartyId},
      {"session-expires", Headers::SessionExpires},
      {"p-media-authorization", Headers::PMediaAuthorization},
      {"target-dialog", Headers::TargetDialog},
      {"timestamp", Headers::Timestamp},
      {"accept-language", Headers::AcceptLanguage},
      {"retry-after", Headers::RetryAfter},
      {"proxy-require", Headers::ProxyRequire},
      {"p-asserted-identity", Headers::PAssertedIdentity},
      {"security-server", Headers::SecurityServer},
      {"max-forwards", Headers::MaxForwards},
      {"subscription-state",Headers::SubscriptionState},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"proxy-authorization", Headers::ProxyAuthorization},
      {"security-verify", Headers::SecurityVerify},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"p-preferred-identity", Headers::PPreferredIdentity}
    };

  static short lookup[] =
    {
        -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,    1,   -1,   -1, -116,   -1,
         4,  -96,   -2,    5,   -1,    6,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,    7,
        -1,   -1,   -1,   -1,    8,   -1,   -1,   -1,
         9,   10,   11,   -1,   12,   -1,   13,   -1,
        -1,   -1,   -1,   14,   -1,   -1,   15,   -1,
        16,   -1,   17,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   18,   19,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   20,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   21,   -1,   22,   -1,
        -1,   -1,   23,   24,   -1,   -1,   -1,   25,
        26,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   27,   -1,   -1,
        -1,   -1,   28,   -1,   -1,   29,   -1,   -1,
        30,   -1,   -1,   -1,   -1,   -1,   -1,   31,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        32,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        33,   -1,   -1,   -1,   -1,   -1,   -1,   34,
        35,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   36,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   37,   38,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   39,   -1,   -1,
        -1,   -1,   40,   -1,   41,   -1,   -1,   -1,
        -1,   42,   -1,   -1,   -1,   -1,   -1,   -1,
        43,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   44,   -1,   -1,   45,   46,
        -1,   -1,   -1,   -1,   -1,   -1,   47,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   48,   49,   50,   51,   52,
        -1,   -1,   -1,   -1,   -1,   53,   -1,   -1,
        54,   55, -363,   58,   59,   60,   61,   62,
       -42,   -2,   -1,   -1,   -1,   63,   -1, -371,
       -34,   -2,   66,   -1,   -1,   -1,   -1,   67,
        -1,   -1,   68,   -1,   -1,   -1,   -1,   69,
        -1,   -1,   70,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   71,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   72,   73,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   74,   -1,   -1,   75,   -1,
        -1,   76,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   77,   78,   -1,   -1,
        -1,   79, -462,  -18,   -2,   82,   83,   -1,
        84,   -1,   -1,   -1,   -1,   -1,   85,   86,
        -1,   -1,   -1,   -1,   -1,   87,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   88,   89,
        -1,   -1,   -1,   -1,   -1,   90,   -1,   91,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   92,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        93,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   94,   95,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   96,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   97
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
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register struct headers *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct headers *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
}
