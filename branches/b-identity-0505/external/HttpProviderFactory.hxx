#ifndef RESIP_HttpProviderFactory_HXX
#define RESIP_HttpProviderFactory_HXX

namespace resip
{
class HttpProvider;

class HttpProviderFactory
{
   public:
      static HttpProvider* createHttpProvider();
};

}

#endif

