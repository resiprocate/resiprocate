#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"

#ifdef USE_SSL

#include "rutil/ssl/OpenSSLInit.hxx"

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

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
Mutex* OpenSSLInit::mMutexes;

bool
OpenSSLInit::init()
{
	static OpenSSLInit instance;
	return true;
}

OpenSSLInit::OpenSSLInit()
{
	int locks = CRYPTO_num_locks();
	mMutexes = new Mutex[locks];
	CRYPTO_set_locking_callback(::resip_OpenSSLInit_lockingFunction);

#if !defined(WIN32)
#if defined(_POSIX_THREADS)
	CRYPTO_set_id_callback(::resip_OpenSSLInit_threadIdFunction);
#else
#error Cannot set OpenSSL up to be threadsafe!
#endif
#endif

#if 0 //?dcm? -- not used by OpenSSL yet?
	CRYPTO_set_dynlock_create_callback(::resip_OpenSSLInit_dynCreateFunction);
	CRYPTO_set_dynlock_destroy_callback(::resip_OpenSSLInit_dynDestroyFunction);
	CRYPTO_set_dynlock_lock_callback(::resip_OpenSSLInit_dynLockFunction);
#endif

	CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	resip_assert(EVP_des_ede3_cbc());
   mInitialized = true;
}

OpenSSLInit::~OpenSSLInit()
{
   mInitialized = false;
   ERR_remove_state(0);// free thread error queue
   EVP_cleanup();// Clean up data allocated during OpenSSL_add_all_algorithms
   CRYPTO_cleanup_all_ex_data();
   ERR_free_strings();// Clean up data allocated during SSL_load_error_strings
   sk_SSL_COMP_free (SSL_COMP_get_compression_methods()); 

//	CRYPTO_mem_leaks_fp(stderr);

	delete [] mMutexes;
}

void
resip_OpenSSLInit_lockingFunction(int mode, int n, const char* file, int line)
{
   if(!resip::OpenSSLInit::mInitialized) return;
   if (mode & CRYPTO_LOCK)
   {
      OpenSSLInit::mMutexes[n].lock();
   }
   else
   {      
      OpenSSLInit::mMutexes[n].unlock();
   }
}

unsigned long 
resip_OpenSSLInit_threadIdFunction()
{
#if defined(WIN32)
   resip_assert(0);
#else
#ifndef _POSIX_THREADS
   resip_assert(0);
#endif
   unsigned long ret;
   ret= (unsigned long)pthread_self();
   return ret;
#endif
   return 0;
}

CRYPTO_dynlock_value* 
resip_OpenSSLInit_dynCreateFunction(char* file, int line)
{
   CRYPTO_dynlock_value* dynLock = new CRYPTO_dynlock_value;
   dynLock->mutex = new Mutex;
   return dynLock;   
}

void
resip_OpenSSLInit_dynDestroyFunction(CRYPTO_dynlock_value* dynlock, const char* file, int line)
{
   delete dynlock->mutex;
   delete dynlock;
}

void 
resip_OpenSSLInit_dynLockFunction(int mode, struct CRYPTO_dynlock_value* dynlock, const char* file, int line)
{
   if (mode & CRYPTO_LOCK)
   {
      dynlock->mutex->lock();
   }
   else
   {
      dynlock->mutex->unlock();
   }
}

#endif
