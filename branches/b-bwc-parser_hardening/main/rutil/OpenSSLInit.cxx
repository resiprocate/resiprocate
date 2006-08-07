#include "rutil/OpenSSLInit.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"

#ifdef USE_SSL

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
using namespace std;

#include <iostream>

static bool invokeOpenSSLInit = OpenSSLInit::init(); //.dcm. - only in hxx
Mutex* OpenSSLInit::mMutexes;

bool
OpenSSLInit::init()
{
	static OpenSSLInit instance;
	return true;
}

OpenSSLInit::OpenSSLInit()
{
	cerr << "OpenSSLInit::init() invoked" << endl;

	//#if defined(THREADS)
	int locks = CRYPTO_num_locks();
	//		ErrLog(<< "Creating " << locks << " locks for OpenSSL");
	//for(int i=0; i < locks; i++)
	//{
	//	OpenSSLInit::mMutexes.push_back(new Mutex());
	//}
	mMutexes = new Mutex[locks];
	CRYPTO_set_locking_callback(OpenSSLInit::lockingFunction);
#if !defined(WIN32) && defined(PTHREADS)
	CRYPTO_set_id_callback(OpenSSLInit::threadIdFunction);
#endif

#if 0 //?dcm? -- not used by OpenSSL yet?
	CRYPTO_set_dynlock_create_callback(OpenSSLInit::dynCreateFunction);
	CRYPTO_set_dynlock_destroy_callback(OpenSSLInit::dynDestroyFunction);
	CRYPTO_set_dynlock_lock_callback(OpenSSLInit::dynLockFunction);
#endif
	//#endif

#if 1
	CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#endif

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	assert(EVP_des_ede3_cbc());
}

OpenSSLInit::~OpenSSLInit()
{
	// Clean up data allocated during SSL_load_error_strings
	ERR_free_strings();

	// free thread error queue
	ERR_remove_state(0);

	CRYPTO_cleanup_all_ex_data();

	// Clean up data allocated during OpenSSL_add_all_algorithms
	EVP_cleanup();

#if 1
	CRYPTO_mem_leaks_fp(stderr);
#endif

	delete [] mMutexes;
}

void
OpenSSLInit::lockingFunction(int mode, int n, const char* file, int line)
{
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
OpenSSLInit::threadIdFunction()
{
#if defined(WIN32)
   assert(0);
#else
#ifndef PTHREADS
   assert(0);
#endif
   unsigned long ret;
   ret= (unsigned long)pthread_self();
   return ret;
#endif
   return 0;
}

CRYPTO_dynlock_value* 
OpenSSLInit::dynCreateFunction(char* file, int line)
{
   CRYPTO_dynlock_value* dynLock = new CRYPTO_dynlock_value;
   ErrLog(<< "OpenSSLInit::dynCreateFunction: " << file << "::" << line << " DynLock: " << dynLock);
   dynLock->mutex = new Mutex;
   return dynLock;   
}

void
OpenSSLInit::dynDestroyFunction(CRYPTO_dynlock_value* dynlock, const char* file, int line)
{
   ErrLog(<< "OpenSSLInit::dynDestroyFunction: " << file << "::" << line << " DynLock: " << dynlock);
   delete dynlock->mutex;
   delete dynlock;
}

void 
OpenSSLInit::dynLockFunction(int mode, struct CRYPTO_dynlock_value* dynlock, const char* file, int line)
{
   ErrLog(<< "OpenSSLInit::dynlockFunction: " << file << "::" << line << " Lock " << dynlock << " Mode: " << mode);
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
