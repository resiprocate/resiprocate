#if !defined(RESIP_TLSBASETRANSPORT_HXX)
#define RESIP_TLSBASETRANSPORT_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif


#include "resip/stack/TcpBaseTransport.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "resip/stack/Compression.hxx"

#include <openssl/ssl.h>

namespace resip
{

class Connection;
class Message;
class Security;

class TlsBaseTransport : public TcpBaseTransport
{
   public:
      RESIP_HeapCount(TlsBaseTransport);
      TlsBaseTransport(Fifo<TransactionMessage>& fifo, 
                   int portNum, 
                   IpVersion version,
                   const Data& interfaceObj,
                   Security& security,
                   const Data& sipDomain, 
                   SecurityTypes::SSLType sslType,
                   TransportType transportType,
                   AfterSocketCreationFuncPtr socketFunc=0,
                   Compression &compression = Compression::Disabled,
                   unsigned transportFlags = 0,
                   SecurityTypes::TlsClientVerificationMode cvm = SecurityTypes::None,
                   bool useEmailAsSIP = false,
                   const Data& certificateFilename = "", 
                   const Data& privateKeyFilename = "",
                   const Data& privateKeyPassPhrase = "");
      virtual  ~TlsBaseTransport();

      SSL_CTX* getCtx() const;

      SecurityTypes::TlsClientVerificationMode getClientVerificationMode() 
         { return mClientVerificationMode; };
      bool isUseEmailAsSIP()
         { return mUseEmailAsSIP; };

      /** @brief Set a custom callback function to be used by the SSL stack
          to inspect the peer certificate.

          callback semantics (the arguments and return values) are very
          specific to the SSL stack in use.  If the vendor parameter does
          not match the SSL stack then this method returns false and does not
          use the callback.

          This method should be called before the stack starts accepting
          connections, otherwise, any connection received before setting the
          callback would be validated using the default validation function
          provided by the SSL stack.

          @param vendor the SSL stack vendor,
                        for example, SecurityTypes::OpenSSL
          @param func a pointer to the callback function, 0 to disable callback
          @param arg an argument to be passed to the callback if the
                     vendor API supports this
          @return true if successful, false on failure
      */
      bool setPeerCertificateVerificationCallback(
         SecurityTypes::SSLVendor vendor,
         void *func,
         void *arg);

   protected:
      Connection* createConnection(const Tuple& who, Socket fd, bool server=false);

      Security* mSecurity;
      SecurityTypes::SSLType mSslType;
      SSL_CTX* mDomainCtx;
      SecurityTypes::TlsClientVerificationMode mClientVerificationMode;
      /* If true, we will accept the email address in a client's subjectAltName
         as if it were a SIP URI.  This is convenient because many commercial
         CAs offer email certificates but not sip: certificates */
      bool mUseEmailAsSIP;
};

}

#endif

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
