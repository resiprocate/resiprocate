#include <iomanip>
#include <iostream>
#include <sstream>

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <memory.h>

#include <openssl/ssl.h>
#include <openssl/srtp.h>

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "CreateCert.hxx"
#include "TestDtlsUdp.hxx"

// The fixed RTP header (RFC 3550 section 5.1).  Defined here because libsrtp's
// srtp_hdr_t lives in srtp_priv.h, which libsrtp2 doesn't install.
struct RtpHeader
{
   unsigned char vpxcc;  // version (2 bits), padding, extension, CSRC count
   unsigned char mpt;    // marker bit, payload type (7 bits)
   uint16_t seq;         // network byte order
   uint32_t ts;          // network byte order
   uint32_t ssrc;        // network byte order
};
static_assert(sizeof(RtpHeader) == 12, "RTP header must be 12 bytes");

void DumpHexa2(const unsigned char* pInMsg, unsigned long ulInMsgLen, std::string &rOutDump)
{
   if(ulInMsgLen == 0 || NULL == pInMsg)
   {
      return;
   }

   std::ostringstream ss;
   ss << "\n\n***new data***";
   ss << " length: " << ulInMsgLen;
   ss << " dump:";

   for (unsigned int z=0; z < ulInMsgLen; z++)
   {
      ss << ' ';
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(pInMsg[z]);
   }

   rOutDump = ss.str();
}

using namespace std;
using namespace dtls;
using namespace resip;

TestDtlsUdpSocketContext::TestDtlsUdpSocketContext(int fd,sockaddr_in *peerAddr)
{
   mFd=fd;
   memcpy(&mPeerAddr,peerAddr,sizeof(sockaddr_in));
}

void
TestDtlsUdpSocketContext::write(const unsigned char* data, unsigned int len)
{
   std::string cdata;
   DumpHexa2(data, len, cdata);

#if 0
   //FILE *f = fopen("c:\\testDtlsClient.txt", "a");
   //if(f)
   //{
   //   fprintf(f, cdata.c_str());
   //   fclose(f);
   //}
   FILE *f = fopen("c:\\testDtlsClient_binary.txt", "ab");
   if(f)
   {
      fwrite(data, 1, len, f);
      fclose(f);
   }
#else
   cout << cdata <<endl;
#endif

   int s = sendto(mFd, (const char*)data, len, 0, (const sockaddr *)&mPeerAddr, sizeof(struct sockaddr_in));

   if ( s == SOCKET_ERROR )
   {
      clog << "err " << getErrno() << " in send" << endl;
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
TestDtlsUdpSocketContext::handshakeCompleted()
{
   char fprint[100];
   SRTP_PROTECTION_PROFILE *srtp_profile;
   int r;

   cout << "Hey, amazing, it worked\n";

   if(mSocket->getRemoteFingerprint("sha-256",fprint,sizeof(fprint)))
   {
      cout << "Remote fingerprint == " << fprint << endl;
   } 
   srtp_profile=mSocket->getSrtpProfile();

   if(srtp_profile)
   {
      cout <<"SRTP Extension negotiated profile="<<srtp_profile->name << endl;
   }

   bool created=mSocket->createSrtpSessionPolicies(srtpPolicyOut,srtpPolicyIn);  // (outbound, inbound)
   assert(created);

   r=srtp_create(&srtpIn,&srtpPolicyIn);
   assert(r==0);
   r=srtp_create(&srtpOut,&srtpPolicyOut);
   assert(r==0);
   delete[] srtpPolicyIn.key;
   delete[] srtpPolicyOut.key;

   useSrtp=true;

   cout << "Made SRTP policies\n";
   if(mSocket->getSocketType()==DtlsSocket::Client) 
   {
      const char *testData="test data";
      sendRtpData((const unsigned char *)testData,strlen(testData)+1);


#if defined(WIN32)
      const char *testData2="test bobo";
      sendRtpData((const unsigned char *)testData2,strlen(testData)+1);
#endif
   }
}

void
TestDtlsUdpSocketContext::handshakeFailed(const char *err)
{
   cout <<  "Bummer, handshake failure "<<err<<endl;
}


void
TestDtlsUdpSocketContext::sendRtpData(const unsigned char *data, unsigned int len)
{
   RtpHeader *hdr;
   unsigned char *buffer, *ptr;
   int l=0;

   cerr << "Sending RTP packet of length " << len << endl;

   buffer=(unsigned char *)malloc(sizeof(RtpHeader)+len+SRTP_MAX_TRAILER_LEN+4);
   assert(buffer!=0);
   ptr=buffer;
   hdr=(RtpHeader *)ptr;
   ptr+=sizeof(RtpHeader);
   l+=sizeof(RtpHeader);

   hdr->vpxcc = 0x80;             /* version 2, no padding, extension or CSRCs */
   hdr->mpt   = 0x0f;             /* no marker, payload type 15 */
   hdr->seq   = htons(mRtpSeq++); /* sequence number     */
   hdr->ts    = htonl(0xdecafbad);/* timestamp           */
   hdr->ssrc  = htonl(ssrc);      /* synch. source       */

   memcpy(ptr,data,len);
   l+=len;

   if(useSrtp)
   {
      srtp_err_status_t r=srtp_protect(srtpOut,(unsigned char *)hdr,&l);
      assert(r==srtp_err_status_ok);
   }
   write((unsigned char *)hdr,l);

   free(buffer);
}

void
TestDtlsUdpSocketContext::recvRtpData(unsigned char *in, unsigned int inlen, unsigned char *out, unsigned int *outlen,unsigned int maxoutlen)
{
   int len_int=(int)inlen;

   if(useSrtp)
   {
      srtp_err_status_t r=srtp_unprotect(srtpIn,in,&len_int);
      assert(r==srtp_err_status_ok);
      inlen=(unsigned int)len_int;
   }

   assert(inlen>=sizeof(RtpHeader));
   in+=sizeof(RtpHeader);
   inlen-=sizeof(RtpHeader);

   assert(inlen<maxoutlen);
   memcpy(out,in,inlen);
   *outlen=inlen;
}


/* ====================================================================

 Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
