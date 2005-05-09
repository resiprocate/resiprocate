#ifndef RESIP_CurlHttpProvider
#define RESIP_CurlHttpProvider


#include "resiprocate/external/HttpProvider.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/GenericUri.hxx"
#include "resiprocate/os/Data.hxx"

namespace resip
{

class CurlHttpProviderFactory;

class CurlHttpProvider : public HttpProvider
{
   public:
      CurlHttpProvider();
      virtual ~CurlHttpProvider();
      virtual void get(const GenericUri& target, const Data& tid, TransactionUser& tu);
   private:
      class RequestThread : public ThreadIf
      {
         public:
            RequestThread(const GenericUri& target, const Data& tid, TransactionUser& tu);
            virtual void thread();
            virtual ~RequestThread();
         private:
            GenericUri mTarget;
            Data mTid;
            TransactionUser& mTransactionUser;
            Data mX509Blob;
            DataStream mStream;
            static int curlCallback(void *ptr, size_t size, size_t nmemb, void *stream);
      };
};

class CurlHttpProviderFactory : public HttpProviderFactory
{
   public:
      virtual CurlHttpProvider* createHttpProvider();      
      virtual ~CurlHttpProviderFactory(){}
};
}

#endif
