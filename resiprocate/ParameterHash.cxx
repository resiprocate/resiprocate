/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
using namespace resip;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 220, duplicates = 1 */

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
  static unsigned char asso_values[] =
    {
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221,  10, 221,  55, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221,   0,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221,   0,  20,  50,
       30,   0,  15,  20,  45,   0, 221, 221,  15,  65,
       40,   0,   0,   0,   0,   0,   0,  35,  10, 221,
       20,  25,   5, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221, 221, 221, 221, 221,
      221, 221, 221, 221, 221, 221
    };
  register int hval = len;

  switch (hval)
    {
      default:
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
      TOTAL_KEYWORDS = 72,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 220
    };

  static struct params wordlist[] =
    {
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"size", ParameterTypes::size},
      {"server", ParameterTypes::server},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"stale", ParameterTypes::stale},
      {"tag", ParameterTypes::tag},
      {"text", ParameterTypes::text},
      {"expires", ParameterTypes::expires},
      {"type", ParameterTypes::type},
      {"id", ParameterTypes::id},
      {"priority", ParameterTypes::priority},
      {"data", ParameterTypes::data},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"opaque", ParameterTypes::opaque},
      {"purpose", ParameterTypes::purpose},
      {"video", ParameterTypes::video},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"actor", ParameterTypes::actor},
      {"events", ParameterTypes::events},
      {"refresher", ParameterTypes::refresher},
      {"expiration", ParameterTypes::expiration},
      {"protocol", ParameterTypes::protocol},
      {"to-tag", ParameterTypes::toTag},
      {"realm", ParameterTypes::realm},
      {"d-qop", ParameterTypes::dQop},
      {"nc", ParameterTypes::nc},
      {"gruu", ParameterTypes::gruu},
      {"received", ParameterTypes::received},
      {"mode", ParameterTypes::mode},
      {"d-ver", ParameterTypes::dVer},
      {"charset", ParameterTypes::charset},
      {"retry-after", ParameterTypes::retryAfter},
      {"isfocus", ParameterTypes::isFocus},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"extensions", ParameterTypes::extensions},
      {"control", ParameterTypes::control},
      {"duration", ParameterTypes::duration},
      {"directory", ParameterTypes::directory},
      {"permission", ParameterTypes::permission},
      {"application", ParameterTypes::application},
      {"comp", ParameterTypes::comp},
      {"d-alg", ParameterTypes::dAlg},
      {"earlyOnly", ParameterTypes::earlyOnly},
      {"maddr", ParameterTypes::maddr},
      {"description", ParameterTypes::description},
      {"mobility", ParameterTypes::mobility},
      {"nonce", ParameterTypes::nonce},
      {"language", ParameterTypes::language},
      {"domain", ParameterTypes::domain},
      {"filename", ParameterTypes::filename},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"username", ParameterTypes::username},
      {"+instance", ParameterTypes::Instance},
      {"algorithm", ParameterTypes::algorithm},
      {"micalg", ParameterTypes::micalg},
      {"boundary", ParameterTypes::boundary},
      {"branch", ParameterTypes::branch},
      {"from-tag", ParameterTypes::fromTag},
      {"schemes", ParameterTypes::schemes},
      {"cnonce", ParameterTypes::cnonce},
      {"access-type", ParameterTypes::accessType},
      {"handling", ParameterTypes::handling},
      {"smime-type", ParameterTypes::smimeType}
    };

  static short lookup[] =
    {
        -1,    0,   -1,    1,    2,    3,   -1,   -1,
        -1,    4,   -1,   -1,   -1,   -1,   -1,   -1,
         5,    6,    7,   -1,    8,   -1,   -1,    9,
        10,   -1,   -1,   11,   -1,   12,   -1,   -1,
        13,   14,   15,   -1,   -1,   -1,   16,   17,
        -1,   18,   19,   -1,   -1,   20,   21,   -1,
        22,   23,   -1,   -1,   -1,   -1,   -1,   24,
        25,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   26,   27,   -1,
        -1,   28,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   29,   -1,   -1,   -1,   30,   -1,   -1,
        -1,   -1,   31,   -1,   32,   -1,   33,   -1,
        -1,   -1,   34,   35,   36,   -1,   37,   -1,
        -1,   -1,   38,   39,   -1, -190,   42,   -1,
        43,   44,   45,   46,   47,  -32,   -2,   48,
        -1,   -1,   -1,   -1,   -1,   49,   -1,   -1,
        -1,   50,   51,   52,   -1,   53,   -1,   54,
        -1,   -1,   55,   -1,   -1,   56,   -1,   57,
        -1,   -1,   58,   59,   60,   61,   -1,   -1,
        -1,   -1,   62,   -1,   63,   -1,   64,   -1,
        -1,   65,   -1,   66,   -1,   -1,   -1,   67,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   68,   -1,   -1,   -1,   -1,   69,
        -1,   -1,   -1,   -1,   -1,   -1,   70,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   71
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
