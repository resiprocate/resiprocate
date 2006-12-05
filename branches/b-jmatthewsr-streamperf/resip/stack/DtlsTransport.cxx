#include "precompile.h"

#ifdef USE_DTLS

#if defined(HAVE_CONFIG_H)
  #include "resip/stack/config.hxx"
#endif

#include <memory>

#ifndef RESIP_COMPAT_HXX
#include "rutil/compat.hxx"
#endif

#ifndef RESIP_DATA_HXX
#include "rutil/Data.hxx"
#endif

#ifndef RESIP_DNSUTIL_HXX
#include "rutil/DnsUtil.hxx"
#endif

#ifndef RESIP_SOCKET_HXX
#include "rutil/Socket.hxx"
#endif

#ifndef RESIP_LOGGER_HXX
#include "rutil/Logger.hxx"
#endif

#ifndef RESIP_SIPMESSAGE_HXX
#include "resip/stack/SipMessage.hxx"
#endif

#ifndef RESIP_HELPER_HXX
#include "resip/stack/Helper.hxx"
#endif

#ifndef RESIP_SECURITY_HXX
#include "resip/stack/Security.hxx"
#endif

#ifndef RESIP_DTLSMESSAGE_HXX
#include "resip/stack/DtlsMessage.hxx"
#endif

#ifndef RESIP_DTLSTRANSPORT_HXX
#include "resip/stack/DtlsTransport.hxx"
#endif

#include "rutil/WinLeakCheck.hxx"

#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/StateChanges.h>
#include <osc/SigcompMessage.h>
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

DtlsTransport::DtlsTransport(Fifo<TransactionMessage>& fifo,
                             int portNum,
                             IpVersion version,
                             const Data& interfaceObj,
                             Security& security,
                             const Data& sipDomain,
                             Compression& compression)
                             : UdpTransport( fifo, portNum, version, 
                                 StunDisabled, interfaceObj, 0, compression ),
                               mTimer( mHandshakePending ),
                               mSecurity( &security ),
                               mDomain(sipDomain)
{
   setTlsDomain(sipDomain);   
   InfoLog ( << "Creating DTLS transport host=" << interfaceObj 
             << " port=" << portNum
             << " ipv4=" << version ) ;

   mTuple.setType( transport() );

   mClientCtx = SSL_CTX_new( DTLSv1_client_method() ) ;
   mServerCtx = SSL_CTX_new( DTLSv1_server_method() ) ;
   assert( mClientCtx ) ;
   assert( mServerCtx ) ;

   mDummyBio = BIO_new( BIO_s_mem() ) ;
   assert( mDummyBio ) ;

   mSendData = NULL ;

   /* trying to read from this BIO always returns retry */
   BIO_set_mem_eof_return( mDummyBio, -1 ) ;
}

DtlsTransport::~DtlsTransport()
{
   DebugLog (<< "Shutting down " << mTuple);

   while(mDtlsConnections.begin() != mDtlsConnections.end())
   {
       _cleanupConnectionState(mDtlsConnections.begin()->second, mDtlsConnections.begin()->first);
   }       
   SSL_CTX_free(mClientCtx);mClientCtx=0;  
   SSL_CTX_free(mServerCtx);mServerCtx=0;  

   BIO_free( mDummyBio) ;
}

void
DtlsTransport::_read( FdSet& fdset )
{
   //should this buffer be allocated on the stack and then copied out, as it
   //needs to be deleted every time EWOULDBLOCK is encountered
   // .dlb. can we determine the size of the buffer before we allocate?
   // something about MSG_PEEK|MSG_TRUNC in Stevens..
   // .dlb. RFC3261 18.1.1 MUST accept 65K datagrams. would have to attempt to
   // adjust the UDP buffer as well...
   unsigned int bufferLen = UdpTransport::MaxBufferSize + 5 ;
   char* buffer = new char[ bufferLen ] ;
   unsigned char *pt = new unsigned char[ bufferLen ] ;

   SSL *ssl ;
   BIO *rbio ;
   BIO *wbio ;
   
   // !jf! how do we tell if it discarded bytes 
   // !ah! we use the len-1 trick :-(
   Tuple tuple(mTuple) ;
   socklen_t slen = tuple.length() ;
   int len = recvfrom( mFd,
                       buffer,
                       UdpTransport::MaxBufferSize,
                       0 /*flags */,
                       &tuple.getMutableSockaddr(), 
                       &slen ) ;
   if ( len == SOCKET_ERROR )
   {
      int err = getErrno() ;
      if ( err != EWOULDBLOCK  )
      {
         error( err ) ;
      }
   }
   
   if (len == 0 || len == SOCKET_ERROR)
   {
      delete [] buffer ; 
      buffer = 0 ;
      return ;
   }

   if ( len + 1 >= UdpTransport::MaxBufferSize )
   {
      InfoLog (<<"Datagram exceeded max length "<<UdpTransport::MaxBufferSize ) ;
      delete [] buffer ; buffer = 0 ;
      return ;
   }

   //DebugLog ( << "UDP Rcv : " << len << " b" );
   //DebugLog ( << Data(buffer, len).escaped().c_str());
   
   /* begin SSL stuff */
   struct sockaddr peer = tuple.getMutableSockaddr() ;

   ssl = mDtlsConnections[ *((struct sockaddr_in *)&peer) ] ;

   /* 
    * If we don't have a binding for this peer,
    * then we're a server.
    */
   if ( ssl == NULL )
   {
      ssl = SSL_new( mServerCtx ) ;
      assert( ssl ) ;

      SSL_set_accept_state( ssl ) ;

      X509 *cert = mSecurity->getDomainCert( mDomain ) ;
      EVP_PKEY *pkey = mSecurity->getDomainKey( mDomain ) ;

      if( !cert )
      {
         Data error = Data("Could not load certifiacte for domain: ") 
             + mDomain;
         throw Security::Exception( error,__FILE__, __LINE__ ) ;
      }

      if( !pkey )
      {
         Data error = Data("Could not load private key for domain: ") 
             + mDomain;
         throw Security::Exception( error,__FILE__, __LINE__ ) ;
      }

      assert( cert ) ;
      assert( pkey ) ;

      if( ! SSL_use_certificate( ssl, cert ) )
      {
         throw Security::Exception( "SSL_use_certificate failed",
                                   __FILE__, __LINE__ ) ;
      }

      if ( ! SSL_use_PrivateKey( ssl, pkey ) )
         throw Security::Exception( "SSL_use_PrivateKey failed.",
                                   __FILE__, __LINE__ ) ;

      wbio = BIO_new_dgram( mFd, BIO_NOCLOSE ) ;
      assert( wbio ) ;

      BIO_dgram_set_peer( wbio, &peer ) ;

      SSL_set_bio( ssl, NULL, wbio ) ;

      /* remember this connection */
      mDtlsConnections[ *((struct sockaddr_in *)&peer) ] = ssl ;
   }

   rbio = BIO_new_mem_buf( buffer, len ) ;
   BIO_set_mem_eof_return( rbio, -1 ) ;

   ssl->rbio = rbio ;

   len = SSL_read( ssl, pt, UdpTransport::MaxBufferSize ) ;
   int err = SSL_get_error( ssl, len ) ;

   /* done with the rbio */
   BIO_free( ssl->rbio ) ;
   ssl->rbio = mDummyBio ;
   delete [] buffer ;
   buffer = 0 ;

   if ( len <= 0 )
   {
      switch( err )
      {
         case SSL_ERROR_NONE:
            break ;
         case SSL_ERROR_SSL:
            break ;
         case SSL_ERROR_WANT_READ:
            break ;
         case SSL_ERROR_WANT_WRITE:
            break ;
         case SSL_ERROR_SYSCALL:
            break ;
            /* connection closed */
         case SSL_ERROR_ZERO_RETURN:
            _cleanupConnectionState( ssl, *((struct sockaddr_in *)&peer) ) ;
            break ;
         case SSL_ERROR_WANT_CONNECT:
            break ;
         case SSL_ERROR_WANT_ACCEPT:
            break ;
         default:
            break ;
      }
   }

   if ( len <= 0 )
       return ;

   if ( SSL_in_init( ssl ) )
      mTimer.add( ssl, DtlsReceiveTimeout ) ;

#ifdef USE_SIGCOMP
   osc::StateChanges *sc = 0;
#endif

   if ((pt[0] & 0xf8) == 0xf8)
   {
      if(!mCompression.isEnabled())
      {
        InfoLog(<< "Discarding unexpected SigComp message");
        delete [] pt;
        return;
      }
#ifdef USE_SIGCOMP
      unsigned char *newPt = new unsigned char[ bufferLen ] ;
      size_t uncompressedLength =
        mSigcompStack->uncompressMessage(pt, len,
                                         newPt, UdpTransport::MaxBufferSize,
                                         sc);

      DebugLog (<< "Unompressed message from "
                << len << " bytes to " 
                << uncompressedLength << " bytes");

      osc::SigcompMessage *nack = mSigcompStack->getNack();

      if (nack)
      {
        mTxFifo.add(new SendData(tuple,
                                 Data(nack->getDatagramMessage(),
                                      nack->getDatagramLength()),
                                 Data::Empty,
                                 Data::Empty,
                                 true)
                   );
        delete nack;
      }

      delete[] buffer;
      buffer = newBuffer;
      len = uncompressedLength;
#endif
   }

   SipMessage* message = new SipMessage(this);
   
   // set the received from information into the received= parameter in the
   // via
   
   // It is presumed that UDP Datagrams are arriving atomically and that
   // each one is a unique SIP message
   
   
   // Save all the info where this message came from
   tuple.transport = this ;
   message->setSource( tuple ) ;
   //DebugLog (<< "Received from: " << tuple);
   
   // Tell the SipMessage about this datagram buffer.
   message->addBuffer( (char *)pt ) ;
   
   mMsgHeaderScanner.prepareForMessage( message ) ;
   
   char *unprocessedCharPtr ;
   if (mMsgHeaderScanner.scanChunk( (char *)pt,
                                    len,
                                    &unprocessedCharPtr ) !=
       MsgHeaderScanner::scrEnd)
   {
      DebugLog( << "Scanner rejecting datagram as unparsable / fragmented from " 
                << tuple ) ;
      DebugLog( << Data( pt, len ) ) ;
      delete message ;
      message = 0 ;
      return ;
   }
   
   // no pp error
   int used = unprocessedCharPtr - (char *)pt ;

   if ( used < len )
   {
      // body is present .. add it up.
      // NB. The Sip Message uses an overlay (again)
      // for the body. It ALSO expects that the body
      // will be contiguous (of course).
      // it doesn't need a new buffer in UDP b/c there
      // will only be one datagram per buffer. (1:1 strict)
      
      message->setBody( (char *)pt + used, len - used ) ;
      //DebugLog(<<"added " << len-used << " byte body");
   }
   
   if ( ! basicCheck( *message ) )
   {
      delete message ; // cannot use it, so, punt on it...
      // basicCheck queued any response required
      message = 0 ;
      return ;
   }
   
   stampReceived( message) ;

#ifdef USE_SIGCOMP
      if (mCompression.isEnabled() && sc)
      {
        const Via &via = message->header(h_Vias).front();
        if (message->isRequest())
        {
          // For requests, the compartment ID is read out of the
          // top via header field; if not present, we use the
          // TCP connection for identification purposes.
          if (via.exists(p_sigcompId))
          {
            Data compId = via.param(p_sigcompId);
            mSigcompStack->provideCompartmentId(
                             sc, compId.data(), compId.size());
          }
          else
          {
            mSigcompStack->provideCompartmentId(sc, this, sizeof(this));
          }
        }
        else
        {
          // For responses, the compartment ID is supposed to be
          // the same as the compartment ID of the request. We
          // *could* dig down into the transaction layer to try to
          // figure this out, but that's a royal pain, and a rather
          // severe layer violation. In practice, we're going to ferret
          // the ID out of the the Via header field, which is where we
          // squirreled it away when we sent this request in the first place.
          Data compId = via.param(p_branch).getSigcompCompartment();
          mSigcompStack->provideCompartmentId(sc, compId.data(), compId.size());
        }

      }
#endif
   
   mStateMachineFifo.add( message ) ;
}

void DtlsTransport::_write( FdSet& fdset )
{
   SSL *ssl ;
   BIO *wBio ;
   int retry = 0 ;

   SendData *sendData ;
   if ( mSendData != NULL )
       sendData = mSendData ;
   else
       sendData = mTxFifo.getNext() ;

   //DebugLog (<< "Sent: " <<  sendData->data);
   //DebugLog (<< "Sending message on udp.");
   
   assert( &(*sendData) );
   assert( sendData->destination.getPort() != 0 );
   
   sockaddr peer = sendData->destination.getSockaddr();
   
   ssl = mDtlsConnections[ *((struct sockaddr_in *)&peer) ] ;

   /* If we don't have a binding, then we're a client */
   if ( ssl == NULL )
   {
      ssl = SSL_new( mClientCtx ) ;
      assert( ssl ) ;

      SSL_set_connect_state( ssl ) ;

      wBio = BIO_new_dgram( mFd, BIO_NOCLOSE ) ;
      assert( wBio ) ;
      
      BIO_dgram_set_peer( wBio, &peer) ;

      /* the real rbio will be set by _read */
      SSL_set_bio( ssl, mDummyBio, wBio ) ;

      /* we should be ready to take this out if the 
       * connection fails later */
      mDtlsConnections [ *((struct sockaddr_in *)&peer) ] = ssl ;
   }

   int expected;
   int count;

#ifdef USE_SIGCOMP
   // If message needs to be compressed, compress it here.
   if (mSigcompStack &&
       sendData->sigcompId.size() > 0 &&
       !sendData->isAlreadyCompressed )
   {
       osc::SigcompMessage *sm = mSigcompStack->compressMessage
         (sendData->data.data(), sendData->data.size(),
          sendData->sigcompId.data(), sendData->sigcompId.size(),
          isReliable());

       DebugLog (<< "Compressed message from "
                 << sendData->data.size() << " bytes to " 
                 << sm->getDatagramLength() << " bytes");

       expected = sm->getDatagramLength();

       count = SSL_Write(ssl,
                         sm->getDatagramMessage(),
                         sm->getDatagramLength());
       delete sm;
   }
   else
#endif
   {
      expected = sendData->data.size();

      count = SSL_write(ssl, sendData->data.data(), 
                        sendData->data.size());
   }

   /* 
    * all reads go through _read, so the most likely result during a handshake 
    * will be SSL_ERROR_WANT_READ 
    */

   if ( count <= 0 )
   {
      /* cache unqueued data */
      mSendData = sendData ; 

      int err = SSL_get_error( ssl, count ) ;
      switch( err )
      {
         case SSL_ERROR_NONE:
            break;
         case SSL_ERROR_SSL:
            break;
         case SSL_ERROR_WANT_READ:
            retry = 1 ;
            break;
         case SSL_ERROR_WANT_WRITE:         
             retry = 1 ;
             fdset.setWrite(mFd);
            break;
         case SSL_ERROR_SYSCALL:
            {
               int e = getErrno();
               error(e);
               InfoLog (<< "Failed (" << e << ") sending to " 
                        << sendData->destination);
               fail(sendData->transactionId);
               break;
            }
         case SSL_ERROR_ZERO_RETURN:
            _cleanupConnectionState( ssl, *((struct sockaddr_in *)&peer) ) ;
            break ;
         case SSL_ERROR_WANT_CONNECT:
            break;
         case SSL_ERROR_WANT_ACCEPT:
            break;
         default:
            break ;
      }
   }
   else
   {
      mSendData = NULL ;
   }

   /* 
    * ngm: is sendData deleted by a higher layer?  Seems to be the case after 
    * checking with UdpTransport
    */
   
   if ( ! retry && count != int(sendData->data.size()) )
   {
      ErrLog (<< "UDPTransport - send buffer full" );
      fail(sendData->transactionId);
   }
}

void
DtlsTransport::_doHandshake( void )
{
   DtlsMessage *msg = mHandshakePending.getNext() ;
   SSL *ssl = msg->getSsl() ;

   delete msg ;

   int ret = SSL_do_handshake( ssl ) ;

   switch( ret )
   {
     case SSL_ERROR_NONE:
            break;
         case SSL_ERROR_SSL:
            break;
         case SSL_ERROR_WANT_READ:
            break;
         case SSL_ERROR_WANT_WRITE:         
            break;
         case SSL_ERROR_SYSCALL:
            break;
         case SSL_ERROR_ZERO_RETURN:
            break;
         case SSL_ERROR_WANT_CONNECT:
            break;
         case SSL_ERROR_WANT_ACCEPT:
            break;
         default:
            break ;
   }
}

void 
DtlsTransport::process(FdSet& fdset)
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   mTimer.process() ;

   while ( mHandshakePending.messageAvailable() )
      _doHandshake() ;

   if ( ( mSendData != NULL || mTxFifo.messageAvailable() )
       && fdset.readyToWrite( mFd ) )
      _write( fdset ) ;
   
   // !jf! this may have to change - when we read a message that is too big
   if ( fdset.readyToRead(mFd) )
      _read( fdset ) ;
}


void 
DtlsTransport::buildFdSet( FdSet& fdset )
{
   fdset.setRead(mFd);
    
   if ( mSendData != NULL || mTxFifo.messageAvailable() )
   {
     fdset.setWrite(mFd);
   }
}

void
DtlsTransport::_cleanupConnectionState( SSL *ssl, struct sockaddr_in peer )
{
   /* 
    * SSL_free decrements the ref-count for mDummyBio by 1, so
    * add 1 to the ref-count to make sure it does not get free'd
    */
   CRYPTO_add( &mDummyBio->references, 1, CRYPTO_LOCK_BIO ) ;
   SSL_free( ssl ) ;
   mDtlsConnections.erase( peer ) ;
}

void
DtlsTransport::_mapDebug( const char *where, const char *action, SSL *ssl )
{
   fprintf( stderr, "%s: %s\t%p\n", where, action, ssl ) ;
   fprintf( stderr, "map sizet = %d\n", mDtlsConnections.size() ) ;
}

void
DtlsTransport::_printSock( const struct sockaddr_in *sock )
{
   fprintf( stderr, "addr = %s\t port = %d\n", inet_ntoa( sock->sin_addr ), 
            ntohs( sock->sin_port ) ) ;
}

#endif /* USE_DTLS */

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
