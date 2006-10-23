#include <iostream>
#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"
#include "bf_dwrap.h"
using namespace std;
using namespace dtls;

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType type):
   mSocketContext(socketContext),
   mFactory(factory),
   mSocketType(type), 
   mHandshakeCompleted(false)
 {
  mSocketContext->setDtlsSocket(this);
    
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
DtlsSocket::startClient()
{
   assert(mSocketType == Client);

   doHandshakeIteration();
}

bool
DtlsSocket::handlePacketMaybe(const unsigned char* bytes, unsigned int len){
  PacketType pType=DtlsFactory::demuxPacket(bytes,len);

  if(pType!=dtls)
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
  
  r=SSL_do_handshake(ssl);
  errbuf[0]=0;
  ERR_error_string_n(ERR_peek_error(),errbuf,sizeof(errbuf));
  
  // Now handle handshake errors */
  switch(sslerr=SSL_get_error(ssl,r)){
    case SSL_ERROR_NONE:
       mHandshakeCompleted = true;       
       mSocketContext->handshakeCompleted();
       break;
    case SSL_ERROR_WANT_READ:
      // There are two cases here:
      // (1) We didn't get enough data. In this case we leave the
      //     timers alone and wait for more packets.
      // (2) We did get a full flight and then handled it, but then
      //     wrote some more message and now we need to flush them
      //     to the network and now reset the timers
      // TODO: reset the timers 
      break;
    default:
      cerr << "SSL error " << sslerr << endl;
      
      mSocketContext->handshakeFailed(errbuf);
      // Note: need to fall through to propagate alerts, if any
      break;
  }

  // If mOutBio is now nonzero-length, then we need to write the
  // data to the network. TODO: warning, MTU issues! 
  unsigned char *outBioData;
  int outBioLen;
  
  outBioLen=BIO_get_mem_data(mOutBio,&outBioData);
  if(outBioLen)
    mSocketContext->write(outBioData,outBioLen);
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
  
  if(strncasecmp(fprint,fingerprint,len)){
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

