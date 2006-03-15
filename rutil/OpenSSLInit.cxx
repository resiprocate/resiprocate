#include "rutil/OpenSSLInit.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"

#include "rutil/Logger.hxx"

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>


#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::RUTIL

using namespace resip;
using namespace std;

static bool invokeOpenSSLInit = OpenSSLInit::init();
vector<Mutex> OpenSSLInit::mMutexes;

bool
OpenSSLInit::init()
{
#if defined(THREADS)
   OpenSSLInit::mMutexes.resize(CRYPTO_num_locks);   
   CRYPTO_set_locking_callback(OpenSSLInit::lockingFunction);
#if !defined(WIN32)
   CRYPTO_set_id_callback(OpenSSLInit::threadIdFunction);
#endif

#if 0 //?dcm? -- not used by OpenSSL yet?
   CRYPTO_set_dynlock_create_callback(OpenSSLInit::dynCreateFunction);
   CRYPTO_set_dynlock_destroy_callback(OpenSSLInit::dynDestroyFunction);
   CRYPTO_set_dynlock_lock_callback(OpenSSLInit::dynLockFunction);
#endif
#endif
   return true;
}

void
OpenSSLInit::lockingFunction(int mode, int n, const char* file, int line)
{
   ErrLog(<< "OpenSSLInit::lockingFunction: " << file << "::" << line << " Mutex# " << n << " Mode: " << mode);
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
