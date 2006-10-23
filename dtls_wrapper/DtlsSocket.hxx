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
class DtlsSocket;

class DtlsSocketContext
{
   public:
      //memory is only valid for duration of callback; must be copied if queueing
      //is required 
      virtual ~DtlsSocketContext(){}      
      virtual void write(const unsigned char* data, unsigned int len)=0;
      virtual void handshakeCompleted()=0;
      virtual void handshakeFailed(const char *err)=0;

   protected:
      DtlsSocket *mSocket;
     
   private:
      friend class DtlsSocket;
     
      void setDtlsSocket(DtlsSocket *sock) {mSocket=sock;}
};

class SrtpSessionKeys
{
   public:
      unsigned char *clientMasterKey;
      int clientMasterKeyLen;
      unsigned char *serverMasterKey;
      int serverMasterKeyLen;
      unsigned char *clientMasterSalt;
      int clientMasterSaltLen;
      unsigned char *serverMasterSalt;
      int serverMasterSaltLen;
};

class DtlsSocket
{
   public:
      bool handlePacketMaybe(const unsigned char* bytes, unsigned int len);
      void forceRetransmit();
      bool checkFingerprint(const char* fingerprint, unsigned int len);
      bool DtlsSocket::getRemoteFingerprint(char *fingerprint);
      void DtlsSocket::getMyCertFingerprint(char *fingerprint);
      void startClient();

      SrtpSessionKeys getSrtpSessionKeys();
      static void DtlsSocket::computeFingerprint(X509 *cert, char *fingerprint);
     
      //may return 0 if profile selection failed
      SRTP_PROTECTION_PROFILE* getSrtpProfile();      
      
   private:
      friend class DtlsFactory;
      enum SocketType { Client, Server};
      DtlsSocket(std::auto_ptr<DtlsSocketContext>, DtlsFactory* factory, enum SocketType);
      void doHandshakeIteration();
      
      // Internals
      std::auto_ptr<DtlsSocketContext> mSocketContext;
      DtlsFactory* mFactory;
      
      // OpenSSL context data
      SSL *ssl;
      BIO *mInBio;
      BIO *mOutBio;
      
      SocketType mSocketType;
      bool mHandshakeCompleted;      
};


}
#endif
