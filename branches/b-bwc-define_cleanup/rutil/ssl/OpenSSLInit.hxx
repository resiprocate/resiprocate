#if !defined(RESIP_OPENSSLINIT_HXX)
#define RESIP_OPENSSLINIT_HXX 

#include "rutil/Mutex.hxx"
#include "rutil/RWMutex.hxx"
#include "rutil/Data.hxx"
#include <cassert>
#include <vector>

// This will not be built or installed if USE_SSL is not defined; if you are 
// building against a source tree, and including this, and getting linker 
// errors, the source tree was probably built with this flag off. Either stop
// including this file, or re-build the source tree with SSL enabled.
//#ifdef USE_SSL

struct CRYPTO_dynlock_value
{
      resip::Mutex* mutex;
};

namespace resip
{

class OpenSSLInit
{
   public:
      static bool init();
   private:
	   OpenSSLInit();
	   ~OpenSSLInit();
      static void lockingFunction(int mode, int n, const char* file, int line);
      static unsigned long threadIdFunction();
      static CRYPTO_dynlock_value* dynCreateFunction(char* file, int line);
      static void dynDestroyFunction(CRYPTO_dynlock_value*, const char* file, int line);
      static void dynLockFunction(int mode, struct CRYPTO_dynlock_value*, const char* file, int line);
      static Mutex* mMutexes;    
      static volatile bool mInitialized;
};
static bool invokeOpenSSLInit = OpenSSLInit::init();

}

//#endif

#endif
