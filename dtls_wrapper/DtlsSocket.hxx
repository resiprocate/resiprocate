#ifndef DtlsSocket_hxx
#define DtlsSocket_hxx

#include <memory>

namespace dtls
{

class DtlsFactory;

class DtlsSocketContext
{
   public:
     //memory is only valid for duration of callback; must be copied if queueing
     //is required 
     virtual void write(const char* data, unsigned int len)=0;
     virtual void handshakeCompleted()=0;
};

class DtlsSocket
{
   public:
     bool consumedPacket(const char* bytes, unsigned int len);
     bool checkFingerprint(const char* fingerprint, unsigned int len);      
      //guts of one connection go here

   private:
     friend class DtlsFactory;
     enum SocketType { Client, Server};
     
     DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory, enum SocketType);
     DtlsFactory* mFactory;

     // OpenSSL context data
     SSL *ssl;
     BIO *mInBio;
     BIO *mOutBio;
};


}
#endif
