#if !defined(RESIP_METHODSHASH_HXX)
#define RESIP_METHODSHASH_HXX
namespace resip 
{

struct methods { char *name; MethodTypes type; };
/* maximum key range = 494, duplicates = 0 */

class MethodHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct methods *in_word_set (const char *str, unsigned int len);
};
// NOTE the cxx file for this class is AUTO GENERATED. DO NOT EDIT IT.
// This file should match it. BUT THIS FILE IS MANUALLY GENERATED.
}

#endif
