/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace resip;
using namespace std;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 151, duplicates = 1 */

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
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152,   0, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152,   0,  35,  30,
       20,   0,   0,  55,  15,   0, 152, 152,  10,  50,
        0,   0,   0,   0,   0,   0,   0,  35,  30, 152,
        0,   0,  10, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
      152, 152, 152, 152, 152, 152
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
      TOTAL_KEYWORDS = 53,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 151
    };

  static struct params wordlist[] =
    {
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"reason", ParameterTypes::reason},
      {"expires", ParameterTypes::expires},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"expiration", ParameterTypes::expiration},
      {"retry-after", ParameterTypes::retryAfter},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"size", ParameterTypes::size},
      {"stale", ParameterTypes::stale},
      {"id", ParameterTypes::id},
      {"d-qop", ParameterTypes::dQop},
      {"nc", ParameterTypes::nc},
      {"nonce", ParameterTypes::nonce},
      {"server", ParameterTypes::server},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"opaque", ParameterTypes::opaque},
      {"purpose", ParameterTypes::purpose},
      {"protocol", ParameterTypes::protocol},
      {"charset", ParameterTypes::charset},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"d-ver", ParameterTypes::dVer},
      {"tag", ParameterTypes::tag},
      {"directory", ParameterTypes::directory},
      {"permission", ParameterTypes::permission},
      {"to-tag", ParameterTypes::toTag},
      {"duration", ParameterTypes::duration},
      {"realm", ParameterTypes::realm},
      {"cnonce", ParameterTypes::cnonce},
      {"filename", ParameterTypes::filename},
      {"access-type", ParameterTypes::accessType},
      {"mode", ParameterTypes::mode},
      {"domain", ParameterTypes::domain},
      {"comp", ParameterTypes::comp},
      {"branch", ParameterTypes::branch},
      {"received", ParameterTypes::received},
      {"d-alg", ParameterTypes::dAlg},
      {"method", ParameterTypes::method},
      {"username", ParameterTypes::username},
      {"maddr", ParameterTypes::maddr},
      {"boundary", ParameterTypes::boundary},
      {"mobility", ParameterTypes::mobility},
      {"handling", ParameterTypes::handling},
      {"smime-type", ParameterTypes::smimeType},
      {"from-tag", ParameterTypes::fromTag},
      {"algorithm", ParameterTypes::algorithm},
      {"micalg", ParameterTypes::micalg}
    };

  static signed char lookup[] =
    {
        -1,    0,   -1,    1,    2,    3,    4,    5,
         6,    7,    8,    9,   10,   11,   12,   13,
        -1,   -1,   -1,   -1,   -1,   -1,   14,   -1,
        -1,   15,   -1,   -1,   -1,   -1,   -1,   -1,
        16,   -1,   -1,   17,   18,   -1,   19,   20,
        -1,   21,   22,   -1,   -1,   -1,   -1,   -1,
        23,   -1,   -1,   -1,   24,   -1, -110,   27,
       -28,   -2,   28,   29,   30,   31,   -1,   32,
        -1,   33,   34,   -1,   35,   -1,   -1,   36,
        -1,   -1,   37,   -1,   38,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   39,   -1,   40,   -1,
        41,   -1,   42,   43,   -1,   44,   -1,   45,
        -1,   -1,   46,   -1,   -1,   -1,   -1,   47,
        -1,   -1,   -1,   -1,   48,   -1,   49,   -1,
        -1,   50,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   51,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   52
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
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register struct params *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct params *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                   if (tolower(*str) == *s && !strncasecmp( str + 1, s + 1, len - 1 ))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
}
