#if !defined(TcpConnection_hxx)
#define TcpConnection_hxx

#include "resiprocate/Connection.hxx"

namespace resip
{

class Tuple;

class TcpConnection : public Connection
{
   public:
      TcpConnection( const Tuple& who, Socket fd );
      
      int read( char* buf, int count );
      int write( const char* buf, int count );
      bool hasDataToRead(); // has data that can be read 
      bool isGood(); // has valid connection
      Data peerName();      
};
 
}

#endif
