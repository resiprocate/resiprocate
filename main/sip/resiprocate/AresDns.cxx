#include "resiprocate/AresDns.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

#if !defined(WIN32)
#include <arpa/nameser.h>
#endif

extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}

#if !defined(USE_ARES)
#error Must have ARES
#endif


using namespace resip;

int 
AresDns::init()
{
   int status;
   if ((status = ares_init(&mChannel)) != ARES_SUCCESS)
   {
      return status;
   }
   else
   {
      return 0;
   }
}

AresDns::~AresDns()
{
   ares_destroy(mChannel);
}

void 
AresDns::lookupARecords(const char* target, ExternalDnsHandler* handler, void* userData)
{
   ares_gethostbyname(mChannel, target, AF_INET, AresDns::aresHostCallback, new Payload(handler, userData));
}

void 
AresDns::lookupAAAARecords(const char* target, ExternalDnsHandler* handler, void* userData)
{
   ares_query(mChannel, target, C_IN, T_AAAA, AresDns::aresAAAACallback, new Payload(handler, userData)); 
}

void 
AresDns::lookupNAPTR(const char* target, ExternalDnsHandler* handler, void* userData)
{
   ares_query(mChannel, target, C_IN, T_NAPTR, AresDns::aresNAPTRCallback, new Payload(handler, userData)); 
}

void 
AresDns::lookupSRV(const char* target, ExternalDnsHandler* handler, void* userData)    
{
   ares_query(mChannel, target, C_IN, T_SRV, AresDns::aresSRVCallback, new Payload(handler, userData)); 
}

ExternalDnsHandler* 
AresDns::getHandler(void* arg)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   ExternalDnsHandler *thisp = reinterpret_cast<ExternalDnsHandler*>(p->first);
   return thisp;
}

ExternalDnsRawResult 
AresDns::makeRawResult(void *arg, int status, unsigned char *abuf, int alen)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   void* userArg = reinterpret_cast<void*>(p->second);
   
   if (status != ARES_SUCCESS)
   {
      return ExternalDnsRawResult(status, userArg);
   }
   else
   {
      return ExternalDnsRawResult(abuf, alen, userArg);
   }
}

void
AresDns::aresHostCallback(void *arg, int status, struct hostent* result)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   ExternalDnsHandler *thisp = reinterpret_cast<ExternalDnsHandler*>(p->first);
   void* userArg = reinterpret_cast<void*>(p->second);

   if (status != ARES_SUCCESS)
   {
      thisp->handle_host(ExternalDnsHostResult(status, userArg));
   }
   else
   {
      thisp->handle_host(ExternalDnsHostResult(result, userArg));
   }
}

void
AresDns::aresNAPTRCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   getHandler(arg)->handle_NAPTR(makeRawResult(arg, status, abuf, alen));
}

void
AresDns::aresSRVCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   getHandler(arg)->handle_SRV(makeRawResult(arg, status, abuf, alen));
}

void
AresDns::aresAAAACallback(void *arg, int status, unsigned char *abuf, int alen)
{
   getHandler(arg)->handle_AAAA(makeRawResult(arg, status, abuf, alen));
}                             
      
bool 
AresDns::requiresProcess()
{
   return true; 
}

void 
AresDns::buildFdSet(fd_set& read, fd_set& write, int& size)
{
   int newsize = ares_fds(mChannel, &read, &write);
   if ( newsize > size )
   {
      size = newsize;
   }
}

void 
AresDns::process(fd_set& read, fd_set& write)
{
   ares_process(mChannel, &read, &write);
}

char* 
AresDns::errorMessage(long errorCode)
{
   const char* aresMsg = ares_strerror(errorCode);

   int len = strlen(aresMsg);
   char* errorString = new char[len];

   strncpy(errorString, aresMsg, len);
   return errorString;
}






