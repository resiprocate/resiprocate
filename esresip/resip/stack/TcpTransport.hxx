/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TCPTRANSPORT_HXX)
#define RESIP_TCPTRANSPORT_HXX

#include "resip/stack/TcpBaseTransport.hxx"
#include "resip/stack/Compression.hxx"

namespace resip
{

class TransactionMessage;

/**
   @ingroup transports
   @internal 
   
    Used in test cases, TransportSelector (which is itself internal)
    and in SipStack::addTransport. 
    Not expected to be used in an API.
*/
class TcpTransport : public TcpBaseTransport
{
   public:

      /** 
	     @brief Specifies a TCP transport.
		 
         @param fifo the TransactionMessage Fifo that will receive any
         ConnectionTerminated or TransportFailure messages.
		 
		 @param portNum is the port to receive and/or send on.
		 
		 @param version the version of the IP address.
      
         @param interfaceObj a "presentation format" representation
         of the IP address of this transport.
       @param socketFunc Functor (defined in rutil/Socket.hxx) that is called 
               when a socket is created; allows app to log fd creation, tweak 
               sockopts, and so forth.
		 @param compression enables/disables compression.
      
		 @see Tuple::inet_ntop() for information about "presentation
         format".
      */
      TcpTransport(Fifo<TransactionMessage>& fifo, 
                   int portNum, 
                   IpVersion version,
                   const Data& interfaceObj,
                   AfterSocketCreationFuncPtr socketFunc=0,
                   Compression &compression = Compression::Disabled(), 
                   unsigned transportFlags = 0,
                   bool hasOwnThread=false,
                   bool doSctp=false);
      virtual  ~TcpTransport();
      
      TransportType transport() const { return (mDoSctp ? SCTP : TCP); }

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

   private:
      // "Interceptor" c'tor for folks who are migrating from a version that did
      // not have a transportFlags param, with extra arg to help explain 
      // what is going on.
      typedef enum
      {
         THANK_GOODNESS
      } ImplicitTypeconversionForArgDisabled;

      TcpTransport(Fifo<TransactionMessage>& fifo, 
                   int portNum, 
                   IpVersion version,
                   const Data& interfaceObj,
                   AfterSocketCreationFuncPtr socketFunc,
                   Compression &compression,
                   bool hasOwnThread,
                   bool doSctp=false,
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
