#if !defined(PARAMETERHASH_HXX)
#define PARAMETERHASH_HXX
namespace resip 
{
using namespace resip;

struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 494, duplicates = 0 */

class ParameterHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct params *in_word_set (const char *str, unsigned int len);
};
// NOTE the cxx file for this class is AUTO GENERATED. DO NOT EDIT IT.
// This file should match it. BUT THIS FILE IS MANUALLY GENERATED.
}

#endif
