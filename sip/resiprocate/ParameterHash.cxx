/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
using namespace resip;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 175, duplicates = 1 */

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
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179,  15, 179,  40, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179,   0,  15,  50,
       30,   0,  30,  30,  50,   0, 179, 179,  20,  50,
       20,   0,   0,  60,   0,   0,   0,  55,  30, 179,
       40,  10,   5, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179, 179, 179, 179, 179,
      179, 179, 179, 179, 179, 179
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
      TOTAL_KEYWORDS = 70,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 178
    };

  static struct params wordlist[] =
    {
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"size", ParameterTypes::size},
      {"type", ParameterTypes::type},
      {"priority", ParameterTypes::priority},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"stale", ParameterTypes::stale},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"id", ParameterTypes::id},
      {"tag", ParameterTypes::tag},
      {"data", ParameterTypes::data},
      {"server", ParameterTypes::server},
      {"text", ParameterTypes::text},
      {"expires", ParameterTypes::expires},
      {"actor", ParameterTypes::actor},
      {"events", ParameterTypes::events},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"q", ParameterTypes::q},
      {"purpose", ParameterTypes::purpose},
      {"qop", ParameterTypes::qopFactory},
      {"video", ParameterTypes::video},
      {"expiration", ParameterTypes::expiration},
      {"nc", ParameterTypes::nc},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"realm", ParameterTypes::realm},
      {"to-tag", ParameterTypes::toTag},
      {"protocol", ParameterTypes::protocol},
      {"permission", ParameterTypes::permission},
      {"mode", ParameterTypes::mode},
      {"extensions", ParameterTypes::extensions},
      {"retry-after", ParameterTypes::retryAfter},
      {"nonce", ParameterTypes::nonce},
      {"control", ParameterTypes::control},
      {"directory", ParameterTypes::directory},
      {"application", ParameterTypes::application},
      {"mobility", ParameterTypes::mobility},
      {"comp", ParameterTypes::comp},
      {"d-ver", ParameterTypes::dVer},
      {"domain", ParameterTypes::domain},
      {"charset", ParameterTypes::charset},
      {"description", ParameterTypes::description},
      {"duration", ParameterTypes::duration},
      {"+instance", ParameterTypes::Instance},
      {"maddr", ParameterTypes::maddr},
      {"received", ParameterTypes::received},
      {"opaque", ParameterTypes::opaque},
      {"d-alg", ParameterTypes::dAlg},
      {"filename", ParameterTypes::filename},
      {"username", ParameterTypes::username},
      {"d-qop", ParameterTypes::dQop},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"boundary", ParameterTypes::boundary},
      {"branch", ParameterTypes::branch},
      {"isfocus", ParameterTypes::isFocus},
      {"gruu", ParameterTypes::gruu},
      {"cnonce", ParameterTypes::cnonce},
      {"micalg", ParameterTypes::micalg},
      {"schemes", ParameterTypes::schemes},
      {"from-tag", ParameterTypes::fromTag},
      {"algorithm", ParameterTypes::algorithm},
      {"smime-type", ParameterTypes::smimeType},
      {"access-type", ParameterTypes::accessType},
      {"language", ParameterTypes::language},
      {"handling", ParameterTypes::handling}
    };

  static short lookup[] =
    {
        -1,   -1,   -1,   -1,    0,    1,   -1,   -1,
        -1,    2,   -1,   -1,   -1,   -1,    3,   -1,
        -1,   -1,    4,   -1,   -1,   -1,    5,    6,
        -1,    7,    8,   -1,    9,   10,   -1,   -1,
        11,   12,   13,   -1,   14,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   15,   -1,   -1,   16,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   17,
        18,   -1,   19,   20,   -1,   21,   22,   23,
        -1,   24,   -1,   -1,   -1,   -1,   25,   -1,
        26,   -1, -152,   29,   30,   -1,   31,   -1,
        32,  -43,   -2,   -1,   33,   -1,   -1,   -1,
        -1,   -1,   34,   35,   -1,   -1,   -1,   36,
        -1,   37,   -1,   38,   -1,   39,   -1,   40,
        41,   42,   43,   44,   -1,   -1,   -1,   45,
        -1,   46,   47,   48,   -1,   -1,   49,   -1,
        -1,   50,   -1,   -1,   -1,   51,   -1,   -1,
        52,   -1,   -1,   -1,   -1,   53,   -1,   54,
        55,   56,   57,   -1,   -1,   58,   59,   -1,
        60,   -1,   61,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   62,   63,   64,   65,
        66,   67,   -1,   68,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   69
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
