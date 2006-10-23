#include <iostream>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "rutil/Data.hxx"
#include "CreateCert.hxx"
#include "TestTimerContext.hxx"
#include <openssl/srtp.h>

using namespace std;
using namespace dtls;
using namespace resip;

class TestDtlsSocketContext : public DtlsSocketContext
{
   public:
     DtlsSocket *mOtherSocket;
     char *mName;
     
     //memory is only valid for duration of callback; must be copied if queueing
     //is required
     TestDtlsSocketContext(char *name):
          mName(name){}
           
      virtual ~TestDtlsSocketContext(){}
     
      virtual void write(const unsigned char* data, unsigned int len)
      {
        cout << mName << ": DTLS Wrapper called write...len = " << len << endl;

        // Discard data and force retransmit
//        mSocket->forceRetransmit();
        mOtherSocket->handlePacketMaybe(data, len);
      }
      
      virtual void handshakeCompleted()
      {
        char fprint[100];
        SRTP_PROTECTION_PROFILE *srtp_profile;
        
        cout << mName<< ": Hey, amazing, it worked\n";

        if(mSocket->getRemoteFingerprint(fprint)){
          cout << mName << ": Remote fingerprint == " << fprint << endl;
          
          mOtherSocket->getMyCertFingerprint(fprint);
          
          bool check=mSocket->checkFingerprint(fprint,strlen(fprint));

          cout << mName << ": Fingerprint check == " << check << endl;
        }
        else {
          cout << mName << ": Peer did not authenticate" << endl;
        }

        srtp_profile=mSocket->getSrtpProfile();

        if(srtp_profile){
          cout << mName << ": SRTP Extension negotiated profile="<<srtp_profile->name << endl;
        }
      }
     
      virtual void handshakeFailed(const char *err)
      {
        cout << mName << ": Bummer, handshake failure "<<err<<endl;
      }
};

int main(int argc,char **argv)
{
  SSL_library_init();
  SSL_load_error_strings();
  ERR_load_crypto_strings();

  X509 *clientCert,*serverCert;
  EVP_PKEY *clientKey,*serverKey;

  createCert(resip::Data("sip:client@example.com"),365,1024,clientCert,clientKey);
  createCert(resip::Data("sip:server@example.com"),365,1024,serverCert,serverKey);
  
  DtlsFactory *clientFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()),clientCert,clientKey);
  DtlsFactory *serverFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()),serverCert,serverKey);
  
  cout << "Created the factories\n";

  TestDtlsSocketContext *clientContext=new TestDtlsSocketContext("Client");
  TestDtlsSocketContext *serverContext=new TestDtlsSocketContext("Server");
  
  DtlsSocket *clientSocket=clientFactory->createClient(std::auto_ptr<DtlsSocketContext>(clientContext));
  DtlsSocket *serverSocket=serverFactory->createServer(std::auto_ptr<DtlsSocketContext>(serverContext));

  clientContext->mOtherSocket=serverSocket;
  serverContext->mOtherSocket=clientSocket;  
  
  clientSocket->startClient();
}
     
