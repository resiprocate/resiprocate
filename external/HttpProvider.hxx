#if !defined(RESIP_EXTERNAL_TIMER_HXX)
#define RESIP_EXTERNAL_TIMER_HXX

#include "resiprocate/os/Mutex.hxx"

namespace resip
{
class TransactionUser;
class Data;
class GenericUri;

//this uses the 'link' compile time selection idiom.  Implement/link in
//HttpProviderFactory.cxx to provide the implemntation of HttpProvider
class HttpProvider
{
   public:
      //ptr so users can check for existence
      static HttpProvider* instance();
      
      //.dcm. tu param will become a postable
      virtual void get(const GenericUri& target, const Data& tid, TransactionUser& tu)=0;
      virtual ~HttpProvider(){} //impl. singleton destructor pattern later
   private:
      static HttpProvider* mInstance;
      static Mutex mMutex;      
};

}

#endif
