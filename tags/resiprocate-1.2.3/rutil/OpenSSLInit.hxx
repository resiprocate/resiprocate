#if !defined(RESIP_OPENSSLINIT_HXX)
#define RESIP_OPENSSLINIT_HXX 

#include "rutil/Mutex.hxx"
#include "rutil/RWMutex.hxx"
#include "rutil/Data.hxx"
#include <cassert>
#include <vector>

#ifdef USE_SSL

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
};
static bool invokeOpenSSLInit = OpenSSLInit::init();

}

#endif

#endif
