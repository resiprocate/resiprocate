#ifndef HostSpecification_hxx
#define HostSpecification_hxx

#include <util/Data.hxx>

namespace Vocal2
{

class HostSpecification
{
   public:
      HostSpecification(Data hostPort);
      HostSpecification(Data host, int port);
      
      const Data& getHost() const;
      const Data& getAddressString() const;

      static Data getLocalHostName();
      
      int getPort() const;
   private:
      Data mHost;
      int mPort;
};

 
}

#endif
