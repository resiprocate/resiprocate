#include <sipstack/Resolver.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/TransportSelector.hxx>
#include <sipstack/UdpTransport.hxx>
#include <sipstack/Uri.hxx>
#include <sipstack/SendingMessage.hxx>

#include <util/DataStream.hxx>
#include <util/Logger.hxx>
#include <util/Socket.hxx>


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP


TransportSelector::TransportSelector(SipStack& stack) :
   mStack(stack)
{
}

TransportSelector::~TransportSelector()
{
   while (!mTransports.empty())
   {
      Transport* t = mTransports[0];
      mTransports.erase(mTransports.begin());
      delete t;
   }
}


void 
TransportSelector::addTransport( Transport::Type protocol, 
                                 int port,
                                 const Data& hostName,
                                 const Data& nic) 
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
         InfoLog( << "could not find local hostname:" << strerror(err) );
         throw Transport::Exception("could not find local hostname",__FILE__,__LINE__);
      }
      hostname = Data(buf);
   }
   assert( !hostname.empty() );
   
   Transport* transport=0;
   switch ( protocol )
   {
      case Transport::UDP:
         transport = new UdpTransport(hostname, port, nic, mStack.mStateMacFifo);
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
   for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
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
   // pick the target destination 
   //   - for request route then request URI -  unless if firs entry in route is
   //     strict router in which case use the URI
   //   - for response look at via  

   Resolver::Tuple tuple = msg->resolve();
   Transport* transport = *mTransports.begin();

   // insert the via
   if (msg->isRequest())
   {
      assert(!msg->header(h_Vias).empty());
      msg->header(h_Vias).front().remove(p_maddr);
      //msg->header(h_Vias).front().param(p_ttl) = 1;
      msg->header(h_Vias).front().transport() = Transport::toData(transport->transport());  //cache !jf! 
      msg->header(h_Vias).front().sentHost() = transport->hostname();
      msg->header(h_Vias).front().sentPort() = transport->port();
   }

   Data& encoded = msg->getEncoded();
   DataStream encodeStream(encoded);
   msg->encode(encodeStream);
   encodeStream.flush();

   DebugLog (<< "encoded=" << encoded.c_str());
   
   
   // send it over the transport
   transport->send(tuple.ipv4, encoded);
   mStack.mStateMacFifo.add(new SendingMessage(msg->getTransactionId(), transport->isReliable()));
}

void
TransportSelector::retransmit(SipMessage* msg)
{
   Transport* transport = *mTransports.begin();
   assert(transport);
   transport->send(msg->tuple().ipv4, msg->getEncoded());
}


void 
TransportSelector::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
   for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
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
