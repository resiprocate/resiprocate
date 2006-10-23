#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
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
  X509 *serverCert;
  EVP_PKEY *serverKey;

  srtp_init();
  
  assert(argc==2);
  
  createCert(resip::Data("sip:server@example.com"),365,1024,serverCert,serverKey);
  
  DtlsFactory *serverFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()),serverCert,serverKey);

  cout << "Created the factory\n";

  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if ( fd == -1 )
  {
    assert(0);
  }

  // Make the UDP socket context
  int port=atoi(argv[1]);
  struct sockaddr_in myaddr;
  memset((char*) &(myaddr),0, sizeof((myaddr)));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);
  int r=bind( fd,(struct sockaddr*)&myaddr, sizeof(myaddr));
  assert(r==0);

  cout << "Made our UDP socket\n";
  

  cout << "Entering wait loop\n";
  TestDtlsUdpSocketContext *sockContext=0;
  DtlsSocket *dtlsSocket;

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

        // The first packet initiates the association
        if(!sockContext){
          sockContext=new TestDtlsUdpSocketContext(fd,&src);

          cout << "Made the socket context\n";          

          dtlsSocket=serverFactory->createServer(std::auto_ptr<DtlsSocketContext>(sockContext));
          
          cout << "Made the DTLS socket\n";
        }

        switch(DtlsFactory::demuxPacket(buffer,r)){
          case DtlsFactory::dtls:
            dtlsSocket->handlePacketMaybe(buffer,r);
            break;
          case DtlsFactory::rtp:
            unsigned char buf2[4096];
            unsigned int buf2l;

            sockContext->recvRtpData(buffer,r,buf2,&buf2l,sizeof(buf2));

            cout << "Read RTP data of length " << buf2l << endl;
            cout << buf2 << endl;

            for(int i=0;i<buf2l;i++){
              buf2[i]=toupper(buf2[i]);
            }

            // Now echo it back
            sockContext->sendRtpData((const unsigned char *)buf2,buf2l);

            break;
        }
      }
    }
  }

  
  exit(0);
}
     
