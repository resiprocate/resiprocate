#ifndef HostSpecification_hxx
#define HostSpecification_hxx

#include <sipstack/Data.hxx>

namespace Vocal2
{

class HostSpecification
{
   public:
      HostSpecification(Data hostPort);
      HostSpecification(Data host, int port);
      
      const Data& getHost() const;
      const Data& getAddressString() const;
      int getPort() const;
   private:
      Data mHost;
      int mPort;
};


#endif
