#include "HttpProvider.hxx"
#include "resiprocate/os/Lock.hxx"

using namespace resip;

HttpProvider* HttpProvider::mInstance = 0;
HttpProviderFactory* HttpProvider::mFactory = 0;
Mutex HttpProvider::mMutex;      

void 
HttpProvider::setFactory(std::auto_ptr<HttpProviderFactory> fact)
{
   mFactory = fact.release();
}

HttpProvider* 
HttpProvider::instance()
{
   if (mFactory && mInstance == 0)
   {
      Lock lock(mMutex);
      if (mInstance == 0)
      {
         mInstance = mFactory->createHttpProvider();
      }
   }
   return mInstance;   
}


      
