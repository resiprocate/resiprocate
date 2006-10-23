#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "CreateCert.hxx"
#include "TestDtlsUdp.hxx"
#include <openssl/srtp.h>

extern "C" 
{
#include <srtp/include/srtp.h>
#include <srtp/include/srtp_priv.h>
}


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

   cerr << "Wrote " << len << " bytes" << endl;
   
}

void
TestDtlsUdpSocketContext::handshakeCompleted(){
  char fprint[100];
  SRTP_PROTECTION_PROFILE *srtp_profile;
  int r;
  
  cout << "Hey, amazing, it worked\n";

  if(mSocket->getRemoteFingerprint(fprint)){
    cout << "Remote fingerprint == " << fprint << endl;
  } 
  srtp_profile=mSocket->getSrtpProfile();
  
  if(srtp_profile){
    cout <<"SRTP Extension negotiated profile="<<srtp_profile->name << endl;
  }

  mSocket->createSrtpSessionPolicies(srtpPolicyIn,srtpPolicyOut);

  r=srtp_create(&srtpIn,&srtpPolicyIn);
  assert(r==0);
  r=srtp_create(&srtpOut,&srtpPolicyOut);
  assert(r==0);

  useSrtp=true;
  
  cout << "Made SRTP policies\n";
}

void
TestDtlsUdpSocketContext::handshakeFailed(const char *err){
  cout <<  "Bummer, handshake failure "<<err<<endl;
}


void
TestDtlsUdpSocketContext::sendRtpData(const unsigned char *data, unsigned int len){
  srtp_hdr_t *hdr;
  unsigned char *ptr;
  int l=0;

  cerr << "Sending RTP packet of length " << len << endl;
  
  ptr=(unsigned char *)malloc(sizeof(srtp_hdr_t)+len+SRTP_MAX_TRAILER_LEN+4);
  assert(ptr!=0);
  hdr=(srtp_hdr_t *)ptr;
  ptr+=sizeof(srtp_hdr_t);
  l+=sizeof(srtp_hdr_t);
  
  hdr->version = 2;              /* RTP version two     */
  hdr->p    = 0;                 /* no padding needed   */
  hdr->x    = 0;                 /* no header extension */
  hdr->cc   = 0;                 /* no CSRCs            */
  hdr->m    = 0;                 /* marker bit          */
  hdr->pt   = 0xf;               /* payload type        */
  hdr->seq  = mRtpSeq++;         /* sequence number     */
  hdr->ts   = htonl(0xdecafbad); /* timestamp           */
  hdr->ssrc = htonl(ssrc);       /* synch. source       */

  memcpy(ptr,data,len);
  l+=len;

  if(useSrtp){
    int r=srtp_protect(srtpOut,(unsigned char *)hdr,&l);
    assert(r==0);
  }
  write((unsigned char *)hdr,l);
}
     
void
TestDtlsUdpSocketContext::recvRtpData(unsigned char *in, unsigned int inlen, unsigned char *out, unsigned int *outlen,unsigned int maxoutlen){
  srtp_hdr_t *hdr;
  int len_int=(int)inlen;
  hdr=(srtp_hdr_t *)in;
  
  if(useSrtp){
    int r=srtp_unprotect(srtpIn,hdr,&len_int);
    assert(r==0);
    inlen=(unsigned int)len_int;
  }
  
  in+=sizeof(srtp_hdr_t);
  inlen-=sizeof(srtp_hdr_t);
  
  assert(inlen<maxoutlen);
  memcpy(out,in,inlen);
  *outlen=inlen;
}



