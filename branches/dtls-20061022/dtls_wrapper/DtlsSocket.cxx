#include <iostream>
#include <cassert>
#include <string.h>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "bf_dwrap.h"
using namespace std;
using namespace dtls;

// Our local timers
class dtls::DtlsSocketTimer : public DtlsTimer
{
  public:
     DtlsSocketTimer(unsigned int seq,DtlsSocket *socket): DtlsTimer(seq),mSocket(socket){}
     ~DtlsSocketTimer()
      {
      }
     
      void expired()
      {
         mSocket->expired(this);        
      }
  private:
     DtlsSocket *mSocket;
};

DtlsSocket::~DtlsSocket()
{
   mReadTimer->invalidate();
}

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType type):
   mSocketContext(socketContext),
   mFactory(factory),
   mReadTimer(0),
   mSocketType(type), 
   mHandshakeCompleted(false)
 {  mSocketContext->setDtlsSocket(this);
    
  assert(factory->mContext);
  ssl=SSL_new(factory->mContext);
  assert(ssl!=0);


  switch(type){
    case Client:
      SSL_set_connect_state(ssl);
      break;
    case Server:
      SSL_set_accept_state(ssl);
//      SSL_set_verify(ssl,SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,0);
      break;
    default:
      assert(0);
  }

  mInBio=BIO_new(BIO_f_dwrap());
  BIO_push(mInBio,BIO_new(BIO_s_mem()));
    
  mOutBio=BIO_new(BIO_f_dwrap());
  BIO_push(mOutBio,BIO_new(BIO_s_mem()));
    
  SSL_set_bio(ssl,mInBio,mOutBio);
}


void
DtlsSocket::expired(DtlsSocketTimer* timer)
{
   forceRetransmit();
   //delete timer;

   //assert(timer == mReadTimer);   
   //mReadTimer = 0;
}

void 
DtlsSocket::startClient()
{
   assert(mSocketType == Client);

   doHandshakeIteration();
}

bool
DtlsSocket::handlePacketMaybe(const unsigned char* bytes, unsigned int len){
  DtlsFactory::PacketType pType=DtlsFactory::demuxPacket(bytes,len);

  if(pType!=DtlsFactory::dtls)
    return false;
  
  int r;
  BIO_reset(mInBio);
  BIO_reset(mOutBio);
  
  r=BIO_write(mInBio,bytes,len);
  assert(r==(int)len);  // Can't happen

  // Note: we must catch any below exceptions--if there are any
  doHandshakeIteration();

  return true;
}

void
DtlsSocket::forceRetransmit(){
  BIO_reset(mInBio);
  BIO_reset(mOutBio);
  BIO_ctrl(mInBio,BIO_CTRL_DGRAM_SET_RECV_TIMEOUT,0,0);
    
  doHandshakeIteration();
}

void
DtlsSocket::doHandshakeIteration() {
  int r;
  char errbuf[1024];
  int sslerr;
  
  if(mHandshakeCompleted)
     return;

  r=SSL_do_handshake(ssl);
  errbuf[0]=0;
  ERR_error_string_n(ERR_peek_error(),errbuf,sizeof(errbuf));

  // See what was written
  int outBioLen;
  unsigned char *outBioData;  
  outBioLen=BIO_get_mem_data(mOutBio,&outBioData);
 
  // Now handle handshake errors */
  switch(sslerr=SSL_get_error(ssl,r)){
    case SSL_ERROR_NONE:
       mHandshakeCompleted = true;       
       mSocketContext->handshakeCompleted();
       if(mReadTimer) mReadTimer->invalidate();
       break;
    case SSL_ERROR_WANT_READ:
      // There are two cases here:
      // (1) We didn't get enough data. In this case we leave the
      //     timers alone and wait for more packets.
      // (2) We did get a full flight and then handled it, but then
      //     wrote some more message and now we need to flush them
      //     to the network and now reset the timers
      //
      // If data was written then this means we got a complete
      // something or a retransmit so we need to reset the timer
      if(outBioLen){
        if(mReadTimer) mReadTimer->invalidate();
        mReadTimer=new DtlsSocketTimer(0,this);
        mFactory->mTimerContext->addTimer(mReadTimer,getReadTimeout());
      }

      break;
    default:
      cerr << "SSL error " << sslerr << endl;
      
      mSocketContext->handshakeFailed(errbuf);
      // Note: need to fall through to propagate alerts, if any
      break;
  }

  // If mOutBio is now nonzero-length, then we need to write the
  // data to the network. TODO: warning, MTU issues! 
  if(outBioLen)
  {
     //cerr << "Writing data: ";
     //cerr.write((char*)outBioData, outBioLen);
     //cerr << " length " << outBioLen << endl;
     mSocketContext->write(outBioData,outBioLen);
  }
}

bool
DtlsSocket::getRemoteFingerprint(char *fprint){
  X509 *x;
  
  x=SSL_get_peer_certificate(ssl);
  if(!x) // No certificate
    return false;

  computeFingerprint(x,fprint);

  return true;
}

bool
DtlsSocket::checkFingerprint(const char* fingerprint, unsigned int len){
  char fprint[100];

  if(getRemoteFingerprint(fprint)==false)
    return false;
  
  // used to be strncasecmp
  if(strncmp(fprint,fingerprint,len)){
    cerr << "Fingerprint mismatch, got " << fprint << "expecting " << fingerprint << endl;
    return false;
  }
  
  return true;
}

SrtpSessionKeys
DtlsSocket::getSrtpSessionKeys()
{
   //TODO: probably an exception candidate
   assert(mHandshakeCompleted);
   SrtpSessionKeys keys;

   memset(&keys, 0x00, sizeof(keys));
   
   SSL_get_srtp_key_info(ssl, 
                         &keys.clientMasterKey,
                         &keys.clientMasterKeyLen,
                         &keys.serverMasterKey,
                         &keys.serverMasterKeyLen,
                         &keys.clientMasterSalt,
                         &keys.clientMasterSaltLen,
                         &keys.serverMasterSalt,
                         &keys.serverMasterSaltLen);
   return keys;
   
}

SRTP_PROTECTION_PROFILE*
DtlsSocket::getSrtpProfile()
{
   //TODO: probably an exception candidate
   assert(mHandshakeCompleted);
   return SSL_get_selected_srtp_profile(ssl);
}
   
void
DtlsSocket::getMyCertFingerprint(char *fingerprint){
  mFactory->getMyCertFingerprint(fingerprint);
}
     
// Fingerprint is assumed to be long enough
void
DtlsSocket::computeFingerprint(X509 *cert, char *fingerprint) {
  unsigned char md[EVP_MAX_MD_SIZE];
  int r;
  unsigned int i,n;
  
  r=X509_digest(cert,EVP_sha1(),md,&n);
  assert(r==1);

  for(i=0;i<n;i++){
    sprintf(fingerprint,"%02X",md[i]);
    fingerprint+=2;
    
    if(i<(n-1))
      *fingerprint++=':';
    else
      *fingerprint++=0;
  }
}

//srtp_t is a pointer(yes, a pointer) to an srtp session from libsrtp. Lifetime
//is managed by the client.
//TODO: assert(0) into exception, as elsewhere
void
DtlsSocket::createSrtpSessionPolicies(srtp_policy_t& outboundPolicy, srtp_policy_t& inboundPolicy)
{
   assert(mHandshakeCompleted);
   
   /* we assume that the defau1lt profile is in effect, for now */
   srtp_profile_t profile = srtp_profile_aes128_cm_sha1_80;
   int key_len = srtp_profile_get_master_key_length(profile);
   int salt_len = srtp_profile_get_master_salt_length(profile);

   /* get keys from srtp_key and initialize the inbound and outbound sessions */
   uint8_t *client_master_key_and_salt=new uint8_t[SRTP_MAX_KEY_LEN];
   uint8_t *server_master_key_and_salt=new uint8_t[SRTP_MAX_KEY_LEN];
   srtp_policy_t client_policy;
   srtp_policy_t server_policy;

   SrtpSessionKeys srtp_key = getSrtpSessionKeys();   
   /* set client_write key */  //Dragos--direct assignment then memcpy? Look
                               //into this...
   client_policy.key = client_master_key_and_salt;
   if (srtp_key.clientMasterKeyLen != key_len)
   {
     cout << "error: unexpected client key length" << endl;
     assert(0);
   }
   if (srtp_key.clientMasterSaltLen != salt_len)
   {
      cout << "error: unexpected client salt length" << endl;
      assert(0);      
   }

   memcpy(client_master_key_and_salt, srtp_key.clientMasterKey, key_len);
   memcpy(client_master_key_and_salt + key_len, srtp_key.clientMasterSalt, salt_len);
   
   cout << "client master key and salt: " << 
      octet_string_hex_string(client_master_key_and_salt, key_len + salt_len) << endl;

   /* initialize client SRTP policy from profile  */
   err_status_t err = crypto_policy_set_from_profile_for_rtp(&client_policy.rtp, profile);
   if (err) assert(0);
   
   err = crypto_policy_set_from_profile_for_rtcp(&client_policy.rtcp, profile);
   if (err) assert(0);
   client_policy.ssrc.type  = ssrc_any_inbound;
   client_policy.next = NULL;

   /* set server_write key */
   //Dragos--direct assignment then memcpy? Look into this...
   server_policy.key = server_master_key_and_salt;

   if (srtp_key.serverMasterKeyLen != key_len)
   {
     cout << "error: unexpected server key length" << endl;
     assert(0);
   }
   if (srtp_key.serverMasterSaltLen != salt_len)
   {
      cout << "error: unexpected salt length" << endl;
      assert(0);      
   }

   memcpy(server_master_key_and_salt, srtp_key.serverMasterKey, key_len);
   memcpy(server_master_key_and_salt + key_len, srtp_key.serverMasterSalt, salt_len);
   cout << "server master key and salt: " << 
     octet_string_hex_string(server_master_key_and_salt, key_len + salt_len) << endl;

   /* initialize server SRTP policy from profile  */
   err = crypto_policy_set_from_profile_for_rtp(&server_policy.rtp, profile);
   if (err) assert(0);
   
   err = crypto_policy_set_from_profile_for_rtcp(&server_policy.rtcp, profile);
   if (err) assert(0);
   server_policy.ssrc.type  = ssrc_any_inbound;
   server_policy.next = NULL;

   if (mSocketType == Client) 
   {
      outboundPolicy = client_policy;
      inboundPolicy = server_policy;
   }
   else
   {
      outboundPolicy = server_policy;
      inboundPolicy = client_policy;
   }
   /* zeroize the input keys (but not the srtp session keys that are in use) */
//not done...not much of a security whole imho...the lifetime of these seems odd though
//    memset(client_master_key_and_salt, 0x00, SRTP_MAX_KEY_LEN);
//    memset(server_master_key_and_salt, 0x00, SRTP_MAX_KEY_LEN);
//    memset(&srtp_key, 0x00, sizeof(srtp_key));
}

// Wrapper for currently nonexistent OpenSSL fxn
int
DtlsSocket::getReadTimeout()
  {
    return 500;
  }
