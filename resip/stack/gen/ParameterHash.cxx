/* C++ code produced by gperf version 3.0.3 */
/* Command-line: gperf -C -D -E -L C++ -t --key-positions='*' --compare-strncmp --ignore-case -Z ParameterHash ParameterHash.gperf  */

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
struct params { const char *name; ParameterTypes::Type type; };
/* maximum key range = 511, duplicates = 0 */

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
  static const struct params *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
ParameterHash::hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513,   0, 513,   5,   0, 513, 513, 513,
      513,   0, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513,   0,  85,   0,  25,   5,
      115,  45,  70,   0, 513,   0,  50,  25,   5,   0,
        5, 119,   0,   0,   0,  35, 100,  40, 125,  65,
        5, 513, 513, 513, 513, 513, 513,   0,  85,   0,
       25,   5, 115,  45,  70,   0, 513,   0,  50,  25,
        5,   0,   5, 119,   0,   0,   0,  35, 100,  40,
      125,  65,   5, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
      513, 513, 513, 513, 513, 513
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[17]];
      /*FALLTHROUGH*/
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      /*FALLTHROUGH*/
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      /*FALLTHROUGH*/
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      /*FALLTHROUGH*/
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      /*FALLTHROUGH*/
      case 13:
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

const struct params *
ParameterHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 106,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 18,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 512
    };

  static const struct params wordlist[] =
    {
#line 100 "ParameterHash.gperf"
      {"rc", ParameterTypes::rc},
#line 23 "ParameterHash.gperf"
      {"actor", ParameterTypes::actor},
#line 60 "ParameterHash.gperf"
      {"nc", ParameterTypes::nc},
#line 83 "ParameterHash.gperf"
      {"site", ParameterTypes::site},
#line 54 "ParameterHash.gperf"
      {"rport", ParameterTypes::rport},
#line 102 "ParameterHash.gperf"
      {"np", ParameterTypes::np},
#line 81 "ParameterHash.gperf"
      {"size", ParameterTypes::size},
#line 71 "ParameterHash.gperf"
      {"reason", ParameterTypes::reason},
#line 34 "ParameterHash.gperf"
      {"transport", ParameterTypes::transport},
#line 59 "ParameterHash.gperf"
      {"nonce", ParameterTypes::nonce},
#line 56 "ParameterHash.gperf"
      {"cnonce", ParameterTypes::cnonce},
#line 111 "ParameterHash.gperf"
      {"content", ParameterTypes::content},
#line 52 "ParameterHash.gperf"
      {"rinstance", ParameterTypes::rinstance},
#line 58 "ParameterHash.gperf"
      {"id", ParameterTypes::id},
#line 63 "ParameterHash.gperf"
      {"response", ParameterTypes::response},
#line 10 "ParameterHash.gperf"
      {"data", ParameterTypes::data},
#line 101 "ParameterHash.gperf"
      {"mp", ParameterTypes::mp},
#line 27 "ParameterHash.gperf"
      {"+sip.instance", ParameterTypes::Instance},
#line 53 "ParameterHash.gperf"
      {"comp", ParameterTypes::comp},
#line 69 "ParameterHash.gperf"
      {"uri", ParameterTypes::uri},
#line 33 "ParameterHash.gperf"
      {"name", ParameterTypes::name},
#line 110 "ParameterHash.gperf"
      {"term-ioi", ParameterTypes::termIoi},
#line 35 "ParameterHash.gperf"
      {"user", ParameterTypes::user},
#line 25 "ParameterHash.gperf"
      {"cause", ParameterTypes::cause},
#line 95 "ParameterHash.gperf"
      {"app-id", ParameterTypes::appId},
#line 30 "ParameterHash.gperf"
      {"gr", ParameterTypes::gr},
#line 48 "ParameterHash.gperf"
      {"tag", ParameterTypes::tag},
#line 82 "ParameterHash.gperf"
      {"permission", ParameterTypes::permission},
#line 13 "ParameterHash.gperf"
      {"description", ParameterTypes::description},
#line 40 "ParameterHash.gperf"
      {"lr", ParameterTypes::lr},
#line 38 "ParameterHash.gperf"
      {"ttl", ParameterTypes::ttl},
#line 43 "ParameterHash.gperf"
      {"to-tag", ParameterTypes::toTag},
#line 42 "ParameterHash.gperf"
      {"purpose", ParameterTypes::purpose},
#line 109 "ParameterHash.gperf"
      {"orig-ioi", ParameterTypes::origIoi},
#line 85 "ParameterHash.gperf"
      {"mode", ParameterTypes::mode},
#line 64 "ParameterHash.gperf"
      {"stale", ParameterTypes::stale},
#line 57 "ParameterHash.gperf"
      {"domain", ParameterTypes::domain},
#line 11 "ParameterHash.gperf"
      {"control", ParameterTypes::control},
#line 77 "ParameterHash.gperf"
      {"protocol", ParameterTypes::protocol},
#line 114 "ParameterHash.gperf"
      {"ws-src-ip", ParameterTypes::wsSrcIp},
#line 115 "ParameterHash.gperf"
      {"ws-src-port", ParameterTypes::wsSrcPort},
#line 113 "ParameterHash.gperf"
      {"addtransport", ParameterTypes::addTransport},
#line 45 "ParameterHash.gperf"
      {"duration", ParameterTypes::duration},
#line 18 "ParameterHash.gperf"
      {"application", ParameterTypes::application},
#line 15 "ParameterHash.gperf"
      {"priority", ParameterTypes::priority},
#line 21 "ParameterHash.gperf"
      {"type", ParameterTypes::type},
#line 39 "ParameterHash.gperf"
      {"maddr", ParameterTypes::maddr},
#line 87 "ParameterHash.gperf"
      {"charset", ParameterTypes::charset},
#line 65 "ParameterHash.gperf"
      {"username", ParameterTypes::username},
#line 62 "ParameterHash.gperf"
      {"realm", ParameterTypes::realm},
#line 28 "ParameterHash.gperf"
      {"reg-id", ParameterTypes::regid},
#line 29 "ParameterHash.gperf"
      {"ob", ParameterTypes::ob},
#line 97 "ParameterHash.gperf"
      {"url", ParameterTypes::url},
#line 112 "ParameterHash.gperf"
      {"encoding", ParameterTypes::encoding},
#line 88 "ParameterHash.gperf"
      {"access-type", ParameterTypes::accessType},
#line 94 "ParameterHash.gperf"
      {"document", ParameterTypes::document},
#line 84 "ParameterHash.gperf"
      {"directory", ParameterTypes::directory},
#line 96 "ParameterHash.gperf"
      {"network-user", ParameterTypes::networkUser},
#line 91 "ParameterHash.gperf"
      {"model", ParameterTypes::model},
#line 17 "ParameterHash.gperf"
      {"schemes", ParameterTypes::schemes},
#line 104 "ParameterHash.gperf"
      {"cgi-3gpp", ParameterTypes::cgi3gpp},
#line 98 "ParameterHash.gperf"
      {"sigcomp-id", ParameterTypes::sigcompId},
#line 86 "ParameterHash.gperf"
      {"server", ParameterTypes::server},
#line 92 "ParameterHash.gperf"
      {"version", ParameterTypes::version},
#line 105 "ParameterHash.gperf"
      {"ccf", ParameterTypes::ccf},
#line 41 "ParameterHash.gperf"
      {"q", ParameterTypes::q},
#line 14 "ParameterHash.gperf"
      {"events", ParameterTypes::events},
#line 106 "ParameterHash.gperf"
      {"ecf", ParameterTypes::ecf},
#line 78 "ParameterHash.gperf"
      {"micalg", ParameterTypes::micalg},
#line 68 "ParameterHash.gperf"
      {"qop", ParameterTypes::qop},
#line 72 "ParameterHash.gperf"
      {"d-alg", ParameterTypes::dAlg},
#line 37 "ParameterHash.gperf"
      {"method", ParameterTypes::method},
#line 16 "ParameterHash.gperf"
      {"methods", ParameterTypes::methods},
#line 36 "ParameterHash.gperf"
      {"ext", ParameterTypes::extension},
#line 24 "ParameterHash.gperf"
      {"text", ParameterTypes::text},
#line 19 "ParameterHash.gperf"
      {"video", ParameterTypes::video},
#line 74 "ParameterHash.gperf"
      {"d-ver", ParameterTypes::dVer},
#line 90 "ParameterHash.gperf"
      {"vendor", ParameterTypes::vendor},
#line 108 "ParameterHash.gperf"
      {"icid-generated-at", ParameterTypes::icidGeneratedAt},
#line 75 "ParameterHash.gperf"
      {"smime-type", ParameterTypes::smimeType},
#line 46 "ParameterHash.gperf"
      {"expires", ParameterTypes::expires},
#line 50 "ParameterHash.gperf"
      {"received", ParameterTypes::received},
#line 80 "ParameterHash.gperf"
      {"expiration", ParameterTypes::expiration},
#line 26 "ParameterHash.gperf"
      {"extensions", ParameterTypes::extensions},
#line 22 "ParameterHash.gperf"
      {"isfocus", ParameterTypes::isFocus},
#line 73 "ParameterHash.gperf"
      {"d-qop", ParameterTypes::dQop},
#line 32 "ParameterHash.gperf"
      {"temp-gruu", ParameterTypes::tempGruu},
#line 99 "ParameterHash.gperf"
      {"index", ParameterTypes::index},
#line 49 "ParameterHash.gperf"
      {"branch", ParameterTypes::branch},
#line 61 "ParameterHash.gperf"
      {"opaque", ParameterTypes::opaque},
#line 51 "ParameterHash.gperf"
      {"require", ParameterTypes::require},
#line 20 "ParameterHash.gperf"
      {"language", ParameterTypes::language},
#line 44 "ParameterHash.gperf"
      {"from-tag", ParameterTypes::fromTag},
#line 55 "ParameterHash.gperf"
      {"algorithm", ParameterTypes::algorithm},
#line 70 "ParameterHash.gperf"
      {"retry-after", ParameterTypes::retryAfter},
#line 47 "ParameterHash.gperf"
      {"handling", ParameterTypes::handling},
#line 67 "ParameterHash.gperf"
      {"refresher", ParameterTypes::refresher},
#line 76 "ParameterHash.gperf"
      {"filename", ParameterTypes::filename},
#line 79 "ParameterHash.gperf"
      {"boundary", ParameterTypes::boundary},
#line 107 "ParameterHash.gperf"
      {"icid-value", ParameterTypes::icidValue},
#line 12 "ParameterHash.gperf"
      {"mobility", ParameterTypes::mobility},
#line 31 "ParameterHash.gperf"
      {"pub-gruu", ParameterTypes::pubGruu},
#line 66 "ParameterHash.gperf"
      {"early-only", ParameterTypes::earlyOnly},
#line 103 "ParameterHash.gperf"
      {"utran-cell-id-3gpp", ParameterTypes::utranCellId3gpp},
#line 89 "ParameterHash.gperf"
      {"profile-type", ParameterTypes::profileType},
#line 93 "ParameterHash.gperf"
      {"effective-by", ParameterTypes::effectiveBy}
    };

  static const signed char lookup[] =
    {
       -1,  -1,   0,  -1,  -1,   1,  -1,   2,  -1,   3,
        4,  -1,   5,  -1,   6,  -1,   7,  -1,  -1,   8,
        9,  10,  11,  -1,  12,  -1,  -1,  13,  14,  15,
       -1,  -1,  16,  17,  18,  -1,  -1,  -1,  19,  20,
       -1,  -1,  -1,  21,  22,  23,  24,  25,  26,  -1,
       27,  28,  29,  30,  -1,  -1,  31,  32,  33,  34,
       35,  36,  37,  38,  39,  -1,  40,  -1,  -1,  -1,
       -1,  -1,  41,  42,  -1,  -1,  43,  -1,  44,  45,
       46,  -1,  47,  48,  -1,  49,  50,  51,  52,  -1,
       -1,  -1,  -1,  53,  -1,  -1,  54,  -1,  -1,  -1,
       -1,  -1,  -1,  55,  56,  -1,  -1,  57,  -1,  -1,
       58,  -1,  59,  60,  -1,  61,  62,  63,  64,  -1,
       65,  66,  -1,  67,  -1,  -1,  68,  69,  -1,  -1,
       70,  71,  72,  73,  74,  75,  -1,  -1,  -1,  -1,
       76,  77,  78,  -1,  -1,  79,  -1,  80,  81,  -1,
       82,  -1,  -1,  -1,  -1,  83,  -1,  84,  -1,  85,
       -1,  -1,  -1,  -1,  86,  87,  88,  -1,  -1,  -1,
       89,  90,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  91,  -1,  -1,  -1,  -1,  92,  93,
       -1,  -1,  -1,  -1,  -1,  -1,  94,  -1,  95,  96,
       -1,  -1,  -1,  97,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  98,  -1,  -1,  -1,  -1,  -1,  -1,
       99,  -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1, 101,  -1, 102,  -1,  -1, 103,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1, 104,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 105
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
#line 116 "ParameterHash.gperf"

}
