#include <sstream>
#include <sipstack/TransportSelector.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/UdpTransport.hxx>
#include <sipstack/SipStack.hxx>
#include <util/Logger.hxx>

// Hack, hack, hack
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


TransportSelector::TransportSelector(SipStack& stack) :
   mStack(stack)
{
   addTransport(Transport::UDP, 5060);
}

TransportSelector::~TransportSelector()
{
   std::vector<Transport*>::iterator i;
   while (!mTransports.empty())
   {
      delete *i;
      mTransports.erase(i);
   }
}


void 
TransportSelector::addTransport( Transport::TransportType protocol, 
                                 int port,
                                 const Data& hostName,
                                 const Data& interface) 
{
   assert( port >  0 );

   Data hostname = hostName;
   if ( hostname.empty() )
   {
      char buf[1024];
      int e = gethostname(buf,sizeof(buf));
      if ( e != 0 )
      {
         int err = errno;
         InfoLog( << "cont not find local hostname:" << strerror(err) );
         throw Transport::Exception("could not find local hostname",__FILE__,__LINE__);
      }
      hostname = Data(buf);
   }
   assert( !hostname.empty() );
   
   Transport* transport=0;
   switch ( protocol )
   {
      case Transport::UDP:
         transport = new UdpTransport(hostname, port, interface, mStack.mStateMacFifo);
         break;
      default:
         assert(0);
         break;
   }

   mTransports.push_back(transport);
}


void 
TransportSelector::process(fd_set* fdSet)
{
   for (std::vector<Transport*>::const_iterator i; i != mTransports.end(); i++)
   {
      try
      {
         (*i)->process(fdSet);
      }
      catch (VException& e)
      {
         InfoLog (<< "Uncaught exception: " << e);
      }
   }
}

void 
TransportSelector::send( SipMessage* msg )
{
   // do loose routing, pick the target destination
   // do a dns lookup
   // insert the via
   // encode the message
   // send it to the appropriate transport

   Transport* transport = *mTransports.begin();
   std::cerr << "Sending message to the wire." << std::endl;
   msg->encode(std::cerr);
   std::cerr.flush();

   // Hmmmm... we shouldn't add a Via if this is a response.
   assert(!msg->header(h_Vias).empty());
   msg->header(h_Vias).front().param(p_maddr) = "";
   //msg->header(h_Vias).front().param(p_ttl) = 1;
   msg->header(h_Vias).front().transport() = Transport::toData(transport->transport());  //cache !jf! 
   msg->header(h_Vias).front().sentHost() = transport->hostname();
   msg->header(h_Vias).front().sentPort() = transport->port();
   
   std::stringstream strm;
   msg->encode(strm);
   
   sockaddr_in dest;
   dest.sin_family = PF_INET;
   dest.sin_addr.s_addr = inet_addr("127.0.0.1");
   dest.sin_port = htons(9999);
   // Figure out what the heck goes in dest here.
   transport->send(dest, strm.str().c_str(), strm.str().length());
   
   //mUdp->send(msg->str(), msg->size());
   //assert(0);
}


void 
TransportSelector::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
   for (std::vector<Transport*>::const_iterator i; i != mTransports.end(); i++)
   {
      (*i)->buildFdSet( fdSet, fdSetSize );
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
