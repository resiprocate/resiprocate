#if !defined(RESIP_EXTERNAL_DNS_HXX)
#define RESIP_EXTERNAL_DNS_HXX

#include "resiprocate/external/AsyncID.hxx"

struct hostent;

namespace resip
{
class ExternalDnsHandler;
class ExternalDnsRawResult;
class ExternalDnsHostResult;

//used by the asynchronous executive
class ExternalDns
{
   public:
      //returns 0 for success, otherwise message can be pulled from errorMessage
      virtual int init() = 0; 

      virtual ~ExternalDns() {};

      //null terminated strings; lifespan of strings passed in is not guaranteed
      //beyond the duration of the call.  
      virtual void lookupARecords(const char* target, ExternalDnsHandler* handler, void* userData) = 0;
      virtual void lookupAAAARecords(const char* target, ExternalDnsHandler* handler, void* userData) = 0;
      virtual void lookupNAPTR(const char* target, ExternalDnsHandler* handler, void* userData) = 0;
      virtual void lookupSRV(const char* target, ExternalDnsHandler* handler, void* userData) = 0;

      //only call buildFdSet and process if requiresProcess is true.  
      virtual bool requiresProcess() = 0;

      //this is scary on windows; the standard way to get a bigger fd_set is to
      //redefine FD_SETSIZE befor each inclusion of winsock2.h, so make sure
      //external libraries have been properly configured      
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size) = 0;
      virtual void process(fd_set& read, fd_set& write) = 0;

      virtual void freeResult(ExternalDnsRawResult res) = 0;
      virtual void freeResult(ExternalDnsHostResult res) = 0;

      //caller must clean up memory
      virtual char* errorMessage(long errorCode) = 0;
};
 
class ExternalDnsResult : public AsyncResult
{
   public:
      ExternalDnsResult(long errorCode, void* uData) : AsyncResult(errorCode) , userData(uData) {}
      ExternalDnsResult(void* uData) : userData(uData) {}
      void* userData;
};

//should this be nested?
class ExternalDnsRawResult : public ExternalDnsResult
{
   public:
      ExternalDnsRawResult(unsigned char* buf, int len, void* uData) : 
         ExternalDnsResult(uData),

         abuf(buf),
         alen(len) 
      {}
      ExternalDnsRawResult(long errorCode, void* uData) : ExternalDnsResult(errorCode, uData) {}
         
      unsigned char* abuf;
      int alen;
};

class ExternalDnsHostResult : public ExternalDnsResult
{
   public:
      ExternalDnsHostResult(hostent* h, void* uData) :
         ExternalDnsResult(uData), 
         host(h)
      {}
      ExternalDnsHostResult(long errorCode, void* uData) : ExternalDnsResult(errorCode, uData) {}
         
      hostent* host;
};

class ExternalDnsHandler
{
   public:
      //underscores are against convention, but pretty impossible to read
      //otherwise. ?dcm? -- results stack or heap? 
      //the free routines can be dealt w/ iheritence instead if pointers are used
      virtual void handle_NAPTR(ExternalDnsRawResult res) = 0;
      virtual void handle_SRV(ExternalDnsRawResult res) = 0;
      virtual void handle_AAAA(ExternalDnsRawResult res) = 0;
      virtual void handle_host(ExternalDnsHostResult res) = 0;
};

}

#endif
      
