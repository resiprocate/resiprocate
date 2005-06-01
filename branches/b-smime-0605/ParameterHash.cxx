/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -D --enum -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/ParameterTypes.hxx"
namespace resip {
using namespace std;
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
       40,   0,   0,   5,   0,   0,   0,  40,  35,   0,
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
      TOTAL_KEYWORDS = 81,
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
      {"type", ParameterTypes::type},
      {"priority", ParameterTypes::priority},
      {"comp", ParameterTypes::comp},
      {"server", ParameterTypes::server},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"reason", ParameterTypes::reason},
      {"purpose", ParameterTypes::purpose},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"opaque", ParameterTypes::opaque},
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
      {"events", ParameterTypes::events},
      {"version", ParameterTypes::version},
      {"realm", ParameterTypes::realm},
      {"schemes", ParameterTypes::schemes},
      {"id", ParameterTypes::id},
      {"data", ParameterTypes::data},
      {"nonce", ParameterTypes::nonce},
      {"url", ParameterTypes::url},
      {"refresher", ParameterTypes::refresher},
      {"retry-after", ParameterTypes::retryAfter},
      {"isfocus", ParameterTypes::isFocus},
      {"smime-type", ParameterTypes::smimeType},
      {"cnonce", ParameterTypes::cnonce},
      {"tag", ParameterTypes::tag},
      {"expiration", ParameterTypes::expiration},
      {"control", ParameterTypes::control},
      {"username", ParameterTypes::username},
      {"application", ParameterTypes::application},
      {"+sip.instance", ParameterTypes::Instance},
      {"mode", ParameterTypes::mode},
      {"app-id", ParameterTypes::appId},
      {"network-user", ParameterTypes::networkUser},
      {"d-qop", ParameterTypes::dQop},
      {"directory", ParameterTypes::directory},
      {"video", ParameterTypes::video},
      {"branch", ParameterTypes::branch},
      {"to-tag", ParameterTypes::toTag},
      {"mobility", ParameterTypes::mobility},
      {"received", ParameterTypes::received},
      {"extensions", ParameterTypes::extensions},
      {"description", ParameterTypes::description},
      {"d-ver", ParameterTypes::dVer},
      {"profile-type", ParameterTypes::profileType},
      {"domain", ParameterTypes::domain},
      {"method", ParameterTypes::method},
      {"methods", ParameterTypes::methods},
      {"vendor", ParameterTypes::vendor},
      {"filename", ParameterTypes::filename},
      {"model", ParameterTypes::model},
      {"duration", ParameterTypes::duration},
      {"gruu", ParameterTypes::gruu},
      {"micalg", ParameterTypes::micalg},
      {"from-tag", ParameterTypes::fromTag},
      {"maddr", ParameterTypes::maddr},
      {"document", ParameterTypes::document},
      {"effective-by", ParameterTypes::effectiveBy},
      {"early-only", ParameterTypes::earlyOnly},
      {"boundary", ParameterTypes::boundary},
      {"algorithm", ParameterTypes::algorithm},
      {"d-alg", ParameterTypes::dAlg},
      {"language", ParameterTypes::language},
      {"handling", ParameterTypes::handling}
    };

  static short lookup[] =
    {
        -1,   -1,   -1,   -1,    0,    1,    2,   -1,
         3,    4,   -1,   -1,   -1,   -1,   -1,    5,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
         6,   -1,   -1,   -1,    7,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,    8,
        -1,    9,   -1,   10,   11,   -1,   12,   13,
        14,   15,   -1,   16,   17,   -1,   -1,   -1,
        -1,   18,   19,   -1,   20,   -1,   21,   -1,
        22,   -1,   -1,   23,   -1, -152,  -57,   -2,
        -1,   26,   -1,   27,   28,   -1,   -1,   -1,
        -1,   29,   30,   -1,   -1,   31,   -1,   32,
        -1,   -1,   -1,   -1,   33,   -1,   34,   35,
        -1,   -1,   36,   37,   -1,   38,   39,   -1,
        -1,   40,   41,   -1,   42,   -1,   43,   -1,
        44,   45,   -1,   -1,   46,   -1,   47,   48,
        -1,   49,   50,   -1,   -1,   51,   -1,   -1,
        -1,   52,   53,   54,   -1,   -1,   -1,   -1,
        55,   -1,   56,   -1,   -1,   -1,   -1,   57,
        -1,   -1,   -1,   -1,   -1,   -1,   58,   59,
        -1,   -1,   -1,   60,   -1,   61,   -1,   -1,
        -1,   62,   -1,   -1,   -1,   -1,   63,   64,
        -1,   -1,   -1,   65,   -1,   66,   -1,   67,
        -1,   -1,   68,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   69,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   70,   -1,   -1,   -1,   -1,   -1,   -1,
        71,   -1,   72,   -1,   -1,   73,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   74,   -1,
        -1,   75,   -1,   -1,   76,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   77,
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
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   79,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   80
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
