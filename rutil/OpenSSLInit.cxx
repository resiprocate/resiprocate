#ifdef USE_SSL

#include "rutil/OpenSSLInit.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"

#include "rutil/Logger.hxx"

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

static bool invokeOpenSSLInit = OpenSSLInit::init();
Mutex* OpenSSLInit::mMutexes;

static bool openSSLInitInvoked = false;

bool
OpenSSLInit::init()
{
   
	if (!openSSLInitInvoked)
	{
       cerr << "OpenSSLInit::init() invoked" << endl;
       
		openSSLInitInvoked = true;
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

        
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        assert(EVP_des_ede3_cbc());
    }
   return true;
}

//.dcm. -- should prob. cleanup in a static guard object...prob. doesn't matter
// Clean up data allocated during OpenSSL_add_all_algorithms
//   EVP_cleanup();       

// Clean up data allocated during SSL_load_error_strings
//   ERR_free_strings();



void
OpenSSLInit::lockingFunction(int mode, int n, const char* file, int line)
{
//   StackLog(<< "OpenSSLInit::lockingFunction: " << file << "::" << line << " Mutex# " << n << " Mode: " << mode);
   if (mode & CRYPTO_LOCK)
   {
      StackLog(<< "OpenSSLInit::lockingFunction, locking: " << file << "::" << line << " Mutex# " << n << " Mode: " << mode);
      OpenSSLInit::mMutexes[n].lock();
   }
   else
   {      
      StackLog(<< "OpenSSLInit::lockingFunction, unlocking: " << file << "::" << line << " Mutex# " << n << " Mode: " << mode);
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
