/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z HeaderHash HeaderHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resip/stack/HeaderTypes.hxx"

namespace resip
{
using namespace std;
struct headers { char *name; Headers::Type type; };
/* maximum key range = 599, duplicates = 4 */

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
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600,   0, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600,   0,  95,  20,
        0,   0,  25, 100,  15,  10,  10,  35,  15,  45,
        0,  70,   0,   0,  50,  30,  40,   0,  75,  20,
       80,  65,  10, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600, 600, 600, 600, 600,
      600, 600, 600, 600, 600, 600
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
      TOTAL_KEYWORDS = 103,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 25,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 599
    };

  static struct headers wordlist[] =
    {
      {"e", Headers::ContentEncoding},
      {"i", Headers::CallID},
      {"l", Headers::ContentLength},
      {"c", Headers::ContentType},
      {"f", Headers::From},
      {"hide", Headers::UNKNOWN},
      {"s", Headers::Subject},
      {"k", Headers::Supported},
      {"t", Headers::To},
      {"date", Headers::Date},
      {"m", Headers::Contact},
      {"r", Headers::ReferTo},
      {"cseq", Headers::CSeq},
      {"path", Headers::Path},
      {"y", Headers::Identity},
      {"call-id", Headers::CallID},
      {"o", Headers::Event},
      {"v", Headers::Via},
      {"x", Headers::SessionExpires},
      {"rseq", Headers::RSeq},
      {"accept", Headers::Accept},
      {"via", Headers::Via},
      {"min-se", Headers::MinSE},
      {"join", Headers::Join},
      {"join", Headers::Join},
      {"b", Headers::ReferredBy},
      {"rack", Headers::RAck},
      {"to", Headers::To},
      {"require", Headers::Require},
      {"event", Headers::Event},
      {"replaces",Headers::Replaces},
      {"allow", Headers::Allow},
      {"reason", Headers::Reason},
      {"call-info", Headers::CallInfo},
      {"route", Headers::Route},
      {"identity", Headers::Identity},
      {"expires", Headers::Expires},
      {"warning", Headers::Warning},
      {"sip-etag", Headers::SIPETag},
      {"content-id", Headers::ContentId},
      {"from", Headers::From},
      {"contact", Headers::Contact},
      {"supported", Headers::Supported},
      {"unsupported", Headers::Unsupported},
      {"subject", Headers::Subject},
      {"sip-if-match", Headers::SIPIfMatch},
      {"server", Headers::Server},
      {"timestamp", Headers::Timestamp},
      {"alert-info",Headers::AlertInfo},
      {"answer-mode", Headers::AnswerMode},
      {"privacy", Headers::Privacy},
      {"user-agent", Headers::UserAgent},
      {"p-called-party-id", Headers::PCalledPartyId},
      {"p-called-party-id", Headers::PCalledPartyId},
      {"min-expires", Headers::MinExpires},
      {"www-authenticate",Headers::WWWAuthenticate},
      {"refer-to",Headers::ReferTo},
      {"reply-to", Headers::ReplyTo},
      {"refer-sub", Headers::ReferSub},
      {"in-reply-to", Headers::InReplyTo},
      {"encryption", Headers::UNKNOWN},
      {"p-associated-uri", Headers::PAssociatedUri},
      {"p-associated-uri", Headers::PAssociatedUri},
      {"allow-events", Headers::AllowEvents},
      {"identity-info", Headers::IdentityInfo},
      {"accept-contact", Headers::AcceptContact},
      {"content-type", Headers::ContentType},
      {"response-key", Headers::UNKNOWN},
      {"accept-encoding", Headers::AcceptEncoding},
      {"priority", Headers::Priority},
      {"accept-language", Headers::AcceptLanguage},
      {"security-client", Headers::SecurityClient},
      {"reject-contact", Headers::RejectContact},
      {"authorization", Headers::Authorization},
      {"retry-after", Headers::RetryAfter},
      {"p-asserted-identity", Headers::PAssertedIdentity},
      {"error-info", Headers::ErrorInfo},
      {"referred-by",Headers::ReferredBy},
      {"mime-version", Headers::MIMEVersion},
      {"content-length", Headers::ContentLength},
      {"session-expires", Headers::SessionExpires},
      {"service-route", Headers::ServiceRoute},
      {"service-route", Headers::ServiceRoute},
      {"p-preferred-identity", Headers::PPreferredIdentity},
      {"record-route", Headers::RecordRoute},
      {"priv-answer-mode", Headers::PrivAnswerMode},
      {"authentication-info", Headers::AuthenticationInfo},
      {"organization", Headers::Organization},
      {"max-forwards", Headers::MaxForwards},
      {"remote-party-id", Headers::RemotePartyId},
      {"content-encoding", Headers::ContentEncoding},
      {"proxy-require", Headers::ProxyRequire},
      {"p-media-authorization", Headers::PMediaAuthorization},
      {"content-language", Headers::ContentLanguage},
      {"request-disposition", Headers::RequestDisposition},
      {"security-server", Headers::SecurityServer},
      {"target-dialog", Headers::TargetDialog},
      {"proxy-authenticate", Headers::ProxyAuthenticate},
      {"security-verify", Headers::SecurityVerify},
      {"content-disposition", Headers::ContentDisposition},
      {"subscription-state",Headers::SubscriptionState},
      {"content-transfer-encoding", Headers::ContentTransferEncoding},
      {"proxy-authorization", Headers::ProxyAuthorization}
    };

  static short lookup[] =
    {
        -1,    0,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,    1,   -1,   -1,   -1,   -1,
         2,   -1,   -1,   -1,   -1,    3,   -1,   -1,
        -1,   -1,    4,   -1,   -1,    5,   -1,    6,
        -1,   -1,   -1,   -1,    7,   -1,   -1,   -1,
        -1,    8,   -1,   -1,    9,   -1,   10,   -1,
        -1,   -1,   -1,   11,   -1,   -1,   12,   -1,
        -1,   -1,   -1,   13,   -1,   -1,   -1,   -1,
        -1,   -1,   14,   15,   -1,   -1,   -1,   16,
        -1,   -1,   -1,   -1,   17,   -1,   -1,   -1,
        -1,   18,   -1,   -1,   19,   -1,   20,   -1,
        21,   -1,   -1,   22,   -1,   -1, -201,   -1,
        25,  -80,   -2,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   26,   -1,   -1,
        27,   -1,   -1,   -1,   -1,   28,   -1,   -1,
        29,   -1,   -1,   30,   -1,   31,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   32,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   33,   34,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   35,   -1,   -1,
        -1,   36,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   37,   38,   -1,   39,   -1,
        -1,   -1,   40,   -1,   -1,   41,   -1,   42,
        -1,   43,   44,   -1,   -1,   -1,   -1,   45,
        -1,   -1,   -1,   46,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   47,   48,   -1,   -1,   -1,
        -1,   -1,   49,   50,   -1,   -1,   51,   -1,
      -337,  -51,   -2,   -1,   54,   -1,   -1,   -1,
        -1,   55,   -1,   56,   -1,   -1,   -1,   -1,
        57,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   58,   -1,   59,   -1,   -1,
        -1,   60,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1, -382,   63,  -42,   -2,
        -1,   -1,   -1,   64,   65,   -1,   -1,   66,
        -1,   -1,   -1,   -1,   67,   -1,   -1,   68,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   69,
        -1,   -1,   -1,   -1,   -1,   -1,   70,   -1,
        -1,   -1,   -1,   71,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   72,   -1,   -1,   -1,
        73,   -1,   -1,   74,   -1,   -1,   75,   76,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   77,   78,   -1,   -1,   -1,   -1,
        -1,   -1,   79,   80,   -1,   -1, -467,   -1,
        83,   -1,   84,  -22,   -2,   -1,   85,   -1,
        -1,   86,   -1,   -1,   87,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   88,   -1,
        -1,   89,   90,   -1,   91,   -1,   -1,   92,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   93,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   95,   -1,   -1,   96,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        97,   -1,   -1,   -1,   -1,   -1,   -1,   98,
        -1,   -1,   -1,   99,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  100,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  101,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  102
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
