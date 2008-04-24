/* C++ code produced by gperf version 3.0.1 */
/* Command-line: gperf --ignore-case -D -E -L C++ -t -k '*' --compare-strncmp -Z ParameterHash ParameterHash.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "ParameterHash.gperf"

#include <string.h>
#include <ctype.h>
#include "resip/stack/ParameterTypes.hxx"
namespace resip {
using namespace std;
#line 8 "ParameterHash.gperf"
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 242, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRNCMP
#define GPERF_CASE_STRNCMP 1
static int
gperf_case_strncmp (register const char *s1, register const char *s2, register unsigned int n)
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

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
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246,   5, 246,  55,   0, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246,  15,  50,   0,  25,   0,
        5,   0,  40,   0, 246,   0,  20,  20,   5,   0,
        0,  50,   0,   0,   0,  35,   0,   0,  75,  60,
        5, 246, 246, 246, 246, 246, 246,  15,  50,   0,
       25,   0,   5,   0,  40,   0, 246,   0,  20,  20,
        5,   0,   0,  50,   0,   0,   0,  35,   0,   0,
       75,  60,   5, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246, 246, 246, 246, 246,
      246, 246, 246, 246, 246, 246
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[12]];
      /*FALLTHROUGH*/
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct params *
ParameterHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 88,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 13,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 245
    };

  static struct params wordlist[] =
    {
#line 72 "ParameterHash.gperf"
      {"site", ParameterTypes::site},
#line 70 "ParameterHash.gperf"
      {"rport", ParameterTypes::rport},
#line 71 "ParameterHash.gperf"
      {"server", ParameterTypes::server},
#line 57 "ParameterHash.gperf"
      {"nc", ParameterTypes::nc},
#line 73 "ParameterHash.gperf"
      {"size", ParameterTypes::size},
#line 14 "ParameterHash.gperf"
      {"events", ParameterTypes::events},
#line 89 "ParameterHash.gperf"
      {"version", ParameterTypes::version},
#line 67 "ParameterHash.gperf"
      {"response", ParameterTypes::response},
#line 58 "ParameterHash.gperf"
      {"nonce", ParameterTypes::nonce},
#line 37 "ParameterHash.gperf"
      {"cnonce", ParameterTypes::cnonce},
#line 76 "ParameterHash.gperf"
      {"tag", ParameterTypes::tag},
#line 23 "ParameterHash.gperf"
      {"actor", ParameterTypes::actor},
#line 51 "ParameterHash.gperf"
      {"lr", ParameterTypes::lr},
#line 79 "ParameterHash.gperf"
      {"ttl", ParameterTypes::ttl},
#line 38 "ParameterHash.gperf"
      {"comp", ParameterTypes::comp},
#line 65 "ParameterHash.gperf"
      {"reason", ParameterTypes::reason},
#line 50 "ParameterHash.gperf"
      {"id", ParameterTypes::id},
#line 61 "ParameterHash.gperf"
      {"protocol", ParameterTypes::protocol},
#line 78 "ParameterHash.gperf"
      {"transport", ParameterTypes::transport},
#line 19 "ParameterHash.gperf"
      {"video", ParameterTypes::video},
#line 11 "ParameterHash.gperf"
      {"control", ParameterTypes::control},
#line 66 "ParameterHash.gperf"
      {"received", ParameterTypes::received},
#line 69 "ParameterHash.gperf"
      {"rinstance", ParameterTypes::rinstance},
#line 60 "ParameterHash.gperf"
      {"permission", ParameterTypes::permission},
#line 87 "ParameterHash.gperf"
      {"vendor", ParameterTypes::vendor},
#line 80 "ParameterHash.gperf"
      {"uri", ParameterTypes::uri},
#line 81 "ParameterHash.gperf"
      {"user", ParameterTypes::user},
#line 75 "ParameterHash.gperf"
      {"stale", ParameterTypes::stale},
#line 13 "ParameterHash.gperf"
      {"description", ParameterTypes::description},
#line 62 "ParameterHash.gperf"
      {"purpose", ParameterTypes::purpose},
#line 26 "ParameterHash.gperf"
      {"+sip.instance", ParameterTypes::Instance},
#line 56 "ParameterHash.gperf"
      {"name", ParameterTypes::name},
#line 22 "ParameterHash.gperf"
      {"isfocus", ParameterTypes::isFocus},
#line 55 "ParameterHash.gperf"
      {"mode", ParameterTypes::mode},
#line 63 "ParameterHash.gperf"
      {"q", ParameterTypes::q},
#line 28 "ParameterHash.gperf"
      {"ob", ParameterTypes::ob},
#line 95 "ParameterHash.gperf"
      {"qop", ParameterTypes::qop},
#line 85 "ParameterHash.gperf"
      {"refresher", ParameterTypes::refresher},
#line 94 "ParameterHash.gperf"
      {"url", ParameterTypes::url},
#line 10 "ParameterHash.gperf"
      {"data", ParameterTypes::data},
#line 64 "ParameterHash.gperf"
      {"realm", ParameterTypes::realm},
#line 54 "ParameterHash.gperf"
      {"micalg", ParameterTypes::micalg},
#line 36 "ParameterHash.gperf"
      {"charset", ParameterTypes::charset},
#line 21 "ParameterHash.gperf"
      {"type", ParameterTypes::type},
#line 18 "ParameterHash.gperf"
      {"application", ParameterTypes::application},
#line 17 "ParameterHash.gperf"
      {"schemes", ParameterTypes::schemes},
#line 15 "ParameterHash.gperf"
      {"priority", ParameterTypes::priority},
#line 88 "ParameterHash.gperf"
      {"model", ParameterTypes::model},
#line 43 "ParameterHash.gperf"
      {"domain", ParameterTypes::domain},
#line 47 "ParameterHash.gperf"
      {"filename", ParameterTypes::filename},
#line 31 "ParameterHash.gperf"
      {"gruu", ParameterTypes::gruu},
#line 77 "ParameterHash.gperf"
      {"to-tag", ParameterTypes::toTag},
#line 82 "ParameterHash.gperf"
      {"ext", ParameterTypes::extension},
#line 24 "ParameterHash.gperf"
      {"text", ParameterTypes::text},
#line 46 "ParameterHash.gperf"
      {"expires", ParameterTypes::expires},
#line 83 "ParameterHash.gperf"
      {"username", ParameterTypes::username},
#line 41 "ParameterHash.gperf"
      {"d-ver", ParameterTypes::dVer},
#line 27 "ParameterHash.gperf"
      {"reg-id", ParameterTypes::regid},
#line 44 "ParameterHash.gperf"
      {"duration", ParameterTypes::duration},
#line 52 "ParameterHash.gperf"
      {"maddr", ParameterTypes::maddr},
#line 53 "ParameterHash.gperf"
      {"method", ParameterTypes::method},
#line 16 "ParameterHash.gperf"
      {"methods", ParameterTypes::methods},
#line 91 "ParameterHash.gperf"
      {"document", ParameterTypes::document},
#line 42 "ParameterHash.gperf"
      {"directory", ParameterTypes::directory},
#line 25 "ParameterHash.gperf"
      {"extensions", ParameterTypes::extensions},
#line 97 "ParameterHash.gperf"
      {"addtransport", ParameterTypes::addTransport},
#line 20 "ParameterHash.gperf"
      {"language", ParameterTypes::language},
#line 92 "ParameterHash.gperf"
      {"app-id", ParameterTypes::appId},
#line 48 "ParameterHash.gperf"
      {"from-tag", ParameterTypes::fromTag},
#line 33 "ParameterHash.gperf"
      {"algorithm", ParameterTypes::algorithm},
#line 45 "ParameterHash.gperf"
      {"expiration", ParameterTypes::expiration},
#line 59 "ParameterHash.gperf"
      {"opaque", ParameterTypes::opaque},
#line 93 "ParameterHash.gperf"
      {"network-user", ParameterTypes::networkUser},
#line 96 "ParameterHash.gperf"
      {"sigcomp-id", ParameterTypes::sigcompId},
#line 35 "ParameterHash.gperf"
      {"branch", ParameterTypes::branch},
#line 49 "ParameterHash.gperf"
      {"handling", ParameterTypes::handling},
#line 39 "ParameterHash.gperf"
      {"d-alg", ParameterTypes::dAlg},
#line 40 "ParameterHash.gperf"
      {"d-qop", ParameterTypes::dQop},
#line 32 "ParameterHash.gperf"
      {"access-type", ParameterTypes::accessType},
#line 68 "ParameterHash.gperf"
      {"retry-after", ParameterTypes::retryAfter},
#line 86 "ParameterHash.gperf"
      {"profile-type", ParameterTypes::profileType},
#line 30 "ParameterHash.gperf"
      {"temp-gruu", ParameterTypes::tempGruu},
#line 12 "ParameterHash.gperf"
      {"mobility", ParameterTypes::mobility},
#line 74 "ParameterHash.gperf"
      {"smime-type", ParameterTypes::smimeType},
#line 90 "ParameterHash.gperf"
      {"effective-by", ParameterTypes::effectiveBy},
#line 34 "ParameterHash.gperf"
      {"boundary", ParameterTypes::boundary},
#line 29 "ParameterHash.gperf"
      {"pub-gruu", ParameterTypes::pubGruu},
#line 84 "ParameterHash.gperf"
      {"early-only", ParameterTypes::earlyOnly}
    };

  static signed char lookup[] =
    {
      -1, -1, -1, -1,  0,  1,  2,  3, -1,  4, -1,  5,  6,  7,
      -1,  8,  9, -1, 10, -1, 11, -1, 12, 13, 14, -1, 15, 16,
      17, 18, 19, -1, 20, 21, 22, 23, 24, -1, 25, 26, 27, 28,
      29, 30, 31, -1, -1, 32, -1, 33, -1, 34, 35, 36, 37, -1,
      -1, -1, 38, 39, 40, 41, 42, -1, 43, -1, 44, 45, 46, -1,
      47, 48, -1, 49, 50, -1, 51, -1, 52, 53, -1, -1, 54, 55,
      -1, 56, 57, -1, 58, -1, 59, 60, 61, 62, 63, 64, -1, 65,
      66, -1, -1, 67, -1, 68, 69, 70, 71, 72, -1, -1, 73, -1,
      -1, -1, -1, -1, 74, -1, 75, -1, 76, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, 77, -1, -1, -1, -1,
      -1, 78, -1, -1, -1, -1, 79, -1, -1, -1, -1, -1, 80, -1,
      81, -1, -1, -1, 82, -1, -1, -1, -1, -1, -1, 83, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 84, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 85, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 86, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, 87
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

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strncmp (str, s, len) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 98 "ParameterHash.gperf"

}
