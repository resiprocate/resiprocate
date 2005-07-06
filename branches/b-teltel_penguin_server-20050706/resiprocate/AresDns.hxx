#if !defined(RESIP_ARES_DNS_HXX)
#define RESIP_ARES_DNS_HXX

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/external/ExternalDns.hxx"

extern "C"
{
struct ares_channeldata;
}

//struct fd_set;

namespace resip
{
class AresDns : public ExternalDns
{
   public:
      AresDns() {}
      virtual ~AresDns();

      virtual int init(); 
      void lookupARecords(const char* target, ExternalDnsHandler* handler, void* userData);
      void lookupAAAARecords(const char* target, ExternalDnsHandler* handler, void* userData);
      void lookupNAPTR(const char* target, ExternalDnsHandler* handler, void* userData);
      void lookupSRV(const char* target, ExternalDnsHandler* handler, void* userData);    

      virtual bool requiresProcess();
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size);
      virtual void process(fd_set& read, fd_set& write);

      //?dcm?  I believe these need to do nothing in the ARES case.
      virtual void freeResult(ExternalDnsRawResult /* res */) {}
      virtual void freeResult(ExternalDnsHostResult /* res */) {}

      virtual char* errorMessage(long errorCode);

   private:
      static void aresNAPTRCallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresSRVCallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresAAAACallback(void *arg, int status, unsigned char *abuf, int alen);
      static void aresHostCallback(void *arg, int status, struct hostent* result);

      typedef std::pair<ExternalDnsHandler*, void*> Payload;
      static ExternalDnsRawResult makeRawResult(void *arg, int status, unsigned char *abuf, int alen);
      static ExternalDnsHandler* getHandler(void* arg);

	  struct ares_channeldata* mChannel;
};
   
}


#endif
