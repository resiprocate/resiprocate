#include <sipstack/ParameterTypes.hxx>
#include <util/compat.hxx>
#include <iostream>

int strncasecmp(char*,char*,int);

using namespace std;

using namespace Vocal2;

ParameterTypes::Factory ParameterTypes::ParameterFactories[ParameterTypes::MAX_PARAMETER] = {0};
Data ParameterTypes::ParameterNames[ParameterTypes::MAX_PARAMETER] = {""};

Transport_Param Vocal2::p_transport;
User_Param Vocal2::p_user;
Method_Param Vocal2::p_method;
Ttl_Param Vocal2::p_ttl;
Maddr_Param Vocal2::p_maddr;
Lr_Param Vocal2::p_lr;
Q_Param Vocal2::p_q;
Purpose_Param Vocal2::p_purpose;
Expires_Param Vocal2::p_expires;
Handling_Param Vocal2::p_handling;
Tag_Param Vocal2::p_tag;
ToTag_Param Vocal2::p_toTag;
FromTag_Param Vocal2::p_fromTag;
Duration_Param Vocal2::p_duration;
Branch_Param Vocal2::p_branch;
Received_Param Vocal2::p_received;
Mobility_Param Vocal2::p_mobility;
Comp_Param Vocal2::p_com;
Rport_Param Vocal2::p_rport;


// to generate the perfect hash:
// call tolower() on instances of the source string
// change strcmp to strncasecmp and pass len-1
// will NOT work for non alphanum chars 

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' parameters.gperf  */
struct params { char *name; ParameterTypes::Type type; };

#define TOTAL_KEYWORDS 19
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 53
/* maximum key range = 53, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54,  0, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54,  0, 10,  0,
       0,  0, 20,  5,  0,  0, 54, 54,  0, 20,
       0,  0,  0,  0,  0,  0,  0, 15, 20, 54,
       5,  0, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54
    };
  register int hval = len;
  switch (hval)
    {
      default:
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

#ifdef __GNUC__
__inline
#endif
struct params *
in_word_set (register const char *str, register unsigned int len)
{
  static struct params wordlist[] =
    {
      {""},
      {"q", ParameterTypes::q},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {""},
      {"rport", ParameterTypes::rport},
      {""}, {""},
      {"tag", ParameterTypes::tag},
      {"transport", ParameterTypes::transport},
      {""},
      {"to-tag", ParameterTypes::toTag},
      {"expires", ParameterTypes::expires},
      {"handling", ParameterTypes::handling},
      {""}, {""},
      {"branch", ParameterTypes::branch},
      {""}, {""},
      {"user", ParameterTypes::user},
      {""}, {""},
      {"purpose", ParameterTypes::purpose},
      {"duration", ParameterTypes::duration},
      {"comp", ParameterTypes::comp},
      {"maddr", ParameterTypes::maddr},
      {"method", ParameterTypes::method},
      {""},
      {"received", ParameterTypes::received},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"mobility", ParameterTypes::mobility},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"from-tag", ParameterTypes::fromTag}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
            return &wordlist[key];
        }
    }
  return 0;
}

ParameterTypes::Type
ParameterTypes::getType(const char* name, unsigned int len)
{
   struct params* p;
   p = in_word_set(name, len);
   return p ? p->type : ParameterTypes::UNKNOWN;
}



