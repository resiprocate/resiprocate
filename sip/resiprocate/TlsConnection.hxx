#if !defined(TlsConnection_hxx)
#define TlsConnection_hxx

#include "resiprocate/Connection.hxx"
#include <openssl/ssl.h>

namespace resip
{

class Tuple;
class Security;

class TlsConnection : public Connection
{
   public:
      TlsConnection( const Tuple& who, Socket fd, Security* security, bool server=false );
      
      int read( char* buf, const int count );
      int write( const char* buf, const int count );
      bool hasDataToRead(); // has data that can be read 
      bool isGood(); // has valid connection
      
      Data peerName();
      
   private:
      SSL* ssl;
      BIO* bio;
};
 
}


#endif
