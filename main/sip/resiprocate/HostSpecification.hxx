#ifndef HostSpecification_hxx
#define HostSpecification_hxx

#include <string>

namespace Vocal2
{

class HostSpecification
{
   public:
      HostSpecification(string hostPort);
      HostSpecification(string host, int port);
      
      const string& getHost() const;
      const string& getAddressString() const;
      int getPort() const;
   private:
      string mHost;
      int mPort;
};


#endif
