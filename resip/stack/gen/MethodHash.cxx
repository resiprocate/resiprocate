/* C++ code produced by gperf version 3.0.3 */
/* Command-line: gperf -C -D -E -L C++ -t --key-positions='*' --compare-strncmp -Z MethodHash MethodHash.gperf  */

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

#line 1 "MethodHash.gperf"

#include <string.h>
#include <ctype.h>
#include "resip/stack/MethodTypes.hxx"

namespace resip
{
#line 9 "MethodHash.gperf"
struct methods { const char *name; MethodTypes type; };
/* maximum key range = 31, duplicates = 0 */

class MethodHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct methods *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
MethodHash::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35,  5, 10,  5,  5,  0,
       0, 15,  0,  0, 35,  0,  0,  0,  0,  0,
       0, 35,  0,  0,  0,  0,  0, 35, 35,  5,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
      35, 35, 35, 35, 35, 35
    };
  register int hval = len;

  switch (hval)
    {
      default:
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

const struct methods *
MethodHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 16,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 9,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 34
    };

  static const struct methods wordlist[] =
    {
#line 24 "MethodHash.gperf"
      {"INFO", INFO},
#line 19 "MethodHash.gperf"
      {"REFER", REFER},
#line 14 "MethodHash.gperf"
      {"INVITE", INVITE},
#line 16 "MethodHash.gperf"
      {"OPTIONS", OPTIONS},
#line 22 "MethodHash.gperf"
      {"RESPONSE", RESPONSE},
#line 15 "MethodHash.gperf"
      {"NOTIFY", NOTIFY},
#line 25 "MethodHash.gperf"
      {"SERVICE", SERVICE},
#line 11 "MethodHash.gperf"
      {"ACK", ACK},
#line 17 "MethodHash.gperf"
      {"PRACK", PRACK},
#line 26 "MethodHash.gperf"
      {"UPDATE", UPDATE},
#line 18 "MethodHash.gperf"
      {"PUBLISH", PUBLISH},
#line 12 "MethodHash.gperf"
      {"BYE", BYE},
#line 13 "MethodHash.gperf"
      {"CANCEL", CANCEL},
#line 20 "MethodHash.gperf"
      {"REGISTER", REGISTER},
#line 23 "MethodHash.gperf"
      {"MESSAGE", MESSAGE},
#line 21 "MethodHash.gperf"
      {"SUBSCRIBE", SUBSCRIBE}
    };

  static const signed char lookup[] =
    {
      -1, -1, -1, -1,  0,  1,  2,  3,  4, -1, -1,  5,  6,  7,
      -1,  8,  9, 10, 11, -1, -1, 12, -1, 13, -1, -1, -1, 14,
      -1, -1, -1, -1, -1, -1, 15
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

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 27 "MethodHash.gperf"

}
