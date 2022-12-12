#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"

#ifdef USE_SSL

#include "rutil/ssl/OpenSSLInit.hxx"

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER < 0x1010100fL
#error OpenSSL 1.1.1 or later is required
#endif

#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#if  defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1900)
// OpenSSL builds use an older version of visual studio that require the following definition
// Also will need to link with legacy_stdio_definitions.lib.  It's possible that future build of 
// SL's windows OpenSSL binaries will be built with VS2015 and will not require this, however it shouldn't
// hurt to be here.
// http://stackoverflow.com/questions/30412951/unresolved-external-symbol-imp-fprintf-and-imp-iob-func-sdl2
extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
using namespace std;

#include <iostream>

static bool invokeOpenSSLInit = OpenSSLInit::init(); //.dcm. - only in hxx
volatile bool OpenSSLInit::mInitialized = false;

bool
OpenSSLInit::init()
{
	static OpenSSLInit instance;
	return true;
}

OpenSSLInit::OpenSSLInit()
{
/* The OpenSSL memory leak checking has been deprecated since
   OpenSSL v3.0.  OpenSSL developers recommend that we rely
   on modern compilers to provide the same functionality. */
#if defined(LIBRESSL_VERSION_NUMBER)
	CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
#elif (OPENSSL_VERSION_NUMBER < 0x30000000L)
	CRYPTO_set_mem_debug(1);
#endif

#if (OPENSSL_VERSION_NUMBER < 0x30000000L) || defined(LIBRESSL_VERSION_NUMBER)
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#endif

	resip_assert(EVP_des_ede3_cbc());
   mInitialized = true;
}

OpenSSLInit::~OpenSSLInit()
{
   mInitialized = false;

//	CRYPTO_mem_leaks_fp(stderr);
}

#endif
