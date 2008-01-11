#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace dtls;

DtlsSocket::DtlsSocket(std::auto_ptr<DtlsSocketContext> socketContext, DtlsFactory* factory, enum SocketType type):
  mSocketContext(socketContext),mFactory(factory),mSocketType(type) {
  ssl=SSL_new(factory->mContext);
  switch(type){
    case Client:
      SSL_set_connect_state(ssl);
      break;
    case Server:
      SSL_set_accept_state(ssl);
      break;
    default:
      assert(0);
  }

  mInBio=BIO_new(BIO_s_mem());
  mOutBio=BIO_new(BIO_s_mem());
  SSL_set_bio(ssl,mInBio,mOutBio);
}

void 
DtlsSocket::startClient()
{
   assert(mSocketType == Client);

   doHandshakeIteration();
}


bool
DtlsSocket::handlePacketMaybe(const char* bytes, unsigned int len){
  // TODO: put in demux logic here
  int r;
  bool retval=false;
  BIO_reset(mInBio);
  BIO_reset(mOutBio);
  
  r=BIO_write(mInBio,bytes,len);
  assert(r==(int)len);  // Can't happen

  // Note: we must catch any below exceptions--if there are any
  doHandshakeIteration();

  return retval;
}

void
DtlsSocket::doHandshakeIteration() {
  int r;
  
  r=SSL_do_handshake(ssl);

  // If mOutBio is now nonzero-length, then we need to write the
  // data to the network. TODO: warning, MTU issues! Do this
  // first because we may have to propagate an alert or somesuch
  unsigned char *outBioData;
  int outBioLen;
  
  outBioLen=BIO_get_mem_data(mOutBio,&outBioData);
  if(outBioLen)
    mSocketContext->write(outBioData,outBioLen);

  // Figure out what the result of the handshake was
  switch(SSL_get_error(ssl,r)){
    case SSL_ERROR_NONE:
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
      // TODO: Throw some kind of handshake failure
      mSocketContext->handshakeFailed();
      break;
  }
}


bool
DtlsSocket::checkFingerprint(const char* fingerprint, unsigned int len){
  return false;
}

