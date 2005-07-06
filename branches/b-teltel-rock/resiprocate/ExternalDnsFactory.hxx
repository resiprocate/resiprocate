#ifndef RESIP_ExternalDnsFactory_HXX
#define RESIP_ExternalDnsFactory_HXX


namespace resip
{
class ExternalDns;

//linker pattern, replace the cxx to use a different Dns provider. The deafult
//is to use ares.
class ExternalDnsFactory
{
   public:
      static ExternalDns* createExternalDns();
};

}

#endif
