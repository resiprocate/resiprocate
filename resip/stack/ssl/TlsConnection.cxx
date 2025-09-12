#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(USE_SSL)

#include "resip/stack/ssl/TlsConnection.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Socket.hxx"

#include <openssl/opensslv.h>
#if !defined(LIBRESSL_VERSION_NUMBER)
#include <openssl/e_os2.h>
#endif
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TlsConnection::TlsConnection(Transport* transport, const Tuple& tuple,
   Socket fd, Security* security,
   bool server, Data domain, SecurityTypes::SSLType sslType,
   Compression& compression) :
   Connection(transport, tuple, fd, compression, server),
   mServer(server),
   mSecurity(security),
   mSslType(sslType),
   mDomain(domain)
{
#if defined(USE_SSL)
   InfoLog(<< "Creating TLS connection for domain " << mDomain << " " << tuple << " on " << fd);

   mSsl = NULL;
   mBio = NULL;

   if (mServer)
   {
      DebugLog(<< "Trying to form TLS connection - acting as server");
      if (mDomain.empty())
      {
         ErrLog(<< "Tranport was not created with a server domain so can not act as server");
         throw Security::Exception("Trying to act as server but no domain specified",
            __FILE__, __LINE__);
      }
   }
   else
   {
      DebugLog(<< "Trying to form TLS connection - acting as client");
   }
   resip_assert(mSecurity);

   TlsBaseTransport* t = dynamic_cast<TlsBaseTransport*>(transport);
   resip_assert(t);

   SSL_CTX* ctx = t->getCtx();
   resip_assert(ctx);

   mSsl = SSL_new(ctx);
   resip_assert(mSsl);

   resip_assert(mSecurity);

   if (mServer)
   {
      // clear SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE set in SSL_CTX if we are a server
      int verify_mode = SSL_VERIFY_PEER;
      switch (t->getClientVerificationMode())
      {
         case SecurityTypes::None:
            verify_mode = SSL_VERIFY_NONE;
            DebugLog(<< "Not expecting client certificate");
            break;
         case SecurityTypes::Optional:
            verify_mode = SSL_VERIFY_PEER;
            DebugLog(<< "Optional client certificate mode");
            break;
         case SecurityTypes::Mandatory:
            verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
            DebugLog(<< "Mandatory client certificate mode");
            break;
         default:
            resip_assert(0);
      }
      SSL_set_verify(mSsl, verify_mode, 0);  // Modify verify flags, but leave callback same as set in Security.cxx
   }

   mBio = BIO_new_socket((int)fd, 0/*close flag*/);
   if (!mBio)
   {
      throw Transport::Exception("Failed to create OpenSSL BIO for socket",
         __FILE__, __LINE__);
   }

   SSL_set_bio(mSsl, mBio, mBio);
   // Set application/ex data on context so that this Connection can be queried in certificate validation callback
   SSL_set_ex_data(mSsl, BaseSecurity::resip_connection_ssl_ex_data_idx, this);

   mTlsState = Initial;
   mHandShakeWantsRead = false;

#endif // USE_SSL
}

TlsConnection::~TlsConnection()
{
#if defined(USE_SSL)
   ERR_clear_error();
   int ret = SSL_shutdown(mSsl);
   if (ret < 0)
   {
      int err = SSL_get_error(mSsl, ret);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
            // WANT_READ or WANT_WRITE can arise for bi-directional shutdown on
            // non-blocking sockets, safe to ignore
            StackLog(<< "Got TLS shutdown error condition of " << err);
            break;

         default:
            handleOpenSSLErrorQueue(ret, err, "SSL_shutdown");
      }
   }
   SSL_free(mSsl);
#endif // USE_SSL
}

const char*
TlsConnection::fromState(TlsConnection::TlsState s)
{
   switch(s)
   {
      case Initial: return "Initial"; break;
      case Handshaking: return "Handshaking"; break;
      case Broken: return "Broken"; break;
      case Up: return "Up"; break;
   }
   resip_assert(false);
   return "????";
}

TlsConnection::TlsState
TlsConnection::checkState()
{
#if defined(USE_SSL)
   //DebugLog(<<"state is " << fromTlsState(mTlsState));

   if (mTlsState == Up || mTlsState == Broken)
   {
      return mTlsState;
   }

   int handshakeRet = 0;

   ERR_clear_error();

   if (mTlsState != Handshaking)
   {
      if (mServer)
      {
         InfoLog(<< "TLS handshake starting (Server mode)");
         SSL_set_accept_state(mSsl);
         mTlsState = Handshaking;
      }
      else
      {
         InfoLog(<< "TLS handshake starting (client mode)");
         /* OpenSSL version < 0.9.8f does not support SSL_set_tlsext_host_name() */
#if defined(SSL_set_tlsext_host_name)
         DebugLog(<< "TLS SNI extension in Client Hello: " << who().getTargetDomain());
         SSL_set_tlsext_host_name(mSsl, who().getTargetDomain().c_str()); // set the SNI hostname
#endif
         SSL_set_connect_state(mSsl);
         mTlsState = Handshaking;
      }

      InfoLog(<< "TLS connected");
      mTlsState = Handshaking;
   }

   mHandShakeWantsRead = false;
   handshakeRet = SSL_do_handshake(mSsl);

   if (handshakeRet <= 0)
   {
      int err = SSL_get_error(mSsl, handshakeRet);

      switch (err)
      {
         case SSL_ERROR_WANT_READ:
            StackLog(<< "TLS handshake want read");
            mHandShakeWantsRead = true;
            return mTlsState;

         case SSL_ERROR_WANT_WRITE:
            StackLog(<< "TLS handshake want write");
            ensureWritable();
            return mTlsState;

         case SSL_ERROR_ZERO_RETURN:
            StackLog(<< "TLS connection closed cleanly");
            return mTlsState;

         case SSL_ERROR_WANT_CONNECT:
            StackLog(<< "BIO not connected, try later");
            return mTlsState;

         case SSL_ERROR_WANT_ACCEPT:
            StackLog(<< "TLS connection want accept");
            return mTlsState;

         case SSL_ERROR_WANT_X509_LOOKUP:
            DebugLog(<< "Try later / SSL_ERROR_WANT_X509_LOOKUP");
            return mTlsState;

         default:
            TransportFailure::FailureReason failureReason = TransportFailure::ConnectionException;
            int failureSubCode = 0;
            Data failureString;
            DataStream ds(failureString);
            bool addToAdditionalFailureStrings = true;
            ds << "TLS handshake failed: ";
            if (err == SSL_ERROR_SYSCALL)
            {
               int e = getErrno();
               switch (e)
               {
                  case EINTR:
                  case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                  case EWOULDBLOCK:  // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
#endif
                     StackLog(<< "try later: " << e);
                     return mTlsState;
               }
               ds << "socket error=" << e << ": " << Transport::errorToString(e);
               failureSubCode = e;
               if (e == 0)
               {
                  TlsBaseTransport* t = dynamic_cast<TlsBaseTransport*>(transport());
                  resip_assert(t);
                  if (mServer && t->getClientVerificationMode() != SecurityTypes::None)
                  {
                     ds << ", client may have disconnected to prompt for user certificate, because it can't supply a certificate (verification mode == " << (t->getClientVerificationMode() == SecurityTypes::Mandatory ? "Mandatory" : "Optional") << " for this transport) or because it does not support using client certificates";
                  }
               }
            }
            else if (err == SSL_ERROR_SSL)
            {
               ds << "SSL cipher or certificate failure(SSL_ERROR_SSL): ";
               int verifyErrorCode = SSL_get_verify_result(mSsl);
               switch (verifyErrorCode)
               {
                  case X509_V_OK:
                  {
                     // OpenSSL docs:  If no peer certificate was presented, the returned result code is X509_V_OK. 
                     // This is because no verification error occurred, it does however not indicate success.
                     X509* peerCert = SSL_get_peer_certificate(mSsl);
                     if (peerCert)
                     {
                        ds << "peer supplied a certificate, it has either not been checked or it was checked successfully";
                        X509_free(peerCert);
                     }
                     else
                     {
                        ds << "no peer supplied certificate";
                     }
                     break;
                  }

                  default:
                     ds << "peer certificate validation failure: " << X509_verify_cert_error_string(verifyErrorCode);
                     // We will have failure strings already added from the certificate validation callback already (Security.cxx), 
                     // adding errors from the SSL error queue doesn't appear to provide any additional information - disable it
                     addToAdditionalFailureStrings = false;
                     break;
               }
               if (mServer)
               {
                  TlsBaseTransport* t = dynamic_cast<TlsBaseTransport*>(transport());
                  resip_assert(t);
                  if (t->getClientVerificationMode() == SecurityTypes::Mandatory)
                  {
                     ds << "; mandatory client certificate verification required";
                  }
               }
               failureReason = TransportFailure::CertValidationFailure;
               failureSubCode = verifyErrorCode;
            }
            else
            {
               ds << "unhandled SSL_get_error result: " << err;
               failureSubCode = err;
            }
            ds.flush();
            ErrLog(<< failureString);
            handleOpenSSLErrorQueue(handshakeRet, err, "SSL_do_handshake", addToAdditionalFailureStrings);
            setFailureReason(failureReason, failureSubCode, failureString);
            mBio = NULL;
            mTlsState = Broken;
            return mTlsState;
      }
   }
   else // handshakeRet > 1
   {
      InfoLog(<< "TLS connected");
   }

   // force peer name to get checked and perhaps cert loaded
   computePeerName();

   //post-connection verification: check that certificate name matches domain name
   if (!mServer)
   {
      bool matches = false;
      for (std::list<BaseSecurity::PeerName>::iterator it = mPeerNames.begin(); it != mPeerNames.end(); it++)
      {
         if (BaseSecurity::matchHostName(it->mName, who().getTargetDomain()))
         {
            matches = true;
            break;
         }
      }
      if (!matches)
      {
         mTlsState = Broken;
         mBio = NULL;
         Data failureString;
         {
            DataStream ds(failureString);
            ds << "Certificate name mismatch: trying to connect to <"
               << who().getTargetDomain()
               << "> remote cert domain(s) are <"
               << getPeerNamesData() << ">";
         }
         ErrLog(<< failureString);
         setFailureReason(TransportFailure::CertNameMismatch, 0, failureString);
         return mTlsState;
      }
   }

   InfoLog(<< "TLS handshake done for peer " << getPeerNamesData());
   mTlsState = Up;
   if (!mOutstandingSends.empty())
   {
      ensureWritable();
   }
#endif // USE_SSL
   return mTlsState;
}

bool
TlsConnection::handleOpenSSLErrorQueue(int ret, unsigned long err, const char* op, bool addToAdditionalFailureStrings)
{
   bool hadReason = false;
   ErrLog(<< op << " error=" << err << ", ret=" << ret);
   while (true)
   {
      const char* file;
      int line;

#if OPENSSL_VERSION_NUMBER < 0x30000000L
      unsigned long code = ERR_get_error_line(&file, &line);
#else
      unsigned long code = ERR_get_error_all(&file, &line, NULL, NULL, NULL);
#endif
      if (code == 0)
      {
         break;
      }

      char buf[256];
      ERR_error_string_n(code, buf, sizeof(buf));
      Data errorString;
      {
         DataStream ds(errorString);
         ds << "  " << buf << " (file=" << file << ", line=" << line << ")";
      }
      ErrLog(<< errorString);
      if (addToAdditionalFailureStrings)
      {
         addAdditionalFailureString(errorString);
      }
      hadReason = true;
   }
   return hadReason;
}

int
TlsConnection::read(char* buf, int count)
{
#if defined(USE_SSL)
   resip_assert(mSsl);
   resip_assert(buf);

   switch (checkState())
   {
      case Broken:
         return -1;
         break;
      case Up:
         break;
      default:
         return 0;
         break;
   }

   if (!mBio)
   {
      DebugLog(<< "Got TLS read bad bio  ");
      return 0;
   }

   if (!isGood())
   {
      return -1;
   }

   int bytesRead = SSL_read(mSsl, buf, count);
   //StackLog(<< "SSL_read returned " << bytesRead << " bytes [" << Data(Data::Borrow, buf, (bytesRead > 0)?(bytesRead):(0)) << "]");

   if (bytesRead > 0)
   {
      int bytesPending = SSL_pending(mSsl);
      if (bytesPending > 0)
      {
         char* buffer = getWriteBufferForExtraBytes(bytesRead, bytesPending);
         if (buffer)
         {
            //StackLog(<< "reading remaining buffered bytes");
            bytesPending = SSL_read(mSsl, buffer, bytesPending);
            //StackLog(<< "SSL_read returned  " << bytesPending << " bytes [" << Data(Data::Borrow, buffer, (bytesPending > 0)?(bytesPending):(0)) << "]");

            if (bytesPending > 0)
            {
               bytesRead += bytesPending;
            }
            else
            {
               // It's not clear why SSL_read would return an error after SSL_pending returned > 0, however we have
               // seen a case where it has returned 0 with an error of SSL_ERROR_WANT_READ.  So we check the error
               // code returned, and if it is a retyable error, then we proceed with the bytes we already read in the 
               // in the initial SSL_read call.
               // Note:  SSL_read docs say the following: "Old documentation indicated a difference between 0 and -1 (return code), 
               //        and that -1 was retryable. You should instead call SSL_get_error() to find out if it's retryable."
               int err = SSL_get_error(mSsl, bytesPending);
               if (err != SSL_ERROR_WANT_READ &&
                  err != SSL_ERROR_WANT_WRITE &&
                  err != SSL_ERROR_NONE)
               {
                  // Not a retryable error, put the error return code into bytesRead to
                  // be used in the conditional block later in this method.
                  bytesRead = bytesPending;
               }
            }
         }
         else
         {
            resip_assert(0);
         }
      }
      else if (bytesPending < 0)
      {
         int err = SSL_get_error(mSsl, bytesPending);
         handleOpenSSLErrorQueue(bytesPending, err, "SSL_pending");
         return -1;
      }
   }

   if (bytesRead <= 0)
   {
      int err = SSL_get_error(mSsl, bytesRead);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            StackLog(<< "Got TLS read got condition of " << err);
            return 0;
         }
         break;
         case SSL_ERROR_ZERO_RETURN:
         {
            DebugLog(<< "Got SSL_ERROR_ZERO_RETURN (TLS shutdown by peer)");
            return -1;
         }
         break;
         default:
         {
            handleOpenSSLErrorQueue(bytesRead, err, "SSL_read");
            if (err == 5)
            {
               WarningLog(<< "err=5 sometimes indicates that intermediate certificates may be missing from local PEM file");
            }
            return -1;
         }
         break;
      }
      resip_assert(0);
   }
   StackLog(<< "SSL bytesRead=" << bytesRead);
   return bytesRead;
#endif // USE_SSL
   return -1;
}

bool
TlsConnection::transportWrite()
{
   switch(mTlsState)
   {
      case Handshaking:
      case Initial:
         checkState();
         if (mTlsState == Handshaking)
         {
            DebugLog(<< "transportWrite in Handshaking state, remove from write: " << (mHandShakeWantsRead ? "true" : "false"));
            return mHandShakeWantsRead;
         }
         else
         {
            DebugLog(<< "transportWrite in Initial state (Handshake complete), calling write");
         }
      case Up:
      case Broken:
      default:
         DebugLog(<< "transportWrite in " << fromState(mTlsState) << " state, calling write");
   }
   return false;
}

int
TlsConnection::write(const char* buf, int count)
{
#if defined(USE_SSL)
   resip_assert(mSsl);
   resip_assert(buf);
   int ret;

   switch (checkState())
   {
      case Broken:
         return -1;
         break;
      case Up:
         break;
      default:
         DebugLog(<< "Tried to Tls write, but connection state is not Up");
         return 0;
         break;
   }

   if (!mBio)
   {
      DebugLog(<< "Got TLS write bad bio");
      return 0;
   }

   ret = SSL_write(mSsl, (const char*)buf, count);
   if (ret < 0)
   {
      int err = SSL_get_error(mSsl, ret);
      switch (err)
      {
         case SSL_ERROR_WANT_READ:
         case SSL_ERROR_WANT_WRITE:
         case SSL_ERROR_NONE:
         {
            StackLog(<< "Got TLS write got condition of " << err);
            return 0;
         }
         break;
         case SSL_ERROR_ZERO_RETURN:
         {
            DebugLog(<< "Got SSL_ERROR_ZERO_RETURN (TLS shutdown by peer)");
            return -1;
         }
         break;
         default:
         {
            handleOpenSSLErrorQueue(ret, err, "SSL_write");
            return -1;
         }
         break;
      }
   }

   Data monkey(Data::Borrow, buf, count);
   StackLog(<< "Did TLS write " << ret << " " << count << " " << "[[" << monkey << "]]");

   return ret;
#endif // USE_SSL
   return -1;
}

bool
TlsConnection::hasDataToRead() // has data that can be read 
{
#if defined(USE_SSL)
   //hack (for now)
   if (mTlsState == Initial)
      return false;

   if (checkState() != Up)
   {
      return false;
   }

   int p = SSL_pending(mSsl);
   //DebugLog(<<"hasDataToRead(): " <<p);
   return (p > 0);
#else // USE_SSL
   return false;
#endif 
}

bool
TlsConnection::isGood() // has data that can be read 
{
#if defined(USE_SSL)
   if (mBio == 0)
   {
      return false;
   }

   int mode = SSL_get_shutdown(mSsl);
   if (mode < 0)
   {
      int err = SSL_get_error(mSsl, mode);
      handleOpenSSLErrorQueue(mode, err, "SSL_get_shutdown");
      return false;
   }

   if (mode != 0)
   {
      return false;
   }

#endif
   return true;
}

bool
TlsConnection::isWritable()
{
#if defined(USE_SSL)
   switch (mTlsState)
   {
      case Handshaking:
         return mHandShakeWantsRead ? false : true;
      case Initial:
      case Up:
         return isGood();
      default:
         return false;
   }
   //dragos -- to remove
   DebugLog(<< "Current state: " << fromState(mTlsState));
   //end dragos
#endif 
   return false;
}

void
TlsConnection::getPeerNames(std::list<Data>& peerNames) const
{
   for (std::list<BaseSecurity::PeerName>::const_iterator it = mPeerNames.begin(); it != mPeerNames.end(); it++)
   {
      peerNames.push_back(it->mName);
   }
}

Data
TlsConnection::getPeerNamesData() const
{
   Data peerNamesString;
   for (std::list<BaseSecurity::PeerName>::const_iterator it = mPeerNames.begin(); it != mPeerNames.end(); it++)
   {
      if (it == mPeerNames.begin())
      {
         peerNamesString += it->mName;
      }
      else
      {
         peerNamesString += ", " + it->mName;
      }
   }
   return peerNamesString;
}

void
TlsConnection::computePeerName()
{
#if defined(USE_SSL)
   Data commonName;

   resip_assert(mSsl);

   if (!mBio)
   {
      ErrLog(<< "bad bio");
      return;
   }

   // print session info
   const SSL_CIPHER* ciph;
   ciph = SSL_get_current_cipher(mSsl);
   InfoLog(<< "TLS session set up with "
           << SSL_get_version(mSsl) << " "
           << SSL_CIPHER_get_version(ciph) << " "
           << SSL_CIPHER_get_name(ciph) << " ");

   // get the certificate if other side has one 
   X509* cert = SSL_get_peer_certificate(mSsl);
   if (!cert)
   {
      DebugLog(<< "No peer certificate in TLS connection");
      return;
   }

   // check that this certificate is valid 
   if (X509_V_OK != SSL_get_verify_result(mSsl))
   {
      DebugLog(<< "Peer certificate in TLS connection is not valid");
      X509_free(cert); cert = NULL;
      return;
   }

   TlsBaseTransport* t = dynamic_cast<TlsBaseTransport*>(mTransport);
   resip_assert(t);

   mPeerNames.clear();
   BaseSecurity::getCertNames(cert, mPeerNames, t->isUseEmailAsSIP());
   if (mPeerNames.empty())
   {
      ErrLog(<< "Invalid certificate: no subjectAltName/CommonName found");
      return;
   }

   if (!mServer)
   {
      // add the certificate to the Security store
      unsigned char* buf = NULL;
      int len = i2d_X509(cert, &buf);
      Data derCert(buf, len);
      for (std::list<BaseSecurity::PeerName>::iterator it = mPeerNames.begin(); it != mPeerNames.end(); it++)
      {
         if (!mSecurity->hasDomainCert(it->mName, false /* logErrors? */))
         {
            mSecurity->addDomainCertDER(it->mName, derCert);
         }
      }
      OPENSSL_free(buf); buf = NULL;
   }

   X509_free(cert); cert = NULL;
#endif // USE_SSL
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
