#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <memory>
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/SipMessage.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

const int UdpTransport::MaxBufferSize = 8192;

UdpTransport::UdpTransport(Fifo<Message>& fifo,
                           int portNum,
                           const Data& pinterface, 
                           bool ipv4) 
   : Transport(fifo, portNum, pinterface, ipv4)
{
   InfoLog (<< "Creating udp transport host=" << pinterface << " port=" << portNum);
   
   mFd = socket(ipv4 ? PF_INET : PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
   
   if ( mFd == INVALID_SOCKET )
   {
      InfoLog (<< "Failed to open socket: " << portNum);
	  throw Exception("Failed to open UDP port", __FILE__,__LINE__);
   }
   
   sockaddr_in addr4;
#ifdef USE_IPV6
   sockaddr_in6 addr6;
#endif
   sockaddr* saddr=0;
   int saddrLen = 0;
   
   if (ipv4)
   {
      memset(&addr4, 0, sizeof(addr4));
      
      addr4.sin_family = AF_INET;
      addr4.sin_port = htons(portNum);
      if (pinterface == Data::Empty)
      {
         addr4.sin_addr.s_addr = htonl(INADDR_ANY); 
      }
      else
      {
         DnsUtil::inet_pton(pinterface, addr4.sin_addr);
      }


      saddr = reinterpret_cast<sockaddr*>(&addr4);
      saddrLen = sizeof(addr4);
   }
   else
   {
#ifdef USE_IPV6
	   memset(&addr6, 0, sizeof(addr6));

      addr6.sin6_family = AF_INET6;
      addr6.sin6_port = htons(portNum);
      if (pinterface == Data::Empty)
      {
         addr6.sin6_addr = in6addr_any;
      }
      else
      {
         DnsUtil::inet_pton(pinterface, addr6.sin6_addr);
      }

      saddr = reinterpret_cast<sockaddr*>(&addr6);
      saddrLen = sizeof(addr6);
#else
	   assert(0);
#endif
   }
   assert(saddr);
   assert(saddrLen);
   
   
   if ( bind( mFd, saddr, saddrLen) == SOCKET_ERROR )
   {
      int err = errno;
      if ( err == EADDRINUSE )
      {
         ErrLog (<< "UDP port " << portNum << " already in use");
         throw Exception("UDP port already in use", __FILE__,__LINE__);
      }
      else
      {
         ErrLog (<< "Could not bind to port: " << portNum);
         throw Exception("Could not use UDP port", __FILE__,__LINE__);
      }
   }

   bool ok = makeSocketNonBlocking(mFd);
   if ( !ok )
   {
      ErrLog (<< "Could not make UDP port non blocking " << portNum);
      throw Exception("Could not use UDP port", __FILE__,__LINE__);
   }
}


UdpTransport::~UdpTransport()
{
   DebugLog (<< "Closing " << mFd);
   close(mFd);
}


void 
UdpTransport::process(FdSet& fdset)
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo


   // how do we know that buffer won't get deleted on us !jf!
   if (mTxFifo.messageAvailable())
   {
      std::auto_ptr<SendData> sendData = std::auto_ptr<SendData>(mTxFifo.getNext());
      //DebugLog (<< "Sent: " <<  sendData->data);
      //DebugLog (<< "Sending message on udp.");

      assert( &(*sendData) );
      assert( sendData->destination.port != 0 );
      
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
         InfoLog (<< strerror(err));
         InfoLog (<< "Failed sending to " << sendData->destination);
         fail(sendData->transactionId);
      }
      else
      {
         if (count != int(sendData->data.size()) )
         {
            ErrLog (<< "UDPTransport - send buffer full" );
            fail(sendData->transactionId);
         }
      }
   }
   
   struct sockaddr from;

   // !jf! this may have to change - when we read a message that is too big
   if ( !fdset.readyToRead(mFd) )
   {
      return;
   }

   //should this buffer be allocated on the stack and then copied out, as it
   //needs to be deleted every time EWOULDBLOCK is encountered
   char* buffer = new char[MaxBufferSize];
   socklen_t fromLen = sizeof(from);

   // !jf! how do we tell if it discarded bytes 
   // !ah! we use the len-1 trick :-(

   //DebugLog( << "starting recvfrom" );
   int len = recvfrom( mFd,
                       buffer,
                       MaxBufferSize,
                       0 /*flags */,
                       (struct sockaddr*)&from,
                       &fromLen);
   if ( len == SOCKET_ERROR )
   {
      int err = errno;
      switch (err)
      {
         case WSANOTINITIALISED:
            assert(0);
            break;

         case EWOULDBLOCK:
            DebugLog (<< " UdpTransport recvfrom got EWOULDBLOCK");
            break;

         case 0:
            DebugLog (<< " UdpTransport recvfrom got error 0 ");
            break;

         //case 9:
            DebugLog (<< " UdpTransport recvfrom got error 9 ");
            break;

         default:
            ErrLog(<<"Error receiving, errno="<<err << " " << strerror(err) );
            break;
      }
   }
   //DebugLog( << "completed recvfrom" );

   if (len == 0 || len == SOCKET_ERROR)
   {
      delete[] buffer; 
      buffer=0;
      return;
   }

   if (len == MaxBufferSize)
   {
      InfoLog(<<"Datagram exceeded max length "<<MaxBufferSize);
      delete [] buffer; buffer=0;
      return;
   }

   buffer[len]=0; // null terminate the buffer string just to make debug easier and reduce errors

   //DebugLog ( << "UDP Rcv : " << len << " b" );
   //DebugLog ( << Data(buffer, len).escaped().c_str());

   SipMessage* message = new SipMessage(this);

   // set the received from information into the received= parameter in the
   // via

   // It is presumed that UDP Datagrams are arriving atomically and that
   // each one is a unique SIP message


   // Save all the info where this message came from
   if (from.sa_family == AF_INET)
   {
      sockaddr_in* addr4 = reinterpret_cast<sockaddr_in*>(&from);
      Tuple tuple(addr4->sin_addr, ntohs(addr4->sin_port), transport());
      tuple.transport = this;      
      message->setSource(tuple);
   }
#ifdef USE_IPV6
   else if (from.sa_family == AF_INET6)
   {
      sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(&from);
      Tuple tuple(addr6->sin6_addr, ntohs(addr6->sin6_port), transport());
      tuple.transport = this;      
      message->setSource(tuple);
   }
#endif
   else
   {
      assert(0);
   }
   

   // Tell the SipMessage about this datagram buffer.
   message->addBuffer(buffer);

#ifndef NEW_MSG_HEADER_SCANNER // {

   // This is UDP, so, initialise the preparser with this
   // buffer.

   int err = mPreparse.process(*message,buffer, len);
	
   if (err)
   {
      InfoLog(<<"Preparse Rejecting datagram as unparsable / fragmented.");
      DebugLog(<< Data(buffer, len));
      delete message; 
      message=0; 
      // Reset the preparse machine if we're not saving the fragment
      mPreparse.reset();
      return;
   }

   if ( !mPreparse.isHeadersComplete() )
   {
      InfoLog(<<"Rejecting datagram as unparsable / fragmented.");
      DebugLog(<< Data(buffer, len));
      delete message; 
      message=0;
      // Reset the preparse machine if we're not saving the fragment
      mPreparse.reset();
      return;
   }

   // no pp error
   int used = mPreparse.nBytesUsed();

#else //defined (NEW_MSG_HEADER_SCANNER) } {

   mMsgHeaderScanner.prepareForMessage(message);

   char *unprocessedCharPtr;
   if (mMsgHeaderScanner.scanChunk(buffer,
                                   len,
                                   &unprocessedCharPtr) !=
                                                      MsgHeaderScanner::scrEnd)
   {
      InfoLog(<<"Preparse Rejecting datagram as unparsable / fragmented.");
      DebugLog(<< Data(buffer, len));
      delete message; 
      message=0; 
      return;
   }

   // no pp error
   int used = unprocessedCharPtr - buffer;

#endif //defined (NEW_MSG_HEADER_SCANNER) }

   if (used < len)
   {
      // body is present .. add it up.
      // NB. The Sip Message uses an overlay (again)
      // for the body. It ALSO expects that the body
      // will be contiguous (of course).
      // it doesn't need a new buffer in UDP b/c there
      // will only be one datagram per buffer. (1:1 strict)

      message->setBody(buffer+used,len-used);
      //DebugLog(<<"added " << len-used << " byte body");
   }

   stampReceived(message);

   mStateMachineFifo.add(message);
}


void 
UdpTransport::buildFdSet( FdSet& fdset )
{
   fdset.setRead(mFd);
    
   //if (mTxFifo.messageAvailable())
   //{
   //  fdset.setWrite(mFd);
   //}
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
