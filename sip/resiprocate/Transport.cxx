#include <iostream>
#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "util/Logger.hxx"
#include "util/Socket.hxx"

#include "sipstack/Transport.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/TransportMessage.hxx"
#include "sipstack/DnsMessage.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP 

Transport::Exception::Exception(const Data& msg, const Data& file, const int line) :
   VException(msg,file,line)
{
}

Transport::Transport(const Data& sendhost, int portNum, const Data& nic, Fifo<Message>& rxFifo) :
   mHost(sendhost),
   mPort(portNum), 
   mInterface(nic),
   mStateMachineFifo(rxFifo),
   mShutdown(false)
{
}

Transport::~Transport()
{
}


void
Transport::run()
{
   while(!mShutdown)
   {
      FdSet fdset; 
      fdset.reset();
      fdset.setRead(mFd);
      fdset.setWrite(mFd);
      int  err = fdset.select(0);
      if (err == 0)
      {
         try
         {
            assert(0);
            //process();
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

void
Transport::fail(const Data& tid)
{
   mStateMachineFifo.add(new TransportMessage(tid, true));
}

void
Transport::ok(const Data& tid)
{
   mStateMachineFifo.add(new TransportMessage(tid, false));
}


void 
Transport::send( const Tuple& dest, const Data& d, const Data& tid)
{
   SendData* data = new  SendData(dest, d, tid);
   DebugLog (<< "Adding message to tx buffer: " << endl << d.c_str());
   mTxFifo.add(data); // !jf!
}


void 
Transport::buildFdSet( FdSet& fdset )
{
   fdset.setRead(mFd);
   fdset.setWrite(mFd);
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

   return Unknown;
};

Transport::Tuple::Tuple() : 
   port(0), 
   transportType(Unknown), 
   transport(0)
{
   memset(&ipv4, 0, sizeof(ipv4));
}

Transport::Tuple::Tuple(in_addr pipv4,
                        int pport,
                        Transport::Type ptype)
   : ipv4(pipv4),
     port(pport),
     transportType(ptype),
     transport(0)
{
}

bool Transport::Tuple::operator==(const Transport::Tuple& rhs) const
{
   return (memcmp(&ipv4, &rhs.ipv4, sizeof(ipv4)) == 0 &&
           port == rhs.port &&
           transport == rhs.transport && 
           transportType == rhs.transportType);
}

bool Transport::Tuple::operator<(const Transport::Tuple& rhs) const
{
   int c = memcmp(&ipv4, &rhs.ipv4, sizeof(ipv4));
   if (c < 0)
   {
      return true;
   }
   
   if (c > 0)
   {
      return false;
   }
   
   if (port < rhs.port)
   {
      return true;
   }
   
   if (port < rhs.port)
   {
      return false;
   }
   
   return transport < rhs.transport;
}

std::ostream&
Vocal2::operator<<(ostream& ostrm, const Transport::Tuple& tuple)
{
   char str[128];

   ostrm << "[ " 
#ifndef WIN32 //  !cj!
	   << inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str))
#endif	
         << " , " 
         << tuple.port
         << " , "
         << Transport::toData(tuple.transportType) 
         << " , "
         << tuple.transport 
         << " ]";

   return ostrm;
}


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )

size_t 
__gnu_cxx::hash<Vocal2::Transport::Tuple>::operator()(const Vocal2::Transport::Tuple& tuple) const
{
   // assumes POD
   unsigned long __h = 0; 
   const char* start = (const char*)&tuple;
   const char* end = start + sizeof(tuple);
   for ( ; start != end; ++start)
   {
      __h = 5*__h + *start; // .dlb. weird hash
   }
   return size_t(__h);

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
