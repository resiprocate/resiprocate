#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/ReliabilityMessage.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TcpTransport.hxx"
#include "resiprocate/TlsTransport.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/Uri.hxx"

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION


TransportSelector::TransportSelector(bool multithreaded, Fifo<Message>& fifo) :
   mMultiThreaded(multithreaded),
   mStateMacFifo(fifo)
{
   mSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
TransportSelector::addTransport( TransportType protocol, 
                                 int port,
                                 const Data& hostName,
                                 const Data& nic) 
{
   assert( port >  0 );

   Data hostname = hostName;
   if ( hostname.empty() )
   {
	   hostname = DnsUtil::getLocalHostName();
   }
   
   Transport* transport=0;
   switch ( protocol )
   {
      case UDP:
         transport = new UdpTransport(mStateMacFifo, port, hostname, true);
         break;
      case TCP:
         transport = new TcpTransport(mStateMacFifo, port, hostname, true);
         break;
      default:
         assert(0);
         break;
   }

   if (mMultiThreaded)
   {
      transport->run();
   }
   
   mTransports.push_back(transport);
}

void 
TransportSelector::addTlsTransport(const Data& domainName, 
                                   const Data& keyDir, const Data& privateKeyPassPhrase,
                                   int port,
                                   const Data& hostName,
                                   const Data& nic) 
{
#if defined( USE_SSL )
   assert( port >  0 );
   assert(mTlsTransports.count(domainName) == 0);

   Data hostname = hostName;
   if ( hostname.empty() )
   {
	   hostname = DnsUtil::getLocalHostName();
   }
   assert (port != 0);
   // if port == 0, do an SRV lookup and use the ports from there
   TlsTransport* transport = new TlsTransport(mStateMacFifo, 
                                              domainName, 
                                              hostname, port, 
                                              keyDir, privateKeyPassPhrase,
                                              true); // ipv4
   if (mMultiThreaded)
   {
      transport->run();
   }

   mTlsTransports[domainName] = transport;
   
#else
   CritLog (<< "TLS not supported in this stack. Maybe you don't have openssl");
   assert(0);
#endif
}

void 
TransportSelector::process(FdSet& fdset)
{
   mDns.process(fdset);
   
   if (!mMultiThreaded)
   {
      for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
      {
         try
         {
            (*i)->process(fdset);
         }
         catch (BaseException& e)
         {
            InfoLog (<< "Uncaught exception: " << e);
         }
      }

      for (HashMap<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
           i != mTlsTransports.end(); i++)
      {
         try
         {
            (i->second)->process(fdset);
         }
         catch (BaseException& e)
         {
            InfoLog (<< "Uncaught exception: " << e);
         }
      }
   }
}


bool 
TransportSelector::hasDataToSend() const
{   
   if (!mMultiThreaded)
   {
      for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
      {
         if (  (*i)->hasDataToSend() )
         {
            return true;
         }
      }
      for (HashMap<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
           i != mTlsTransports.end(); i++)
      {
         if ( (i->second)->hasDataToSend() )
         {
            return true;
         }
      }
   }
   
   return false;
}


DnsResult*
TransportSelector::dnsResolve( SipMessage* msg, DnsHandler* handler)
{
   // Picking the target destination:
   //   - for request, use forced target if set
   //     otherwise use loose routing behaviour (route or, if none, request-uri)
   //   - for response, use forced target if set, otherwise look at via  

   //!jf! the problem here is that DnsResult is returned after looking
   //mDns.lookup() but this can result in a synchronous call to handle() which
   //assumes that dnsresult has been assigned to the TransactionState
   DnsResult* result=0;
   if (msg->isRequest())
   {
      // If this is an ACK we need to fix the tid to reflect that
      if (msg->hasTarget())
      {
         result = mDns.lookup(msg->getTarget(), handler);
      }
      else if (msg->exists(h_Routes) && !msg->header(h_Routes).empty())
      {
         // put this into the target, in case the send later fails, so we don't
         // lose the target
         msg->setTarget(msg->header(h_Routes).front().uri());
         result = mDns.lookup(msg->getTarget(), handler);
      }
      else
      {
         result = mDns.lookup(msg->header(h_RequestLine).uri(), handler);
      }
   }
   else if (msg->isResponse())
   {
      assert(0);
   }
   else
   {
      assert(0);
   }

   assert(result);
   return result;
}

// !jf! there may be an extra copy of a tuple here. can probably get rid of it
// but there are some const issues.  
void 
TransportSelector::transmit( SipMessage* msg, Tuple& destination)
{
   assert( &destination != 0 );

   if (destination.transport == 0)
   {
      if (destination.transportType == TLS)
      {
         destination.transport = findTlsTransport(msg->getTlsDomain());
      }
      else
      {
         destination.transport = findTransport(destination);
      }
   }

   // !jf!
   // This can be problematic if the far side closed the TcpTransport after the
   // SipMessage was handed to the TU. The TU can then process the request and
   // send a response to the transaction. In the meantime, the Transport has
   // been deleted. So when the response gets here it references a transport
   // that has already been deleted. 
   // To solve the problem, a transport handle should be used instead of a
   // pointer. A smart pointer would also work. 
   if (destination.transport)
   {
      // insert the via
      if (msg->isRequest())
      {
         assert(!msg->header(h_Vias).empty());
         msg->header(h_Vias).front().remove(p_maddr);
         //msg->header(h_Vias).front().param(p_ttl) = 1;
         msg->header(h_Vias).front().transport() = Tuple::toData(destination.transport->transport());  //cache !jf! 

//         if (msg->header(h_Vias).front().sentHost().empty())
//         {
            // wing in the transport address based on where this is going.
            Data interfaceHost;

            if (!destination.v6)
            {
               
               sockaddr_in servaddr,cliaddr;
               memset(&servaddr,0,sizeof(servaddr));
               servaddr.sin_addr = destination.ipv4;
               servaddr.sin_port = htons(destination.port);
               servaddr.sin_family = AF_INET;
               
               connect(mSocket, 
                       (const sockaddr *)&servaddr,
                       sizeof(servaddr));
               socklen_t len = sizeof(cliaddr);
               getsockname(mSocket,(sockaddr*)&cliaddr, &len);
               
                     
               interfaceHost = DnsUtil::inet_ntop(cliaddr.sin_addr);

            }
            else
            {
               ErrLog(<<"IPv6 Route Table search unimplemented.");
            }

            msg->header(h_Vias).front().sentHost() = 
               interfaceHost;
            
            DebugLog(<<"Route table selection chooses: " << interfaceHost);
//         }

         const Via &v(msg->header(h_Vias).front());

         if (!DnsUtil::isIpAddress(v.sentHost()) &&  destination.transport->port() == 5060)
         {
            DebugLog(<<"supressing port 5060 w/ symname");
            // backward compat for 2543 and the symbolic host w/ 5060 ;
            // being a clue for SRV (see RFC 3263 sec 4.2 par 5).
         }
         else
         {
            msg->header(h_Vias).front().sentPort() = destination.transport->port();
         }
      }

      Data& encoded = msg->getEncoded();
      encoded.clear();
      DataStream encodeStream(encoded);
      msg->encode(encodeStream);
      encodeStream.flush();

      assert(!msg->getEncoded().empty());
      //DebugLog (<< "encoded=" << std::endl << encoded.escaped().c_str() << "EOM");
   
      // send it over the transport
      destination.transport->send(destination, encoded, msg->getTransactionId());
   }
   else
   {
      InfoLog (<< "Failed to find a transport for " << msg->getTransactionId() << " to " << destination);
      mStateMacFifo.add(new TransportMessage(msg->getTransactionId(), true));
   }
}

void
TransportSelector::retransmit(SipMessage* msg, Tuple& destination)
{
   assert(destination.transport);
   assert(!msg->getEncoded().empty());
   destination.transport->send(destination, msg->getEncoded(), msg->getTransactionId());
}


void 
TransportSelector::buildFdSet( FdSet& fdset )
{
   mDns.buildFdSet(fdset);
   
   if (!mMultiThreaded)
   {
      for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
      {
         (*i)->buildFdSet( fdset );
      }

   
      for (HashMap<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
           i != mTlsTransports.end(); i++)
      {
         (i->second)->buildFdSet( fdset );
      }
   }
}

Transport*
TransportSelector::findTransport(const Tuple& tuple) const
{
    return findTransport(tuple.transportType);
}

Transport*
TransportSelector::findTransport(const TransportType type) const
{
   // !jf! not done yet
   for (std::vector<Transport*>::const_iterator i=mTransports.begin(); i != mTransports.end(); i++)
   {
      //ErrLog( << "have transport type" <<  (*i)->transport() );
      if ( (*i)->transport() == type )
      {
         return *i;
      }
   }
   ErrLog( << "Couldn't find a transport for " << " type=" <<  type );
   return 0;
}

Transport*
TransportSelector::findTlsTransport(const Data& domainname) 
{
   DebugLog (<< "Searching for TLS transport for domain='" << domainname << "'");
   // If no domainname specified and there is only 1 TLS transport, use it. 
   if (domainname == Data::Empty && mTlsTransports.size() == 1)
   {
      DebugLog (<< "Found default TLS transport for domain=" << mTlsTransports.begin()->first);
      return mTlsTransports.begin()->second;
   }
   else if (mTlsTransports.count(domainname))
   {
      DebugLog (<< "Found TLS transport for domain=" << mTlsTransports.begin()->first);
      return mTlsTransports[domainname];
   }
   else  // don't know which one to use
   {
      DebugLog (<< "No TLS transport found");
      return 0;
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
