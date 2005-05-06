#if !defined(RESIP_EXTERNAL_TIMER_HXX)
#define RESIP_EXTERNAL_TIMER_HXX

#include "resiprocate/os/Mutex.hxx"
#include <memory>

namespace resip
{
class TransactionUser;
class Data;
class GenericUri;

//To provide this functionality, plug in an instance of
//HttpProviderFactory(before any instances of DialogUsageManager/SipStack are
//created. Null will be returned by default.
class HttpProviderFactory;

class HttpProvider
{
   public:
      //HttpProvider assumes memory
      static void setFactory(std::auto_ptr<HttpProviderFactory> fact);
      //ptr so users can check for existence
      static HttpProvider* instance();
      
      //.dcm. tu param will become a postable
      virtual void get(const GenericUri& target, const Data& tid, TransactionUser& tu)=0;
      virtual ~HttpProvider(){} //impl. singleton destructor pattern later
   private:
      static HttpProvider* mInstance;
      static HttpProviderFactory* mFactory;
      static Mutex mMutex;      
};

class HttpProviderFactory
{
   public:
      virtual HttpProvider* createHttpProvider()=0;
      virtual ~HttpProviderFactory(){}
};
}

#endif
