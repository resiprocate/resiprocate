
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
        typedef struct { const char * name; size_t sz; } ApiEntry;
        ApiCheck(ApiEntry * list);
};
};
#endif


// end of ApiCheck
