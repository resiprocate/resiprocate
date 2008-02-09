#ifndef DtlsFactory_hxx
#define DtlsFactory_hxx

#include <memory>

#include "DtlsTimer.hxx"

typedef struct x509_st X509;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct evp_pkey_st EVP_PKEY;

namespace dtls
{

class DtlsSocket;
class DtlsSocketContext;
class DtlsTimerContext;

//Not threadsafe. Timers must fire in the same thread as dtls processing.
class DtlsFactory
{
   public:
     enum PacketType { rtp, dtls, stun, unknown};
     
     DtlsFactory(std::auto_ptr<DtlsTimerContext> tc, X509 *cert, EVP_PKEY *privkey);

     // Note: this orphans any DtlsSockets you were stupid enough
     // not to free
     ~DtlsFactory();
     
     DtlsSocket* createClient(std::auto_ptr<DtlsSocketContext> context);
     DtlsSocket* createServer(std::auto_ptr<DtlsSocketContext> context);
     void getMyCertFingerprint(char *fingerprint);
     DtlsTimerContext& getTimerContext() {return *mTimerContext;}
     void setSrtpProfiles(const char *policyStr);
     void setCipherSuites(const char *cipherSuites);
     static const char* DefaultSrtpProfile; 

     static PacketType demuxPacket(const unsigned char *buf, unsigned int len);
     
      //context accessor
private:
     friend class DtlsSocket;
     SSL_CTX* mContext;
     std::auto_ptr<DtlsTimerContext> mTimerContext;
     X509 *mCert;
};

}
#endif


/* ====================================================================

 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
