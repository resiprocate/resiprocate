#ifndef DtlsSocket_hxx
#define DtlsSocket_hxx

#include <memory>

#include <openssl/e_os2.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

namespace dtls
{

class DtlsFactory;

class DtlsSocketContext
{
   public:
     //memory is only valid for duration of callback; must be copied if queueing
     //is required 
      virtual ~DtlsSocketContext(){}      
     virtual void write(const char* data, unsigned int len)=0;
     virtual void handshakeCompleted()=0;
     virtual void handshakeFailed()=0;
};

class DtlsSocket
{
   public:
     bool consumedPacket(const char* bytes, unsigned int len);
     bool checkFingerprint(const char* fingerprint, unsigned int len);      
      //guts of one connection go here

     void startClient();      

   private:
     friend class DtlsFactory;
     enum SocketType { Client, Server};
     
     DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory, enum SocketType);
     DtlsFactory* mFactory;

     // OpenSSL context data
     SSL *ssl;
     BIO *mInBio;
     BIO *mOutBio;
      
     SocketType mSocketType;
      
};


}
#endif
