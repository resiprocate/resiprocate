
#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <memory>
#include "util/Data.hxx"
#include "util/Socket.hxx"
#include "util/Logger.hxx"
#include "sipstack/UdpTransport.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Preparse.hxx"


#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace std;
using namespace Vocal2;

const int UdpTransport::MaxBufferSize = 8192;

UdpTransport::UdpTransport(const Data& sendhost, int portNum, const Data& nic, Fifo<Message>& fifo) : 
   Transport(sendhost, portNum, nic , fifo)
{
   mFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if ( mFd == INVALID_SOCKET )
   {
      InfoLog (<< "Failed to open socket: " << portNum);
   }
   
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY); // !jf! 
   addr.sin_port = htons(portNum);
   
   if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR )
   {
      int err = errno;
      if ( err == EADDRINUSE )
      {
         InfoLog (<< "Address already in use");
      }
      else
      {
         InfoLog (<< "Could not bind to port: " << portNum);
      }
      
      throw Exception("Address already in use", __FILE__,__LINE__);
   }

   // make non blocking 
#if WIN32
   unsigned long block = 0;
   int errNoBlock = ioctlsocket( mFd, FIONBIO , &block );
   assert( errNoBlock == 0 );
#else
   int flags  = fcntl( mFd, F_GETFL, 0);
   int errNoBlock = fcntl(mFd,F_SETFL, flags| O_NONBLOCK );
   assert( errNoBlock == 0 );
#endif

}


UdpTransport::~UdpTransport()
{
}


void 
UdpTransport::process(fd_set* fdSet)
{
	
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   
   // how do we know that buffer won't get deleted on us !jf!
   if (mTxFifo.messageAvailable())
   {
      std::auto_ptr<SendData> sendData = std::auto_ptr<SendData>(mTxFifo.getNext());
      DebugLog (<< "Sending message on udp");

      sockaddr_in addrin;
      addrin.sin_addr = sendData->destination.ipv4;
      addrin.sin_port = htons(sendData->destination.port);
      addrin.sin_family = AF_INET;
      
      int count = sendto(mFd, 
                         sendData->data.data(), sendData->data.size(),  
                         0, // flags
                         (const sockaddr*)&addrin, sizeof(sockaddr_in) );
   
      if ( count == SOCKET_ERROR )
      {
         int err = errno;
         DebugLog (<< strerror(err));
         fail(sendData->transactionId);
         
         // !jf! what to do if it fails
         assert(0);
      }
      else
      {
         assert ( (count == int(sendData->data.size()) ) || (count == SOCKET_ERROR ) );
         ok(sendData->transactionId);
      }
   }

   struct sockaddr_in from;

   // !ah! debug is just to always return a sample message
   // !jf! this may have to change - when we read a message that is too big
   
   if ( !FD_ISSET(mFd, fdSet ) )
   {
	   return;
   }

   char* buffer = new char[MaxBufferSize];
   int fromLen = sizeof(from);
   
   // !jf! how do we tell if it discarded bytes 
   // !ah! we use the len-1 trick :-(

#if (! defined (__QNX__) )
   int len = recvfrom( mFd,
                       buffer,
                       MaxBufferSize,
                       0 /*flags */,
                       (struct sockaddr*)&from,
                       (socklen_t*)&fromLen);
#else
   int len = recvfrom( mFd,
                       buffer,
                       MaxBufferSize,
                       0 /*flags */,
                       (struct sockaddr*)&from,
                       (size_t*)&fromLen);
#endif


   if ( len == SOCKET_ERROR )
   {
	   int err = errno;
	   //cerr << "Err=" << err << " " << strerror(err) << endl;
	   
	   switch (err)
	   {
		   case EWOULDBLOCK:
		   {
		   }
		   break;
		   default:
		   {
			   ErrLog(<<"Error receiving, errno="<<err);
		   }
		   break;
	   }
	   
   }
   else if (len > 0)
   {
      
      if (len == MaxBufferSize)
      {
         InfoLog(<<"Datagram exceeded max length "<<MaxBufferSize);
         InfoLog(<<"Possibly truncated");
      }
      DebugLog ( << "UDP Rcv : " << len << " b" );
      DebugLog ( << Data(buffer, len).c_str());
      
      SipMessage* message = new SipMessage(true);
      
      // set the received from information into the received= parameter in the
      // via

      // It is presumed that UDP Datagrams are arriving atomically and that
      // each one is a unique SIP message


      // Save all the info where this message came from
      Tuple tuple;
      tuple.ipv4 = from.sin_addr;
      tuple.port = ntohs(from.sin_port);
      tuple.transport = this;
      tuple.transportType = transport();
      message->setSource(tuple);

      // Tell the SipMessage about this datagram buffer.
      message->addBuffer(buffer);

      // This is UDP, so, initialise the preparser with this
      // buffer.


      bool error = false;
      bool complete = false;
      
      int used = 0;

      PreparseState::TransportAction status = PreparseState::NONE;
      
      mPreparse.process(*message,buffer, len, used, status);
      
      // this won't work if UDPs are fragd !ah!

      if (status == PreparseState::preparseError || 
          status == PreparseState::fragment )
      {
         InfoLog(<<"Rejecting datagram as unparsable / fragmented.");
         delete message;

      }
      else
      {
          // no pp error
          if (status == PreparseState::headersComplete &&
              used < len)
          {
              // body is present .. add it up.
              // NB. The Sip Message uses an overlay (again)
              // for the body. It ALSO expects that the body
              // will be contiguous (of course).
              // it doesn't need a new buffer in UDP b/c there
              // will only be one datagram per buffer. (1:1 strict)

              message->setBody(buffer+used,len-used);
              DebugLog(<<"added " << len-used << " byte body");
          }
          
         DebugLog (<< "adding new SipMessage to state machine's Fifo: " << message->brief());
         // set the received= and rport= parameters in the message if necessary !jf!
         mStateMachineFifo.add(message);
      }
   }
}


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
