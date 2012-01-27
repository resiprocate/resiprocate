/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TLSTRANSPORT_HXX)
#define RESIP_TLSTRANSPORT_HXX

#include "resip/stack/TcpBaseTransport.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "resip/stack/Compression.hxx"

namespace resip
{

class Connection;
class Message;
class Security;

/**
   @ingroup transports
   @internal 
   
    Used in test cases, TransportSelector (which is itself internal)
    and in SipStack::addTransport. Not expected to be used in an API.

   Implements a Transport that uses TLS with TCP. 
*/
class TlsTransport : public TcpBaseTransport
{
   public:
      RESIP_HeapCount(TlsTransport);
      /**
         @param fifo the TransactionMessage Fifo that will receive any
         ConnectionTerminated or TransportFailure messages.
      
         @param interfaceObj a "presentation format" representation
         of the IP address of this transport
         @see Tuple::inet_ntop() for information about "presentation
         format"
      
         @param socketFunc Functor (defined in rutil/Socket.hxx) that is called 
               when a socket is created; allows app to log fd creation, tweak 
               sockopts, and so forth.

         @param portNum is the port to receive and/or send on
      
         @param sipDomain the domain name of the Transport.

         @see tlsDomain()

         @todo Note that because of InternalTransport's constructor,
         tlsDomain is always set to Data::Empty at construction time,
         in practice.
      
         @param socketFunc subclassers can call this function after
         the socket is created.  This is not currently used by
         Transport.
      */
      TlsTransport(Fifo<TransactionMessage>& fifo, 
                   int portNum, 
                   IpVersion version,
                   const Data& interfaceObj,
                   Security& security,
                   const Data& sipDomain, 
                   SecurityTypes::SSLType sslType,
                   AfterSocketCreationFuncPtr socketFunc=0,
                   Compression &compression = Compression::Disabled(), 
                   unsigned transportFlags = 0,
                   bool hasOwnThread=false);
      virtual  ~TlsTransport();

      TransportType transport() const { return TLS; }
   protected:

      /**
         A connection is created during process().  Incoming
         connections can be accept()ed while processing readable FDs
         (if a connection has been requested from a peer).  Outgoing
         connections are requested when attempting to send data while
         processing writable FDs and a usable connection hasn't
         already been established.
      */
      Connection* createConnection(Tuple& who, Socket fd, bool server=false);

      Security* mSecurity;
      SecurityTypes::SSLType mSslType;

   private:
      // "Interceptor" c'tor for folks who are migrating from a version that did
      // not have a transportFlags param, with extra arg to help explain 
      // what is going on.
      typedef enum
      {
         THANK_GOODNESS
      } ImplicitTypeconversionForArgDisabled;

      TlsTransport(Fifo<TransactionMessage>& fifo, 
                   int portNum, 
                   IpVersion version,
                   const Data& interfaceObj,
                   Security& security,
                   const Data& sipDomain, 
                   SecurityTypes::SSLType sslType,
                   AfterSocketCreationFuncPtr socketFunc,
                   Compression &compression, 
                   bool hasOwnThread,
                   ImplicitTypeconversionForArgDisabled=THANK_GOODNESS);

};

}

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
