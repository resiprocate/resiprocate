#if !defined(TRANSPORT_HXX)
#define TRANSPORT_HXX

#include <exception>

#ifndef WIN32
#include <sys/select.h>
#include <netinet/in.h>
#endif

#include <util/Data.hxx>
#include <util/Fifo.hxx>
#include <util/Socket.hxx>
#include <util/VException.hxx>
#include <sipstack/Message.hxx>

namespace Vocal2
{

class SipMessage;

class SendData
      {
	  public:
            SendData(const sockaddr_in& dest)
               : destination(dest) 
               {}
            
            const sockaddr_in& destination;
            //TransactionId tid;
            const char* buffer;
            size_t length;
      };

class Transport
{
   public:
      class Exception : public VException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };
      
      Transport(int portNum, Fifo<Message>& rxFifo);
      // !ah! need to think about type for
      // interface specification here.
      
      virtual ~Transport();
      
      virtual void send( const sockaddr_in& address, const  char* buffer, size_t length)=0; //, TransactionId txId) = 0;

      virtual void process(fd_set* fdSet=NULL) = 0 ;
	
	virtual void buildFdSet( fd_set* fdSet, int* fdSetSize );

      void run();               // will not return.
      
      void shutdown();

   protected:
	Socket mFd; // this is a unix file descriptor or a windows SOCKET
      int mPort;
      Fifo<SendData> mTxFifo; // owned by the transport
      Fifo<Message>& mStateMachineFifo; // passed in

   private:

      bool mShutdown ;
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
