#if !defined(TlsConnection_hxx)
#define TlsConnection_hxx

#include "resiprocate/Connection.hxx"
#ifdef USE_SSL
#include <openssl/ssl.h>
#endif

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
#if USE_SSL
      SSL* ssl;
      BIO* bio;
#endif
};
 
}


#endif
