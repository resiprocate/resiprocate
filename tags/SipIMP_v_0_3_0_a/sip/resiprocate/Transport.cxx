
#include "resiprocate/os/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#endif

#include <iostream>

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"

#include "resiprocate/Transport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TransportMessage.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

const Data Transport::transportNames[Transport::MAX_TRANSPORT] =
 {
    Data("Unknown"),
    Data("UDP"),
    Data("TCP"),
    Data("TLS"),
    Data("SCTP"),
    Data("DCCP")
 };

Transport::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}

Transport::Transport(const Data& sendhost, int portNum, const Data& nic, Fifo<Message>& rxFifo) :
   mHost(sendhost),
   mPort(portNum), 
   mInterface(nic),
   mStateMachineFifo(rxFifo),
   mShutdown(false)
{
   if (!mInterface.empty())
   {
#ifdef WIN32
	   assert(0); // !cj! TODO 
#else
      struct ifconf ifc;
   
      int s = socket( AF_INET, SOCK_DGRAM, 0 );
      int len = 100 * sizeof(struct ifreq);

      char buf[ len ];
   
      ifc.ifc_len = len;
      ifc.ifc_buf = buf;
   
      int e = ioctl(s,SIOCGIFCONF,&ifc);
      char *ptr = buf;
      int tl = ifc.ifc_len;
      int count=0;
  
      int maxRet = 10;
      while ( (tl > 0) && ( count < maxRet) )
      {
         struct ifreq* ifr = (struct ifreq *)ptr;
      
         int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
         tl -= si;
         ptr += si;
         //char* name = ifr->ifr_ifrn.ifrn_name;
         char* name = ifr->ifr_name;
 
         struct ifreq ifr2;
         ifr2 = *ifr;
      
         e = ioctl(s,SIOCGIFADDR,&ifr2);

         struct sockaddr a = ifr2.ifr_addr;
         struct sockaddr_in* addr = (struct sockaddr_in*) &a;
      
         char str[256];
         inet_ntop(AF_INET, (u_int32_t*)(&addr->sin_addr.s_addr), str, sizeof(str));
         DebugLog (<< "Considering: " << name << " -> " << str);

         if (nic == Data(name))
         {
            mIpAddress = str;
            InfoLog (<< "Using interface: " << mInterface << "->" << mIpAddress);
            break;
         }
      }
#endif
   }
}

Transport::~Transport()
{
}


void
Transport::run()
{
   while (!mShutdown)
   {
      FdSet fdset; 
      fdset.reset();
      fdset.setRead(mFd);
      fdset.setWrite(mFd);
      int  err = fdset.selectMilliSeconds(0);
      if (err == 0)
      {
         try
         {
            assert(0);
            //process();
         }
         catch (BaseException& e)
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


bool 
Transport::hasDataToSend() const
{
   return mTxFifo.messageAvailable();
}


void 
Transport::send( const Tuple& dest, const Data& d, const Data& tid)
{
   SendData* data = new SendData(dest, d, tid);
   assert(dest.port != -1);
   DebugLog (<< "Adding message to tx buffer to: " << dest); // << " " << d.escaped());
   mTxFifo.add(data); // !jf!
}

const Data&
Transport::toData(Transport::Type type)
{
   assert(type >= Transport::Unknown &&
          type < Transport::MAX_TRANSPORT);
   return Transport::transportNames[type];
}

Transport::Type
Transport::toTransport(const Data& type)
{
   for (Transport::Type i = Transport::Unknown; i < Transport::MAX_TRANSPORT; 
        i = static_cast<Transport::Type>(i + 1))
   {
      if (isEqualNoCase(type, Transport::transportNames[i]))
      {
         return i;
      }
   }
   assert(0);
   return Transport::Unknown;
};

void
Transport::stampReceived(SipMessage* message)
{
   //DebugLog (<< "adding new SipMessage to state machine's Fifo: " << message->brief());
   // set the received= and rport= parameters in the message if necessary !jf!
   if (message->isRequest() && !message->header(h_Vias).empty())
   {
      const Tuple& tuple = message->getSource();
      
#ifndef WIN32
      char received[255];
      inet_ntop(AF_INET, &tuple.ipv4.s_addr, received, sizeof(received));
      message->header(h_Vias).front().param(p_received) = received;
#else
      char * buf = inet_ntoa(tuple.ipv4); // !jf! not threadsafe
      message->header(h_Vias).front().param(p_received) = buf;
#endif

      if (message->header(h_Vias).front().exists(p_rport))
      {
         message->header(h_Vias).front().param(p_rport).port() = tuple.port;
      }
   }
}


Transport::Tuple::Tuple() : 
   port(0), 
   transportType(Unknown), 
   transport(0),
   connection(0)
{
   memset(&ipv4, 0, sizeof(ipv4));
}

Transport::Tuple::Tuple(in_addr pipv4,
                        int pport,
                        Transport::Type ptype)
   : ipv4(pipv4),
     port(pport),
     transportType(ptype),
     transport(0),
     connection(0)
{
}

bool Transport::Tuple::operator==(const Transport::Tuple& rhs) const
{
   return (memcmp(&ipv4, &rhs.ipv4, sizeof(ipv4)) == 0 &&
           port == rhs.port &&
           transportType == rhs.transportType);
   // !dlb! don't include connection 
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
   
   if (port > rhs.port)
   {
      return false;
   }

   return transportType < rhs.transportType;
}

std::ostream&
resip::operator<<(ostream& ostrm, const Transport::Tuple& tuple)
{
	ostrm << "[ " ;

#if defined(WIN32) 
//	ostrm   << inet_ntoa(tuple.ipv4);
	
#else	
	char str[128];
	ostrm << inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str));
#endif	
	
	ostrm  << " , " 
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
__gnu_cxx::hash<resip::Transport::Tuple>::operator()(const resip::Transport::Tuple& tuple) const
{
   // !dlb! do not include the connection
   Transport::Tuple& tup(const_cast<Transport::Tuple&>(tuple));
   Connection* conn = 0;
   std::swap(conn, tup.connection);
   // assumes POD
   unsigned long __h = 0; 
   const char* start = (const char*)&tuple;
   const char* end = start + sizeof(tuple);
   for ( ; start != end; ++start)
   {
      __h = 5*__h + *start; // .dlb. weird hash
   }

   std::swap(conn, tup.connection);
   return size_t(__h);

}

#elif  defined(__INTEL_COMPILER )
size_t 
std::hash_value(const resip::Transport::Tuple& tuple) 
{
   // !dlb! do not include the connection
   Transport::Tuple& tup(const_cast<Transport::Tuple&>(tuple));
   Connection* conn = 0;
   std::swap(conn, tup.connection);

   // assumes POD
   unsigned long __h = 0; 
   const char* start = (const char*)&tuple;
   const char* end = start + sizeof(tuple);
   for ( ; start != end; ++start)
   {
      __h = 5*__h + *start; // .dlb. weird hash
   }

   std::swap(conn, tup.connection);
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
