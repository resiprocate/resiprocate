#include <util/Logger.hxx>
#include <util/Socket.hxx>

#include <sipstack/Transport.hxx>
#include <sipstack/SipMessage.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP 

Transport::Exception::Exception(const Data& msg, const Data& file, const int line) :
   VException(msg,file,line)
{
}

Transport::Transport(const Data& sendhost, int portNum, const Data& interface, Fifo<Message>& rxFifo) :
   mHost(sendhost),
   mPort(portNum), 
   mInterface(interface),
   mStateMachineFifo(rxFifo),
   mShutdown(false)
{
}

void
Transport::run()
{
   while(!mShutdown)
   {
      fd_set fdSet; 
      int fdSetSize;

      FD_ZERO(&fdSet); 
      fdSetSize=0;
      FD_SET(mFd,&fdSet); 

#ifdef WIN32
      assert(0);
#else
      if ( mFd+1 > fdSetSize )
      {	
         fdSetSize =  mFd+1; 
      }
#endif

      int  err = select(fdSetSize, &fdSet, 0, 0, 0);
      if (err == 0)
      {
         try
         {
            process();
         }
         catch (VException& e)
         {
            InfoLog (<< "Uncaught exception: " << e);
         }
      }
      else
      {
         assert(0);
      }
   }
}

void
Transport::shutdown()
{
   mShutdown = true;
}

Transport::~Transport()
{
}


void 
Transport::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
   assert( fdSet );
   assert( fdSetSize );
	
   FD_SET(mFd,fdSet);
   if ( mFd+1 > (*fdSetSize) )
   {
      *fdSetSize = mFd+1;
   }
}

Data
Transport::toData(Transport::Type type)
{
   switch (type)
   {
      case UDP:
         return "UDP";
      case TCP:
         return "TCP";
      case TLS:
         return "TLS";
      case SCTP:
         return"SCTP";
      case DCCP:
         return "DCCP";
      case Unknown:
      default:
         assert(0);
         return "Unknown";
   }
}

Transport::Type
Transport::toTransport(const Data& type)
{
   if (type == "UDP")
   {
      return UDP;
   }
   else if (type == "TCP")
   {
      return TCP;
   }
   else if (type == "TLS")
   {
      return TLS;
   }
   else if (type == "SCTP")
   {
      return SCTP;
   }
   else if (type == "DCCP")
   {
      return DCCP;
   }
   else if (type == "Unknown")
   {
      return Unknown;
   }
   else
   {
      assert(0);
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
