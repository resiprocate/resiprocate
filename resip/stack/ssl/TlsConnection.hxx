#if !defined(TlsConnection_hxx)
#define TlsConnection_hxx

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif


#include "resip/stack/Connection.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/ssl/Security.hxx"

// If USE_SSL is not defined, this will not be built, and this header will 
// not be installed. If you are including this file from a source tree, and are 
// getting link errors, the source tree was probably built without USE_SSL.
//#ifdef USE_SSL
//#include <openssl/ssl.h>
//#else
//typedef void BIO;
//typedef void SSL;
//#endif

#include <openssl/ssl.h>

namespace resip
{

class Tuple;
class Security;

class TlsConnection : public Connection
{
   public:
      RESIP_HeapCount(TlsConnection);

      TlsConnection( Transport* transport, const Tuple& who, Socket fd, 
                     Security* security, bool server, Data domain, 
                     SecurityTypes::SSLType sslType ,
                     Compression &compression);
      
      virtual ~TlsConnection();

      int read( char* buf, const int count );
      int write( const char* buf, const int count );
      virtual bool hasDataToRead(); // has data that can be read 
      virtual bool isGood(); // has valid connection
      virtual bool isWritable();
      
      virtual bool transportWrite();
      
      void getPeerNames(std::list<Data> & peerNames) const;
      
      typedef enum TlsState { Initial, Broken, Handshaking, Up } TlsState;
      static const char * fromState(TlsState);
   
   private:
      /// No default c'tor
      TlsConnection();
      void computePeerName();
      Data getPeerNamesData() const;
      TlsState checkState();

      bool mServer;
      Security* mSecurity;
      SecurityTypes::SSLType mSslType;
      Data mDomain;
      
      TlsState mTlsState;
      bool mHandShakeWantsRead;

      SSL* mSsl;
      BIO* mBio;
      std::list<BaseSecurity::PeerName> mPeerNames;
};
 
}

#endif

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
