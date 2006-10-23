#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "CreateCert.hxx"
#include "TestDtlsUdp.hxx"
#include <openssl/srtp.h>

using namespace std;
using namespace dtls;
using namespace resip;

TestDtlsUdpSocketContext::TestDtlsUdpSocketContext(int fd,sockaddr_in *peerAddr){
  mFd=fd;
  memcpy(&mPeerAddr,peerAddr,sizeof(sockaddr_in));
}

void
TestDtlsUdpSocketContext::write(const unsigned char* data, unsigned int len){
  int s = sendto(mFd, data, len, 0, (const sockaddr *)&mPeerAddr, sizeof(struct sockaddr_in));
   if ( s == SOCKET_ERROR )
   {
      int e = errno;
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            clog << "err EAFNOSUPPORT in send" << endl;
         }
         break;
         default:
         {
            clog << "err " << e << " "  << strerror(e) << " in send" << endl;
         }
      }
      assert(0);
   }
    
   if ( s == 0 )
   {
      clog << "no data sent in send" << endl;
      assert(0);
   }
}

void
TestDtlsUdpSocketContext::handshakeCompleted(){
  char fprint[100];
  SRTP_PROTECTION_PROFILE *srtp_profile;
        
  cout << "Hey, amazing, it worked\n";

  if(mSocket->getRemoteFingerprint(fprint)){
    cout << "Remote fingerprint == " << fprint << endl;
  } 
  srtp_profile=mSocket->getSrtpProfile();
  
  if(srtp_profile){
    cout <<"SRTP Extension negotiated profile="<<srtp_profile->name << endl;
  }
}

void
TestDtlsUdpSocketContext::handshakeFailed(const char *err){
  cout <<  "Bummer, handshake failure "<<err<<endl;
}





