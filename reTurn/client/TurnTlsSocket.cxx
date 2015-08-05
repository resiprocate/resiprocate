
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL
#include <boost/bind.hpp>

#include "TurnTlsSocket.hxx"
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

TurnTlsSocket::TurnTlsSocket(bool validateServerCertificateHostname, 
                             const asio::ip::address& address, 
                             unsigned short port) : 
   TurnTcpSocket(address,port),
   mSslContext(mIOService, asio::ssl::context::tlsv1),  // TLSv1.0
   mSocket(mIOService, mSslContext),
   mValidateServerCertificateHostname(validateServerCertificateHostname)
{
   mLocalBinding.setTransportType(StunTuple::TLS);

   // Setup SSL context
   
   // Enable certificate validation
   mSslContext.set_verify_mode(asio::ssl::context::verify_peer |   // Verify the peer.
                               asio::ssl::context::verify_fail_if_no_peer_cert);  // Fail verification if the peer has no certificate.
 
   // File that should contain all required root certificates
   mSslContext.load_verify_file("ca.pem");

   asio::error_code errorCode;
   mSocket.lowest_layer().open(address.is_v6() ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.lowest_layer().set_option(asio::ip::tcp::socket::reuse_address(true));
      mSocket.lowest_layer().set_option(asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.lowest_layer().bind(asio::ip::tcp::endpoint(mLocalBinding.getAddress(), mLocalBinding.getPort()), errorCode);
   }
}

asio::error_code 
TurnTlsSocket::connect(const std::string& address, unsigned short port)
{
   // Get a list of endpoints corresponding to the server name.
   asio::ip::tcp::resolver resolver(mIOService);
   resip::Data service(port);
#ifdef USE_IPV6
   asio::ip::tcp::resolver::query query(address, service.c_str());   
#else
   asio::ip::tcp::resolver::query query(asio::ip::tcp::v4(), address, service.c_str());   
#endif
   asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   asio::ip::tcp::resolver::iterator end;

   // Try each endpoint until we successfully establish a connection.
   asio::error_code errorCode = asio::error::host_not_found;
   while (errorCode && endpoint_iterator != end)
   {
      mSocket.lowest_layer().close();
      mSocket.lowest_layer().connect(*endpoint_iterator, errorCode);
      if(!errorCode)
      {
         DebugLog(<< "Connected!");
         mSocket.handshake(asio::ssl::stream_base::client, errorCode);
         if(!errorCode)
         {  
            DebugLog(<< "Handshake complete!");

            // Validate that hostname in cert matches connection hostname
            if(!mValidateServerCertificateHostname || validateServerCertificateHostname(address))
            {
               mConnected = true;
               mConnectedTuple.setTransportType(StunTuple::TLS);
               mConnectedTuple.setAddress(endpoint_iterator->endpoint().address());
               mConnectedTuple.setPort(endpoint_iterator->endpoint().port());
            }
            else
            {
               WarningLog(<< "Hostname in certificate does not match connection hostname!");
               mSocket.lowest_layer().close();
               errorCode = asio::error::operation_aborted;
            }
         }
      }
      endpoint_iterator++;
   }

   return errorCode;
}

bool 
TurnTlsSocket::validateServerCertificateHostname(const std::string& hostname)
{
   bool valid = false;

   // Validate that hostname in cert matches connection hostname

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
         if(resip::isEqualNoCase(dns, hostname.c_str()))
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
         if(resip::isEqualNoCase(name, hostname.c_str()))
         {
            valid = true;
         }
      }
   }
   
   X509_free(cert);
   return valid;
}

asio::error_code 
TurnTlsSocket::rawWrite(const char* buffer, unsigned int size)
{
   asio::error_code errorCode;
   asio::write(mSocket, asio::buffer(buffer, size), asio::transfer_all(), errorCode); 
   return errorCode;
}

asio::error_code 
TurnTlsSocket::rawWrite(const std::vector<asio::const_buffer>& buffers)
{
   asio::error_code errorCode;
   asio::write(mSocket, buffers, asio::transfer_all(), errorCode);
   return errorCode;
}

void 
TurnTlsSocket::readHeader()
{
   asio::async_read(mSocket, asio::buffer(mReadBuffer, 4),
                    boost::bind(&TurnTlsSocket::handleReadHeader, this, asio::placeholders::error));
}

void 
TurnTlsSocket::readBody(unsigned int len)
{
   asio::async_read(mSocket, asio::buffer(&mReadBuffer[4], len),
                    boost::bind(&TurnTlsSocket::handleRawRead, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
}

void
TurnTlsSocket::cancelSocket()
{
   asio::error_code ec;
   mSocket.lowest_layer().cancel(ec);
}

} // namespace
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
