
// This is intentionally without a namespace.
// To use this intelligently, include it from within
// a reasonable namespace (resip::) in the stack and
// whatever your client namespace is if you want
// a list in your application.


#if !defined(RESIP_API_CHECK_HXX)
#define RESIP_API_CHECK_HXX

#include <unistd.h>

namespace resip
{
class ApiCheck
{
    public:
        typedef struct { const char * name; size_t sz; const char * culprits;} ApiEntry;
        ApiCheck(ApiEntry * list, int len);
};
};

#endif


// end of ApiCheck
