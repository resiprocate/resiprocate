#if !defined(RESIP_TRANSPORT_HXX)
#define RESIP_TRANSPORT_HXX

#include <exception>

#include "resiprocate/Message.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/ThreadIf.hxx"

namespace resip
{

class SipMessage;
class SendData;
class Connection;

class Transport : public ThreadIf
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };
      
      // sendHost what to put in the Via:sent-by
      // portNum is the port to receive and/or send on
      Transport(Fifo<Message>& rxFifo, int portNum, const Data& interfaceObj, bool ipv4);
      virtual ~Transport();

      // shared by UDP, TCP, and TLS
      static Socket socket(TransportType type, bool ipv4);
      static void error();
      void bind();
      
      virtual void send( const Tuple& tuple, const Data& data, const Data& tid);
      virtual void process(FdSet& fdset) = 0;
      virtual void buildFdSet( FdSet& fdset) =0;
      
      void thread(); // from ThreadIf
      
      void fail(const Data& tid); // called when transport failed
      
      // These methods are used by the TransportSelector
      const Data& interfaceName() const { return mInterface; } 
      int port() const { return mPort; } 
      bool isV4() const { return mV4; }

      const Data& tlsDomain() const { return Data::Empty; }
      const sockaddr& boundInterface() const { return mBoundInterface; }

      virtual TransportType transport() const =0 ;
      virtual bool isReliable() const =0;
      
      static TransportType toTransport( const Data& );
      static const Data& toData( TransportType );

      // mark the received= and rport parameters if necessary
      static void stampReceived(SipMessage* request);

      bool hasDataToSend() const;

      // also used by the TransportSelector. 
      // requires that the two transports be 
      bool operator==(const Transport& rhs) const;
      
   protected:
      bool mV4;
      Socket mFd; // this is a unix file descriptor or a windows SOCKET
      int mPort;
      Data mInterface;

      sockaddr mBoundInterface;
      
      Fifo<SendData> mTxFifo; // owned by the transport
      Fifo<Message>& mStateMachineFifo; // passed in

   private:
      static const Data transportNames[MAX_TRANSPORT];
};

class SendData
{
   public:
      SendData(const Tuple& dest, const Data& pdata, const Data& tid): 
         destination(dest),
         data(pdata),
         transactionId(tid) 
      {}
      Tuple destination;
      const Data data;
      const Data transactionId;
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
