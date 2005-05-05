#include "HttpProvider.hxx"
#include "HttpProviderFactory.hxx"

using namespace resip;

HttpProvider* HttpProvider::mInstance = 0;
Mutex HttpProvider::mMutex;      

HttpProvider* 
HttpProvider::instance()
{
   if (mInstance == 0)
   {
      Lock(mMutex);
      if (mInstance == 0)
      {
         mInstance = HttpProviderFactory::createExternalDns();
      }
   }
   return mInstance;   
}


      
