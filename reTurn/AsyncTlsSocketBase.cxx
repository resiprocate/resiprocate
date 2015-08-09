
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "AsyncTlsSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

AsyncTlsSocketBase::AsyncTlsSocketBase(asio::io_service& ioService, asio::ssl::context& context, bool validateServerCertificateHostname) 
   : AsyncSocketBase(ioService),
   mSocket(ioService, context),
   mResolver(ioService),
   mValidateServerCertificateHostname(validateServerCertificateHostname)
{
}

AsyncTlsSocketBase::~AsyncTlsSocketBase() 
{
}

unsigned int 
AsyncTlsSocketBase::getSocketDescriptor() 
{ 
   return (unsigned int)mSocket.lowest_layer().native(); 
}

asio::error_code 
AsyncTlsSocketBase::bind(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   mSocket.lowest_layer().open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.lowest_layer().set_option(asio::ip::tcp::socket::reuse_address(true), errorCode);
      mSocket.lowest_layer().set_option(asio::ip::tcp::no_delay(true), errorCode); // ?slg? do we want this?
      mSocket.lowest_layer().bind(asio::ip::tcp::endpoint(address, port), errorCode);
   }
   return errorCode;
}

void 
AsyncTlsSocketBase::connect(const std::string& address, unsigned short port)
{
   mHostname = address;

   // Start an asynchronous resolve to translate the address
   // into a list of endpoints.
   resip::Data service(port);
#ifdef USE_IPV6
   asio::ip::tcp::resolver::query query(address, service.c_str());   
#else
   asio::ip::tcp::resolver::query query(asio::ip::tcp::v4(), address, service.c_str());   
#endif
   mResolver.async_resolve(query,
        boost::bind(&AsyncSocketBase::handleTcpResolve, shared_from_this(),
                    asio::placeholders::error,
                    asio::placeholders::iterator));
}

void 
AsyncTlsSocketBase::handleTcpResolve(const asio::error_code& ec,
                                  asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      //asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void 
AsyncTlsSocketBase::handleConnect(const asio::error_code& ec,
                                  asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // The connection was successful - now do handshake.
      mSocket.async_handshake(asio::ssl::stream_base::client, 
                              boost::bind(&AsyncSocketBase::handleClientHandshake, shared_from_this(), 
                                          asio::placeholders::error, endpoint_iterator));
   }
   else if (++endpoint_iterator != asio::ip::tcp::resolver::iterator())
   {
      // The connection failed. Try the next endpoint in the list.
      asio::error_code ec;
      mSocket.lowest_layer().close(ec);
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void 
AsyncTlsSocketBase::handleClientHandshake(const asio::error_code& ec,
                                          asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // The handshake was successful.
      mConnected = true;
      mConnectedAddress = endpoint_iterator->endpoint().address();
      mConnectedPort = endpoint_iterator->endpoint().port();

      // Validate that hostname in cert matches connection hostname
      if(!mValidateServerCertificateHostname || validateServerCertificateHostname())
      {
         onConnectSuccess();
      }
      else
      {
         WarningLog(<< "Hostname in certificate does not match connection hostname!");
         onConnectFailure(asio::error::operation_aborted);
      }
   }
   else if (++endpoint_iterator != asio::ip::tcp::resolver::iterator())
   {
      // The handshake failed. Try the next endpoint in the list.
      asio::error_code ec;
      mSocket.lowest_layer().close(ec);
      mSocket.lowest_layer().async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

bool 
AsyncTlsSocketBase::validateServerCertificateHostname()
{
   bool valid = false;

   // print session info
   const SSL_CIPHER *ciph;
   ciph=SSL_get_current_cipher(mSocket.impl()->ssl);
   InfoLog( << "TLS session set up with " 
      <<  SSL_get_version(mSocket.impl()->ssl) << " "
      <<  SSL_CIPHER_get_version(ciph) << " "
      <<  SSL_CIPHER_get_name(ciph) << " " );

   // get the certificate - should always exist since mode is set for SSL to verify the cert first
   X509* cert = SSL_get_peer_certificate(mSocket.impl()->ssl);
   resip_assert(cert);

   // Look at the SubjectAltName, and if found, set as peerName
   bool hostnamePresentInSubjectAltName = false;
   GENERAL_NAMES* gens;
   gens = (GENERAL_NAMES*)X509_get_ext_d2i(cert, NID_subject_alt_name, 0, 0);
   for(int i = 0; i < sk_GENERAL_NAME_num(gens); i++)
   {  
      GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);

      DebugLog(<< "subjectAltName of cert contains type <" << gen->type << ">" );

      if (gen->type == GEN_DNS)
      {
         ASN1_IA5STRING* asn = gen->d.dNSName;
         resip::Data dns(asn->data, asn->length);
         InfoLog(<< "subjectAltName of TLS session cert contains DNS <" << dns << ">" );
         hostnamePresentInSubjectAltName = true;
         if(resip::isEqualNoCase(dns, mHostname.c_str()))
         {
            valid = true;
            break;
         }
      }

      if (gen->type == GEN_EMAIL)
      {
         DebugLog(<< "subjectAltName of cert has EMAIL type" );
      }

      if(gen->type == GEN_URI) 
      {
         DebugLog(<< "subjectAltName of cert has URI type" );
      }
   }
   sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

   // If there are no peer names from the subjectAltName, then use the commonName
   if(!hostnamePresentInSubjectAltName)
   {   
      // look at the Common Name to find the peerName of the cert 
      X509_NAME* subject = X509_get_subject_name(cert);
      if(!subject)
      {
         ErrLog( << "Invalid certificate: subject not found ");
      }
   
      int i =-1;
      while( !valid )
      {
         i = X509_NAME_get_index_by_NID(subject, NID_commonName,i);
         if ( i == -1 )
         {
            break;
         }
         resip_assert( i != -1 );
         X509_NAME_ENTRY* entry = X509_NAME_get_entry(subject,i);
         resip_assert( entry );
   
         ASN1_STRING*	s = X509_NAME_ENTRY_get_data(entry);
         resip_assert( s );
   
         int t = M_ASN1_STRING_type(s);
         int l = M_ASN1_STRING_length(s);
         unsigned char* d = M_ASN1_STRING_data(s);
         resip::Data name(d,l);
         DebugLog( << "got x509 string type=" << t << " len="<< l << " data=" << d );
         resip_assert( name.size() == (unsigned)l );
   
         InfoLog( << "Found common name in cert: " << name );      
         if(resip::isEqualNoCase(name, mHostname.c_str()))
         {
            valid = true;
         }
      }
   }
   
   X509_free(cert);
   return valid;
}

void
AsyncTlsSocketBase::doHandshake()
{
   mSocket.async_handshake(asio::ssl::stream_base::server, 
                           boost::bind(&AsyncSocketBase::handleServerHandshake, shared_from_this(), asio::placeholders::error));  
}

void 
AsyncTlsSocketBase::handleServerHandshake(const asio::error_code& e)
{
   if(e)
   {
      onServerHandshakeFailure(e);
   }
   else
   {
      asio::error_code ec;
      mConnectedAddress = mSocket.lowest_layer().remote_endpoint(ec).address();
      mConnectedPort = mSocket.lowest_layer().remote_endpoint(ec).port();

      onServerHandshakeSuccess();
   }
}

const asio::ip::address 
AsyncTlsSocketBase::getSenderEndpointAddress() 
{ 
   return mConnectedAddress; 
}

unsigned short 
AsyncTlsSocketBase::getSenderEndpointPort() 
{ 
   return mConnectedPort; 
}

void 
AsyncTlsSocketBase::transportSend(const StunTuple& destination, std::vector<asio::const_buffer>& buffers)
{
   // Note: destination is ignored for TLS
   asio::async_write(mSocket, buffers, 
                     boost::bind(&AsyncTlsSocketBase::handleSend, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTlsSocketBase::transportReceive()
{
   mSocket.async_read_some(asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE),
                           boost::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));

}

void 
AsyncTlsSocketBase::transportFramedReceive()
{
   asio::async_read(mSocket, asio::buffer((void*)mReceiveBuffer->data(), 4),
                    boost::bind(&AsyncSocketBase::handleReadHeader, shared_from_this(), asio::placeholders::error));
}

void 
AsyncTlsSocketBase::transportClose()
{
   if (mOnBeforeSocketCloseFp)
   {
      mOnBeforeSocketCloseFp(mSocket.lowest_layer().native());
   }

   asio::error_code ec;
   //mSocket.shutdown(ec);  // ?slg? Should we use async_shutdown? !slg! note: this fn gives a stack overflow since ASIO 1.0.0 for some reason
   mSocket.lowest_layer().close(ec);
}

void 
AsyncTlsSocketBase::handleReadHeader(const asio::error_code& e)
{
   if (!e)
   {
      /*
      std::cout << "Read header from tls socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)(*mReceiveBuffer)[i] << "(" << (int)(*mReceiveBuffer)[i] << ") ";
      }
      std::cout << std::endl;
      */

      // Note:  For both StunMessages and ChannelData messages the length in bytes 3 and 4
      UInt16 dataLen;
      memcpy(&dataLen, &(*mReceiveBuffer)[2], 2);
      dataLen = ntohs(dataLen);

      if(((*mReceiveBuffer)[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }

      if(dataLen+4 < RECEIVE_BUFFER_SIZE)
      {
         asio::async_read(mSocket, asio::buffer(&(*mReceiveBuffer)[4], dataLen),
                          boost::bind(&AsyncTlsSocketBase::handleReceive, shared_from_this(), asio::placeholders::error, dataLen+4));
      }
      else
      {
         WarningLog(<< "Receive buffer (" << RECEIVE_BUFFER_SIZE << ") is not large enough to accomdate incoming framed data (" << dataLen+4 << ") closing connection.");
         close();
      }
   }
   else if (e != asio::error::operation_aborted)
   {
      if(e != asio::error::eof && e != asio::error::connection_reset)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
      }
      close();
   }
}

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

