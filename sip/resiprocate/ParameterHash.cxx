/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
using namespace resip;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 260, duplicates = 1 */

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
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261,  15, 261,  55,   0, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261,   0,  15,  55,
       25,   0,  55,  15,  25,   0, 261, 261,  60,  55,
       35,   0,   0,   0,   0,   0,   0,  30,   5, 261,
       60,  20,   5, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261, 261, 261, 261, 261,
      261, 261, 261, 261, 261, 261
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
      TOTAL_KEYWORDS = 72,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 13,
      MIN_HASH_VALUE = 1,
      MAX_HASH_VALUE = 260
    };

  static struct params wordlist[] =
    {
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"size", ParameterTypes::size},
      {"server", ParameterTypes::server},
      {"tag", ParameterTypes::tag},
      {"type", ParameterTypes::type},
      {"id", ParameterTypes::id},
      {"priority", ParameterTypes::priority},
      {"data", ParameterTypes::data},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"video", ParameterTypes::video},
      {"opaque", ParameterTypes::opaque},
      {"purpose", ParameterTypes::purpose},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"events", ParameterTypes::events},
      {"actor", ParameterTypes::actor},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"text", ParameterTypes::text},
      {"stale", ParameterTypes::stale},
      {"expires", ParameterTypes::expires},
      {"to-tag", ParameterTypes::toTag},
      {"gruu", ParameterTypes::gruu},
      {"mode", ParameterTypes::mode},
      {"d-qop", ParameterTypes::dQop},
      {"charset", ParameterTypes::charset},
      {"refresher", ParameterTypes::refresher},
      {"d-ver", ParameterTypes::dVer},
      {"nc", ParameterTypes::nc},
      {"received", ParameterTypes::received},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"duration", ParameterTypes::duration},
      {"permission", ParameterTypes::permission},
      {"expiration", ParameterTypes::expiration},
      {"directory", ParameterTypes::directory},
      {"maddr", ParameterTypes::maddr},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"comp", ParameterTypes::comp},
      {"realm", ParameterTypes::realm},
      {"domain", ParameterTypes::domain},
      {"protocol", ParameterTypes::protocol},
      {"description", ParameterTypes::description},
      {"username", ParameterTypes::username},
      {"nonce", ParameterTypes::nonce},
      {"boundary", ParameterTypes::boundary},
      {"branch", ParameterTypes::branch},
      {"extensions", ParameterTypes::extensions},
      {"retry-after", ParameterTypes::retryAfter},
      {"schemes", ParameterTypes::schemes},
      {"isfocus", ParameterTypes::isFocus},
      {"+sip.instance", ParameterTypes::Instance},
      {"control", ParameterTypes::control},
      {"mobility", ParameterTypes::mobility},
      {"d-alg", ParameterTypes::dAlg},
      {"application", ParameterTypes::application},
      {"language", ParameterTypes::language},
      {"algorithm", ParameterTypes::algorithm},
      {"cnonce", ParameterTypes::cnonce},
      {"from-tag", ParameterTypes::fromTag},
      {"micalg", ParameterTypes::micalg},
      {"smime-type", ParameterTypes::smimeType},
      {"access-type", ParameterTypes::accessType},
      {"handling", ParameterTypes::handling},
      {"filename", ParameterTypes::filename},
      {"early-only", ParameterTypes::earlyOnly}
    };

  static short lookup[] =
    {
        -1,    0,   -1,    1,    2,    3,   -1,   -1,
        -1,    4,   -1,    5,   -1,   -1,   -1,   -1,
        -1,   -1,    6,   -1,   -1,   -1,   -1,   -1,
         7,   -1,   -1,    8,    9,   10,   -1,   -1,
        -1,   11,   12,   13,   14,   15,   -1,   -1,
        -1,   16,   -1,   17,   18,   -1,   19,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   20,   -1,   21,   22,
        23,   24,   -1,   25,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   26,   -1,   -1,   27,
        -1,   -1,   -1,   -1,   28,   29,   -1,   30,
        -1,   31,   32,   -1,   33,   34, -168,  -37,
        -2,   -1,   37,   -1,   38,   -1,   -1,   -1,
        -1,   39,   -1,   -1,   -1,   40,   41,   42,
        43,   -1,   44,   -1,   -1,   -1,   -1,   -1,
        45,   46,   -1,   47,   -1,   -1,   48,   -1,
        49,   -1,   50,   -1,   -1,   51,   -1,   -1,
        52,   -1,   -1,   -1,   53,   54,   55,   -1,
        -1,   -1,   -1,   56,   -1,   -1,   -1,   -1,
        -1,   57,   -1,   -1,   -1,   58,   59,   -1,
        60,   61,   -1,   62,   63,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   64,   -1,   65,   -1,   -1,   66,
        -1,   -1,   -1,   67,   68,   -1,   -1,   -1,
        -1,   -1,   -1,   69,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   70,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
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
