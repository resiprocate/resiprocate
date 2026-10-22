#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "rutil/ResipAssert.h"

#ifdef USE_SSL

#include "rutil/ssl/OpenSSLInit.hxx"

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER < 0x1010100fL
#error resip requires OpenSSL 1.1.1 or later
#endif

#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

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
