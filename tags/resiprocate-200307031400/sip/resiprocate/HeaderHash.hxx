#if !defined(HEADERHASH_HXX)
#define HEADERHASH_HXX
namespace resip 
{

struct headers { char *name; Headers::Type type; };
/* maximum key range = 494, duplicates = 0 */

class HeaderHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct headers *in_word_set (const char *str, unsigned int len);
};
// NOTE the cxx file for this class is AUTO GENERATED. DO NOT EDIT IT.
// This file should match it. BUT THIS FILE IS MANUALLY GENERATED.
}

#endif
