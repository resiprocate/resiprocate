#include <sipstack/ParameterTypes.hxx>

#include <iostream>
using namespace std;

using namespace Vocal2;

ParameterTypes::Factory ParameterTypes::ParameterFactories[MAX_PARAMETER] = {0};
Data ParameterTypes::ParameterNames[MAX_PARAMETER] = {0};

ParameterType<ParameterTypes::transport> Vocal2::p_transport;
ParameterType<ParameterTypes::user> Vocal2::p_user;
ParameterType<ParameterTypes::method> Vocal2::p_method;
ParameterType<ParameterTypes::ttl> Vocal2::p_ttl;
ParameterType<ParameterTypes::maddr> Vocal2::p_maddr;
ParameterType<ParameterTypes::lr> Vocal2::p_lr;
ParameterType<ParameterTypes::q> Vocal2::p_q;
ParameterType<ParameterTypes::purpose> Vocal2::p_purpose;
ParameterType<ParameterTypes::expires> Vocal2::p_expires;
ParameterType<ParameterTypes::handling> Vocal2::p_handling;
ParameterType<ParameterTypes::tag> Vocal2::p_tag;
ParameterType<ParameterTypes::duration> Vocal2::p_duration;
ParameterType<ParameterTypes::branch> Vocal2::p_branch;
ParameterType<ParameterTypes::received> Vocal2::p_received;
ParameterType<ParameterTypes::comp> Vocal2::p_com;
ParameterType<ParameterTypes::rport> Vocal2::p_rport;

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
static unsigned int
hash (register const char *str, register unsigned int len)
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
in_word_set (register const char *str, register unsigned int len)
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

  // stupid search for now...
  for (unsigned int i = 0; i < sizeof(pwordlist)/sizeof(pwordlist[0]); i++)
  {
     cerr << "trying: " << pwordlist[i].name << endl;
     if (!strncasecmp(pwordlist[i].name, str, len))
     {
        return &pwordlist[i];
     }
  }
  return 0;
  
  cerr << "[";
  cerr.write(str, len);
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);
      cerr << "] = " << key << endl;

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = pwordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &pwordlist[key];
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

