/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf --enum -E -L C++ -t -k '*' -D -Z MethodHash MethodHash.gperf  */
#include <string.h>
#include <ctype.h>
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/MethodHash.hxx"

using namespace resip;

inline unsigned int
MethodHash::hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19,  0,  0,  0, 19,  0,
       0, 10, 19,  0, 19, 10,  5,  0,  0,  0,
       0, 19,  0,  0,  0,  0, 10, 19, 19,  0,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
      19, 19, 19, 19, 19, 19
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct methods *
MethodHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 12,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 9,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 18
    };

  static struct methods wordlist[] =
    {
      {"BYE", BYE},
      {"INFO", INFO},
      {"REFER", REFER},
      {"NOTIFY", NOTIFY},
      {"OPTIONS", OPTIONS},
      {"RESPONSE", RESPONSE},
      {"SUBSCRIBE", SUBSCRIBE},
      {"CANCEL", CANCEL},
      {"ACK", ACK},
      {"INVITE", INVITE},
      {"MESSAGE", MESSAGE},
      {"REGISTER", REGISTER}
    };

  static signed char lookup[] =
    {
      -1, -1, -1,  0,  1,  2,  3,  4,  5,  6, -1,  7, -1,  8,
      -1, -1,  9, 10, 11
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

               if ((*str) == *s && !strncmp( str + 1, s + 1, len - 1))
                return &wordlist[index];
            }
        }
    }
  return 0;
}
