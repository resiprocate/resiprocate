#include <iostream>

#include <openssl/ssl.h>
#include <openssl/srtp.h>

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "TestTimerContext.hxx"
#include "TestDtlsUdp.hxx"
#include "CreateCert.hxx"


using namespace std;
using namespace dtls;
using namespace resip;


int main(int argc,char **argv)
{
   X509 *clientCert;
   EVP_PKEY *clientKey;

   resip::initNetwork();
   srtp_init();

   assert(argc==3);

   createCert(resip::Data("sip:client@example.com"),365,1024,clientCert,clientKey);

   TestTimerContext *ourTimer=new TestTimerContext();
   DtlsFactory *clientFactory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(ourTimer),clientCert,clientKey);

   cout << "Created the factory\n";

   Socket fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

   while(1)
   {
      FdSet fdset;
      unsigned char buffer[4096];
      struct sockaddr_in src;
      socklen_t srclen;
      int r;

      fdset.setRead(fd);
#if !defined(WIN32)
      fdset.setRead(0);
#endif

      UInt64 towait=ourTimer->getRemainingTime();

      // cerr << "Invoking select for time " << towait << endl;

      int toread=fdset.selectMilliSeconds(towait);

      ourTimer->updateTimer();

      if(toread >=0)
      {
         if (fdset.readyToRead(0))
         {
            char inbuf[1024];
            cin.getline(inbuf, 1024);
            cout << "Read from stdin " << inbuf << endl;
            sockContext->sendRtpData((const unsigned char *)inbuf,strlen(inbuf));
         }
         if (fdset.readyToRead(fd))
         {
            srclen=sizeof(src);
            r=recvfrom(fd, reinterpret_cast<char*>(buffer),
               sizeof(buffer), 0, (sockaddr *)&src,&srclen);
            assert(r>=0);

            cerr << "Read bytes from peer, len = " << r << endl; 

            switch(DtlsFactory::demuxPacket(buffer,r))
            {
            case DtlsFactory::dtls:
               dtlsSocket->handlePacketMaybe(buffer,r);
               break;
            case DtlsFactory::rtp:
               unsigned char buf2[4096];
               unsigned int buf2l;

               // Read data 
               sockContext->recvRtpData(buffer,r,buf2,&buf2l,sizeof(buf2));

               cout << "Read RTP data of length " << buf2l << endl;
               cout.write(reinterpret_cast<char *>(buf2),buf2l);
               cout << endl;
               break;
            default:
               break;
            }
         }
      }
   }

   exit(0);
}
     

/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
