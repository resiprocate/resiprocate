#if !defined(compat_hxx)
#define compat_hxx

#include <strings.h>

#if defined(WIN32) || defined(__QNX__)
#define strcasecmp(a,b) stricmp(a,b)
#define strncasecmp(a,b,c) strnicmp(a,b,c)
#endif

// perhaps not the best thing to do here
#include <util/Data.hxx>

namespace Vocal2
{

// do a case-insensitive match
inline bool isEqualNoCase(const Data& left, const Data& right)
{
   return (strcasecmp(left.c_str(), right.c_str()) == 0);
}
 
}

#endif
