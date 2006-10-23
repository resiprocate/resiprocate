#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "TestTimerContext.hxx"
#include "TestDtlsUdp.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "CreateCert.hxx"
#include <openssl/srtp.h>

using namespace std;
using namespace dtls;
using namespace resip;


int main(int argc,char **argv)
{
  X509 *clientCert;
  EVP_PKEY *clientKey;
  
  assert(argc==3);
  
  createCert(resip::Data("sip:client@example.com"),365,1024,clientCert,clientKey);
  
  DtlsFactory *clientFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()),clientCert,clientKey);

  cout << "Created the factory\n";

  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if ( fd == -1 )
  {
    assert(0);
  }

  // Make the UDP socket context
  char *host=argv[1];
  int port=atoi(argv[2]);
  struct hostent *hp=gethostbyname(host);
  struct sockaddr_in peer_addr;
  assert(hp!=0);
  memset(&peer_addr,0,sizeof(struct in_addr));
  peer_addr.sin_addr=*(struct in_addr*)
    hp->h_addr_list[0];
  peer_addr.sin_family=AF_INET;
  peer_addr.sin_port=htons(port);

  cout << "Made our UDP socket\n";
  
  TestDtlsUdpSocketContext *sockContext=new TestDtlsUdpSocketContext(fd,&peer_addr);

  cout << "Made the socket context\n";

  DtlsSocket *dtlsSocket=clientFactory->createClient(std::auto_ptr<DtlsSocketContext>(sockContext));

  cout << "Made the DTLS socket\n";

  // Now kick things off
  dtlsSocket->startClient();

  cout << "Sent first packet, entering wait loop\n";

  while(1){
    FdSet fdset;
    unsigned char buffer[4096];
    struct sockaddr_in src;
    socklen_t srclen;
    int r;
    
    fdset.setRead(fd);

    if (fdset.selectMilliSeconds(1000000) >= 0)
    {
      if (fdset.readyToRead(fd))
      {
        srclen=sizeof(src);
        r=recvfrom(fd, buffer, sizeof(buffer), 0, (sockaddr *)&src,&srclen);
        assert(r>=0);

        dtlsSocket->handlePacketMaybe(buffer,r);
      }
    }
  }

  
  exit(0);
}
     
