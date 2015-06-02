/* C++ code produced by gperf version 3.0.3 */
/* Command-line: gperf -C -D -E -L C++ -t --key-positions='*' --compare-strncmp -Z MonthHash MonthHash.gperf  */

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

#line 1 "MonthHash.gperf"

#include <string.h>
#include <ctype.h>
#include "resip/stack/DateCategory.hxx"

namespace resip
{
#line 9 "MonthHash.gperf"
struct months { const char *name; Month type; };
/* maximum key range = 41, duplicates = 0 */

class MonthHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct months *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
MonthHash::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 10, 44, 44,  4, 44,
       4, 44, 44, 44,  0, 44, 44,  5, 15, 15,
      44, 44, 44, 15, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44,  5,  0, 10,
      44, 10, 44, 15, 44, 44, 44, 44,  4, 44,
       0, 15,  5, 44,  5, 44, 10,  0, 10, 44,
      44,  0, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
      44, 44, 44, 44, 44, 44
    };
  return len + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

const struct months *
MonthHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 12,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 3,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 43
    };

  static const struct months wordlist[] =
    {
#line 16 "MonthHash.gperf"
      {"Jun", Jun},
#line 17 "MonthHash.gperf"
      {"Jul", Jul},
#line 11 "MonthHash.gperf"
      {"Jan", Jan},
#line 15 "MonthHash.gperf"
      {"May", May},
#line 12 "MonthHash.gperf"
      {"Feb", Feb},
#line 13 "MonthHash.gperf"
      {"Mar", Mar},
#line 14 "MonthHash.gperf"
      {"Apr", Apr},
#line 22 "MonthHash.gperf"
      {"Dec", Dec},
#line 18 "MonthHash.gperf"
      {"Aug", Aug},
#line 19 "MonthHash.gperf"
      {"Sep", Sep},
#line 20 "MonthHash.gperf"
      {"Oct", Oct},
#line 21 "MonthHash.gperf"
      {"Nov", Nov}
    };

  static const signed char lookup[] =
    {
      -1, -1, -1,  0, -1, -1, -1,  1,  2, -1, -1, -1, -1,  3,
      -1, -1, -1,  4,  5, -1, -1, -1, -1,  6, -1, -1, -1,  7,
       8, -1, -1, -1, -1,  9, -1, -1, -1, -1, 10, -1, -1, -1,
      -1, 11
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
#line 23 "MonthHash.gperf"

}
