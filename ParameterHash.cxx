/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
using namespace resip;
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 380, duplicates = 1 */

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
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384,  15, 384,  25,   0, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384,   0,  30,  10,
       90,   0,  45, 105,  45,   0, 384,   5,  55,  25,
       40,   0,   0,   5,   0,   0,   0,  15,  35,   0,
       60,  20,   5, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384, 384, 384, 384, 384,
      384, 384, 384, 384, 384, 384
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
      TOTAL_KEYWORDS = 80,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 13,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 383
    };

  static struct params wordlist[] =
    {
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"size", ParameterTypes::size},
      {"actor", ParameterTypes::actor},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"purpose", ParameterTypes::purpose},
      {"type", ParameterTypes::type},
      {"opaque", ParameterTypes::opaque},
      {"priority", ParameterTypes::priority},
      {"comp", ParameterTypes::comp},
      {"server", ParameterTypes::server},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"nc", ParameterTypes::nc},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"stale", ParameterTypes::stale},
      {"charset", ParameterTypes::charset},
      {"text", ParameterTypes::text},
      {"expires", ParameterTypes::expires},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"protocol", ParameterTypes::protocol},
      {"permission", ParameterTypes::permission},
      {"access-type", ParameterTypes::accessType},
      {"isfocus", ParameterTypes::isFocus},
      {"events", ParameterTypes::events},
      {"version", ParameterTypes::version},
      {"realm", ParameterTypes::realm},
      {"schemes", ParameterTypes::schemes},
      {"username", ParameterTypes::username},
      {"id", ParameterTypes::id},
      {"data", ParameterTypes::data},
      {"nonce", ParameterTypes::nonce},
      {"network-user", ParameterTypes::networkUser},
      {"refresher", ParameterTypes::refresher},
      {"retry-after", ParameterTypes::retryAfter},
      {"smime-type", ParameterTypes::smimeType},
      {"cnonce", ParameterTypes::cnonce},
      {"tag", ParameterTypes::tag},
      {"expiration", ParameterTypes::expiration},
      {"control", ParameterTypes::control},
      {"application", ParameterTypes::application},
      {"+sip.instance", ParameterTypes::Instance},
      {"mode", ParameterTypes::mode},
      {"app-id", ParameterTypes::appId},
      {"d-qop", ParameterTypes::dQop},
      {"directory", ParameterTypes::directory},
      {"video", ParameterTypes::video},
      {"branch", ParameterTypes::branch},
      {"to-tag", ParameterTypes::toTag},
      {"mobility", ParameterTypes::mobility},
      {"gruu", ParameterTypes::gruu},
      {"received", ParameterTypes::received},
      {"extensions", ParameterTypes::extensions},
      {"description", ParameterTypes::description},
      {"duration", ParameterTypes::duration},
      {"d-ver", ParameterTypes::dVer},
      {"profile-type", ParameterTypes::profileType},
      {"domain", ParameterTypes::domain},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"vendor", ParameterTypes::vendor},
      {"filename", ParameterTypes::filename},
      {"model", ParameterTypes::model},
      {"document", ParameterTypes::document},
      {"micalg", ParameterTypes::micalg},
      {"boundary", ParameterTypes::boundary},
      {"from-tag", ParameterTypes::fromTag},
      {"maddr", ParameterTypes::maddr},
      {"effective-by", ParameterTypes::effectiveBy},
      {"early-only", ParameterTypes::earlyOnly},
      {"algorithm", ParameterTypes::algorithm},
      {"d-alg", ParameterTypes::dAlg},
      {"language", ParameterTypes::language},
      {"handling", ParameterTypes::handling}
    };

  static short lookup[] =
    {
        -1,   -1,   -1,   -1,    0,    1,    2,   -1,
         3,    4,   -1,   -1,   -1,   -1,   -1,    5,
        -1,   -1,    6,    7,   -1,   -1,    8,   -1,
         9,   -1,   10,   -1,   11,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   12,
        -1,   13,   -1,   -1,   -1,   -1,   14,   -1,
        15,   16,   -1,   -1,   17,   -1,   -1,   -1,
        -1,   18,   19,   -1,   20,   -1,   21,   -1,
        22,   -1,   -1,   23,   -1, -151,  -56,   -2,
        -1,   26,   -1,   27,   28,   29,   -1,   -1,
        -1,   30,   31,   -1,   -1,   32,   -1,   33,
        34,   -1,   -1,   -1,   35,   -1,   36,   37,
        -1,   38,   -1,   39,   -1,   40,   -1,   -1,
        -1,   41,   42,   -1,   43,   -1,   44,   -1,
        45,   -1,   -1,   -1,   46,   -1,   47,   48,
        -1,   49,   -1,   -1,   -1,   50,   -1,   -1,
        -1,   51,   52,   53,   -1,   -1,   -1,   -1,
        54,   -1,   55,   56,   -1,   -1,   -1,   57,
        -1,   -1,   -1,   -1,   -1,   -1,   58,   59,
        -1,   60,   -1,   61,   -1,   62,   -1,   -1,
        -1,   63,   -1,   -1,   -1,   -1,   64,   65,
        -1,   -1,   -1,   66,   -1,   67,   -1,   68,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   69,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   70,   -1,   71,   -1,   -1,   -1,   -1,
        72,   -1,   73,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   74,   -1,
        -1,   75,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   76,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        77,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        78,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   79
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
