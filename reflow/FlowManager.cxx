#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/function.hpp>
#include <map>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ThreadIf.hxx>
#include <rutil/Random.hxx>
#include <rutil/SharedPtr.hxx>
#include <rutil/Timer.hxx>

#ifdef WIN32
#include <srtp.h>
#else
#include <srtp/srtp.h>
#endif

#ifdef USE_SSL  
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include "FlowDtlsTimerContext.hxx"
#endif //USE_SSL

#include "FlowManagerSubsystem.hxx"
#include "FlowManager.hxx"

using namespace flowmanager;
using namespace resip;
#ifdef USE_SSL 
using namespace dtls;
#endif 
using namespace std;

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

namespace flowmanager
{
class IOServiceThread : public ThreadIf
{
public:
   IOServiceThread(asio::io_service& ioService) : mIOService(ioService) {}

   virtual ~IOServiceThread() {}

   virtual void thread()
   {
      mIOService.run();
   }
private:
   asio::io_service& mIOService;
};
}

FlowManager::FlowManager()
#ifdef USE_SSL
   : 
   mSslContext(mIOService, asio::ssl::context::tlsv1),
   mClientCert(0),
   mClientKey(0),
   mDtlsFactory(0)
#endif  
{
   mIOServiceWork = new asio::io_service::work(mIOService);
   mIOServiceThread = new IOServiceThread(mIOService);
   mIOServiceThread->run();

#ifdef USE_SSL
   // Setup SSL context
   asio::error_code ec; 
   mSslContext.set_verify_mode(asio::ssl::context::verify_peer | 
                               asio::ssl::context::verify_fail_if_no_peer_cert);
#define VERIFY_FILE "ca.pem"
   mSslContext.load_verify_file(VERIFY_FILE, ec);   // TODO make a setting
   if(ec)
   {
      ErrLog(<< "Unable to load verify file: " << VERIFY_FILE << ", error=" << ec.value() << "(" << ec.message() << ")");
   }
#endif 

   // Initialize SRTP 
   err_status_t status = srtp_init();
   if(status && status != err_status_bad_param)  // Note: err_status_bad_param happens if srtp_init is called twice - we allow this for test programs
   {
      ErrLog(<< "Unable to initialize SRTP engine, error code=" << status);
      throw FlowManagerException("Unable to initialize SRTP engine", __FILE__, __LINE__);
   }
   status = srtp_install_event_handler(FlowManager::srtpEventHandler);   
}
  

FlowManager::~FlowManager()
{
   delete mIOServiceWork;
   mIOServiceThread->join();
   delete mIOServiceThread;
 
 #ifdef USE_SSL
   if(mDtlsFactory) delete mDtlsFactory;
   if(mClientCert) X509_free(mClientCert);
   if(mClientKey) EVP_PKEY_free(mClientKey);
 #endif 
}

#ifdef USE_SSL
void 
FlowManager::initializeDtlsFactory(const char* certAor)
{
   if(mDtlsFactory)
   {
      ErrLog(<< "initializeDtlsFactory called when DtlsFactory is already initialized.");    
      return;
   }

   Data aor(certAor);  
   if(createCert(aor, 365 /* expireDays */, 1024 /* keyLen */, mClientCert, mClientKey))
   {
      FlowDtlsTimerContext* timerContext = new FlowDtlsTimerContext(mIOService);
      mDtlsFactory = new DtlsFactory(std::auto_ptr<DtlsTimerContext>(timerContext), mClientCert, mClientKey);
      resip_assert(mDtlsFactory);
   }
   else
   {
      ErrLog(<< "Unable to create a client cert, cannot use Dtls-Srtp.");    
   }   
}
#endif 

void
FlowManager::srtpEventHandler(srtp_event_data_t *data) 
{
   switch(data->event) {
   case event_ssrc_collision:
     WarningLog(<< "SRTP SSRC collision");
     break;
   case event_key_soft_limit:
     WarningLog(<< "SRTP key usage soft limit reached");
     break;
   case event_key_hard_limit:
     WarningLog(<< "SRTP key usage hard limit reached");
     break;
   case event_packet_index_limit:
     WarningLog(<< "SRTP packet index limit reached");
     break;
   default:
     WarningLog(<< "SRTP unknown event reported to handler");
   }
 }
 
MediaStream* 
FlowManager::createMediaStream(MediaStreamHandler& mediaStreamHandler,
                               const StunTuple& localBinding, 
                               bool rtcpEnabled,
                               MediaStream::NatTraversalMode natTraversalMode,
                               const char* natTraversalServerHostname, 
                               unsigned short natTraversalServerPort, 
                               const char* stunUsername,
                               const char* stunPassword)
{
   MediaStream* newMediaStream = 0;
   if(rtcpEnabled)
   {
      StunTuple localRtcpBinding(localBinding.getTransportType(), localBinding.getAddress(), localBinding.getPort() + 1);
      newMediaStream = new MediaStream(mIOService,
#ifdef USE_SSL
                                       mSslContext,
#endif
                                       mediaStreamHandler,
                                       localBinding,
                                       localRtcpBinding,
#ifdef USE_SSL
                                       mDtlsFactory,
#endif 
                                       natTraversalMode,
                                       natTraversalServerHostname, 
                                       natTraversalServerPort, 
                                       stunUsername, 
                                       stunPassword);
   }
   else
   {
      StunTuple rtcpDisabled;  // Default constructor sets transport type to None - this signals Rtcp is disabled
      newMediaStream = new MediaStream(mIOService,
#ifdef USE_SSL
                                       mSslContext, 
#endif
                                       mediaStreamHandler, 
                                       localBinding, 
                                       rtcpDisabled, 
#ifdef USE_SSL
                                       mDtlsFactory,
#endif 
                                       natTraversalMode, 
                                       natTraversalServerHostname, 
                                       natTraversalServerPort, 
                                       stunUsername, 
                                       stunPassword);
   }
   return newMediaStream;
}

#ifdef USE_SSL 
int 
FlowManager::createCert(const resip::Data& pAor, int expireDays, int keyLen, X509*& outCert, EVP_PKEY*& outKey )
{
   int ret;
   
   Data aor = "sip:" + pAor;
   
   // Make sure that necessary algorithms exist:
   resip_assert(EVP_sha1());

   RSA* rsa = RSA_generate_key(keyLen, RSA_F4, NULL, NULL);
   resip_assert(rsa);    // couldn't make key pair
   
   EVP_PKEY* privkey = EVP_PKEY_new();
   resip_assert(privkey);
   ret = EVP_PKEY_set1_RSA(privkey, rsa);
   resip_assert(ret);

   X509* cert = X509_new();
   resip_assert(cert);
   
   X509_NAME* subject = X509_NAME_new();
   X509_EXTENSION* ext = X509_EXTENSION_new();
   
   // set version to X509v3 (starts from 0)
   X509_set_version(cert, 2L);
   
   int serial = Random::getRandom();  // get an int worth of randomness
   resip_assert(sizeof(int)==4);
   ASN1_INTEGER_set(X509_get_serialNumber(cert),serial);
   
//    ret = X509_NAME_add_entry_by_txt( subject, "O",  MBSTRING_ASC, 
//                                      (unsigned char *) domain.data(), domain.size(), 
//                                      -1, 0);
   resip_assert(ret);
   ret = X509_NAME_add_entry_by_txt( subject, "CN", MBSTRING_ASC, 
                                     (unsigned char *) aor.data(), aor.size(), 
                                     -1, 0);
   resip_assert(ret);
   
   ret = X509_set_issuer_name(cert, subject);
   resip_assert(ret);
   ret = X509_set_subject_name(cert, subject);
   resip_assert(ret);
   
   const long duration = 60*60*24*expireDays;   
   X509_gmtime_adj(X509_get_notBefore(cert),0);
   X509_gmtime_adj(X509_get_notAfter(cert), duration);
   
   ret = X509_set_pubkey(cert, privkey);
   resip_assert(ret);
   
   Data subjectAltNameStr = Data("URI:sip:") + aor
      + Data(",URI:im:")+aor
      + Data(",URI:pres:")+aor;
   ext = X509V3_EXT_conf_nid( NULL , NULL , NID_subject_alt_name, 
                              (char*) subjectAltNameStr.c_str() );
   X509_add_ext( cert, ext, -1);
   X509_EXTENSION_free(ext);
   
   static char CA_FALSE[] = "CA:FALSE";
   ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, CA_FALSE);
   ret = X509_add_ext( cert, ext, -1);
   resip_assert(ret);
   X509_EXTENSION_free(ext);
   
   // TODO add extensions NID_subject_key_identifier and NID_authority_key_identifier
   
   ret = X509_sign(cert, privkey, EVP_sha1());
   resip_assert(ret);

   outCert = cert;
   outKey = privkey;
   return ret; 
}
#endif

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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
