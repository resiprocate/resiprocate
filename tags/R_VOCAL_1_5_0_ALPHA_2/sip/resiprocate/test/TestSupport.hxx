#if !defined(TESTSUPPORT_HXX)
#define TESTSUPPORT_HXX
#include <sys/types.h>
#include "sip2/util/Data.hxx"
#include "sip2/sipstack/SipMessage.hxx"

namespace Vocal2 {

   class TestSupport
   {
      public:
         static void prettyPrint(const char *buffer, size_t length);
         static Data showN(const char *buffer, size_t length);
         static SipMessage* makeMessage(const Data& data, bool isExternal = false);
   };
};
#endif
