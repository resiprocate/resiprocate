#include <sipstack/ParameterTypes.hxx>

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
Comp_Param Vocal2::p_com;
Rport_Param Vocal2::p_rport;

///// Massaging required:
///// 1. use not 0x20 as bit mask in character comparisons
///// 2. use strncasecmp in string compare -- pass len-1

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' parameters.gperf  */
struct params { char *name; ParameterTypes::Type type; };

#define TOTAL_KEYWORDS 18
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 38
/* maximum key range = 38, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
unsigned int
p_hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39,  5, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39,  0, 10, 10,
       0,  0,  0,  0,  0,  0, 39, 39, 15, 15,
       0,  0,  0,  0,  0,  0,  0,  0, 20, 39,
       5, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
      39, 39, 39, 39, 39, 39
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

#ifdef __GNUC__
__inline
#endif
struct params *
p_in_word_set (register const char *str, register unsigned int len)
{
  static struct params pwordlist[] =
     {
        {""},
        {"q",ParameterTypes::q},
        {""},
        {"tag",ParameterTypes::tag},
        {"user",ParameterTypes::user},
        {"rport",ParameterTypes::rport},
        {""},
        {"purpose",ParameterTypes::purpose},
        {"duration",ParameterTypes::duration},
        {"transport",ParameterTypes::transport},
        {""},
        {"to-tag",ParameterTypes::toTag},
        {"expires",ParameterTypes::expires},
        {""}, {""}, {""}, {""},
        {"lr",ParameterTypes::lr},
        {"ttl",ParameterTypes::ttl},
        {""},
        {"maddr",ParameterTypes::maddr},
        {"method",ParameterTypes::method},
        {""},
        {"handling",ParameterTypes::handling},
        {""}, {""},
        {"branch",ParameterTypes::branch},
        {""},
        {"from-tag",ParameterTypes::fromTag},
        {"comp",ParameterTypes::comp},
        {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
        {"received",ParameterTypes::received}
     };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
  {
     register int key = p_hash (str, len);
     if (key <= MAX_HASH_VALUE && key >= 0)
     {
        register const char *s = pwordlist[key].name;
        
        if (*str == *s && !strncmp (str + 1, s + 1, len-1))
        {
           return &pwordlist[key];
        }
     }
  }
  
  return 0;
}

ParameterTypes::Type
ParameterTypes::getType(const char* name, unsigned int len)
{
   struct params* p;
   p = p_in_word_set(name, len);
   return p ? p->type : ParameterTypes::UNKNOWN;
}



