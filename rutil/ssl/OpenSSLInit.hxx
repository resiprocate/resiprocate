#if !defined(RESIP_OPENSSLINIT_HXX)
#define RESIP_OPENSSLINIT_HXX 

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

// This will not be built or installed if USE_SSL is not defined; if you are
// building against a source tree, and including this, and getting linker
// errors, the source tree was probably built with this flag off. Either stop
// including this file, or re-build the source tree with SSL enabled.
//#ifdef USE_SSL

namespace resip
{

class OpenSSLInit
{
   public:
      static bool init();
   private:
	   OpenSSLInit();
	   ~OpenSSLInit();

      static volatile bool mInitialized;
};
static bool invokeOpenSSLInit = OpenSSLInit::init();

}

//#endif

#endif
