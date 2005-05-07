#include "CurlHttpProvider.hxx"
#include "resiprocate/external/HttpGetMessage.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/os/Logger.hxx"

#include <curl/curl.h>

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace resip;

CurlHttpProvider::CurlHttpProvider()
{
   curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlHttpProvider::~CurlHttpProvider()
{
}

void 
CurlHttpProvider::get(const GenericUri& target, const Data& tid, TransactionUser& tu)
{
   //fix cleanup logic, self deletes for now
   RequestThread* rt = new RequestThread(target, tid, tu);
   rt->run();   
}

CurlHttpProvider* 
CurlHttpProviderFactory::createHttpProvider()
{
   return new CurlHttpProvider();
}

CurlHttpProvider::RequestThread::RequestThread(const GenericUri& target, 
                             const Data& tid, 
                             TransactionUser& tu) :
   mTarget(target),
   mTid(tid),
   mTransactionUser(tu),
   mX509Blob(),
   mStream(mX509Blob)
{}

CurlHttpProvider::RequestThread::~RequestThread()
{
}

void 
CurlHttpProvider::RequestThread::thread()
{
   CURL *curl = curl_easy_init();
   if (!curl)
   {
       HttpGetMessage* res = new HttpGetMessage(mTid, false, Data::Empty, Mime());
       mTransactionUser.post(res);
   }

   curl_easy_setopt(curl, CURLOPT_URL, mTarget.uri().c_str());
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RequestThread::curlCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mStream);
   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
   
   int code = 404;
   curl_easy_perform(curl);
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
   InfoLog ( << "Curl returned: " << code);   

    if (code / 100 == 2) 
    {
        //.dcm.  vodoo to trick the lazy parsers, should add convenience methods to
        //clean this up
        Mime contentType; 
        {
           char* contentTypeString;
           curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentTypeString);

           InfoLog ( << "Content type from curl: " << contentTypeString);
           
           if (contentTypeString)
           {
              HeaderFieldValue tmp(contentTypeString, strlen(contentTypeString));
              Mime tempMime(&tmp, Headers::ContentType);
              contentType = tempMime;
              delete contentTypeString;          
           }
        }
        
        HttpGetMessage* res = new HttpGetMessage(mTid, true, mX509Blob, contentType);
        mTransactionUser.post(res);
    }
    else
    {
       HttpGetMessage* res = new HttpGetMessage(mTid, false, Data::Empty, Mime());
       mTransactionUser.post(res);
    }
    curl_easy_cleanup(curl);

    delete this;
}

int 
CurlHttpProvider::RequestThread::curlCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
   DataStream* ds = reinterpret_cast<DataStream*>(stream);
   assert(size == 1);
   ds->write(reinterpret_cast<const char*>(ptr), nmemb);
   return nmemb;
}

