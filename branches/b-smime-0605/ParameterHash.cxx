/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 327, duplicates = 1 */

class ParameterHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct params *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
ParameterHash::hash (register const char *str, register unsigned int len)
{
  static unsigned short asso_values[] =
    {
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331,  25, 331,  30,   0, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331,   0,  65,  10,
       35,   0,  10,  50,  60,   0, 331,  10,  55, 125,
       20,   0,   0, 110,   0,   0,   0, 100,  55,   0,
       60,  40,   5, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331
    };
  register int hval = len;

  switch (hval)
    {
      default:
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

struct params *
ParameterHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 82,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 13,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 330
    };

  static struct params wordlist[] =
    {
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"size", ParameterTypes::size},
      {"actor", ParameterTypes::actor},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"nc", ParameterTypes::nc},
      {"id", ParameterTypes::id},
      {"data", ParameterTypes::data},
      {"type", ParameterTypes::type},
      {"priority", ParameterTypes::priority},
      {"tag", ParameterTypes::tag},
      {"nonce", ParameterTypes::nonce},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"stale", ParameterTypes::stale},
      {"server", ParameterTypes::server},
      {"text", ParameterTypes::text},
      {"cnonce", ParameterTypes::cnonce},
      {"expires", ParameterTypes::expires},
      {"app-id", ParameterTypes::appId},
      {"protocol", ParameterTypes::protocol},
      {"description", ParameterTypes::description},
      {"charset", ParameterTypes::charset},
      {"refresher", ParameterTypes::refresher},
      {"events", ParameterTypes::events},
      {"version", ParameterTypes::version},
      {"to-tag", ParameterTypes::toTag},
      {"+sip.instance", ParameterTypes::Instance},
      {"expiration", ParameterTypes::expiration},
      {"retry-after", ParameterTypes::retryAfter},
      {"control", ParameterTypes::control},
      {"directory", ParameterTypes::directory},
      {"video", ParameterTypes::video},
      {"application", ParameterTypes::application},
      {"access-type", ParameterTypes::accessType},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"purpose", ParameterTypes::purpose},
      {"received", ParameterTypes::received},
      {"extensions", ParameterTypes::extensions},
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"vendor", ParameterTypes::vendor},
      {"d-ver", ParameterTypes::dVer},
      {"isfocus", ParameterTypes::isFocus},
      {"+sip.flowid", ParameterTypes::FlowId},
      {"comp", ParameterTypes::comp},
      {"profile-type", ParameterTypes::profileType},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"permission", ParameterTypes::permission},
      {"url", ParameterTypes::url},
      {"branch", ParameterTypes::branch},
      {"duration", ParameterTypes::duration},
      {"mode", ParameterTypes::mode},
      {"network-user", ParameterTypes::networkUser},
      {"d-alg", ParameterTypes::dAlg},
      {"d-qop", ParameterTypes::dQop},
      {"realm", ParameterTypes::realm},
      {"domain", ParameterTypes::domain},
      {"maddr", ParameterTypes::maddr},
      {"schemes", ParameterTypes::schemes},
      {"opaque", ParameterTypes::opaque},
      {"filename", ParameterTypes::filename},
      {"model", ParameterTypes::model},
      {"from-tag", ParameterTypes::fromTag},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"effective-by", ParameterTypes::effectiveBy},
      {"micalg", ParameterTypes::micalg},
      {"handling", ParameterTypes::handling},
      {"early-only", ParameterTypes::earlyOnly},
      {"username", ParameterTypes::username},
      {"gruu", ParameterTypes::gruu},
      {"boundary", ParameterTypes::boundary},
      {"language", ParameterTypes::language},
      {"mobility", ParameterTypes::mobility},
      {"document", ParameterTypes::document},
      {"algorithm", ParameterTypes::algorithm},
      {"smime-type", ParameterTypes::smimeType}
    };

  static short lookup[] =
    {
        -1,   -1,   -1,   -1,    0,    1,   -1,   -1,
        -1,    2,   -1,   -1,   -1,   -1,   -1,    3,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,    4,   -1,    5,    6,   -1,   -1,
         7,   -1,   -1,   -1,   -1,    8,   -1,    9,
        -1,   -1,   -1,   -1,   10,   -1,   -1,   -1,
        11,   -1,   -1,   -1,   -1,   12,   -1,   13,
        -1,   14,   15,   -1,   16,   17,   -1,   -1,
        18,   -1,   19,   20,   -1,   -1,   -1,   21,
        -1,   22,   -1,   -1,   23,   24,   -1,   25,
        -1,   26,   27,   -1,   -1,   -1,   28,   -1,
        29,   -1,   30,   31,   32,   -1,   33,   34,
        35,   -1,   -1,   -1,   -1,   36,   -1,   37,
        38,   -1,   -1,   39,   40,   -1,   41,   42,
        -1,   43,   -1,   -1,   44,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   45,   -1,   46,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        47,   -1,   -1,   48,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   49,   -1, -233,  -32,   -2,
        -1,   -1,   -1,   52,   -1,   -1,   53,   -1,
        -1,   54,   -1,   55,   56,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   57,   -1,   -1,   58,
        -1,   -1,   -1,   -1,   59,   -1,   -1,   -1,
        -1,   60,   61,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        62,   -1,   63,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        64,   -1,   65,   -1,   66,   -1,   -1,   67,
        -1,   -1,   68,   69,   -1,   -1,   -1,   -1,
        70,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   71,   -1,
        72,   -1,   73,   -1,   -1,   74,   75,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   76,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   77,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   78,   -1,   -1,
        -1,   -1,   79,   80,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   81
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
              register struct params *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct params *wordendptr = wordptr + -lookup[offset + 1];

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
