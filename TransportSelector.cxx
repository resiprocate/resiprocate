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
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


TransportSelector::TransportSelector(bool multithreaded, Fifo<Message>& fifo) :
   mMultiThreaded(multithreaded),
   mStateMacFifo(fifo),
   mSocket( INVALID_SOCKET )
{
 
}

TransportSelector::~TransportSelector()
{
   while (!mTransports.empty())
   {
      Transport* t = mTransports.begin()->second;
      mTransports.erase(mTransports.begin()->first);
      delete t;
   }
}


// !jf! Note that it uses ipv6 here but ipv4 in the Transport classes (ugggh!)
void 
TransportSelector::addTransport( TransportType protocol, 
                                 int port,
                                 bool ipv6,
                                 const Data& ipInterface)
{
   assert( port >  0 );

   Transport* transport=0;
   switch ( protocol )
   {
      case UDP:
         transport = new UdpTransport(mStateMacFifo, port, ipInterface, !ipv6);
         break;
      case TCP:
         transport = new TcpTransport(mStateMacFifo, port, ipInterface, !ipv6);
         break;
      default:
         assert(0);
         break;
   }

   if (mMultiThreaded)
   {
      transport->run();
   }
   
   Tuple key(ipInterface, port, !ipv6, protocol);
   assert(mTransports.count(key) == 0);
   DebugLog (<< "Adding transport: " << key);
   mTransports[key] = transport;

   if (mDefaultTransports.count(protocol) == 0)
   {
      mDefaultTransports[protocol] = transport;
   }
}

void 
TransportSelector::addTlsTransport(const Data& domainName, 
                                   const Data& keyDir, const Data& privateKeyPassPhrase,
                                   int port,
                                   bool ipv6,
                                   const Data& ipInterface)
{
#if defined( USE_SSL )
   assert( port >  0 );
   assert(mTlsTransports.count(domainName) == 0);

   assert (port != 0);
   // if port == 0, do an SRV lookup and use the ports from there
   TlsTransport* transport = new TlsTransport(mStateMacFifo, 
                                              domainName, 
                                              ipInterface, port, 
                                              keyDir, privateKeyPassPhrase,
                                              !ipv6); 
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
      for (std::map<Tuple,Transport*>::const_iterator i=mTransports.begin();
           i != mTransports.end(); i++)
      {
         try
         {
            i->second->process(fdset);
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
      for (std::map<Tuple,Transport*>::const_iterator i=mTransports.begin();
           i != mTransports.end(); i++)
      {
         if (  i->second->hasDataToSend() )
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

void
TransportSelector::srcAddrForDest(const Tuple& dest, Tuple& source) const
{
   if (mSocket == INVALID_SOCKET)
   {
      mSocket = Transport::socket(UDP, dest.isV4()); // may throw
   }
   
   int ret = connect(mSocket,&dest.getSockaddr(), dest.length());
   if (ret < 0)
   {
      int err = getErrno();
      ErrLog(<< "Unable to route to " << DnsUtil::inet_ntop(dest) << strerror(err));
      throw Transport::Exception("Can't find source address for Via", __FILE__,__LINE__);
   }
   
   socklen_t len = source.length();  
   ret = getsockname(mSocket,&source.getMutableSockaddr(), &len);
   if (ret < 0)
   {
      int err = getErrno();
      ErrLog(<< "Can't determine name of socket " << DnsUtil::inet_ntop(dest) << strerror(err));
      throw Transport::Exception("Can't find source address for Via", __FILE__,__LINE__);
   }
}

// !jf! there may be an extra copy of a tuple here. can probably get rid of it
// but there are some const issues.  
void 
TransportSelector::transmit( SipMessage* msg, Tuple& destination)
{
   assert( &destination != 0 );
   DebugLog (<< "Transmitting " << *msg << " to " << destination);
   try
   {
      Tuple source(destination);
      srcAddrForDest(destination,source);
      if (destination.transport == 0)
      {
         if (destination.getType() == TLS)
         {
            destination.transport = findTlsTransport(msg->getTlsDomain());
         }
         else
         {
            // there must be a via, use the port in the via as a hint of what
            // port to send on
            source.setPort(msg->header(h_Vias).front().sentPort());
            destination.transport = findTransport(source);
         }
      }
      
      if (destination.transport)
      {
         // insert the via
         if (msg->isRequest())
         {
            assert(!msg->header(h_Vias).empty());
            msg->header(h_Vias).front().remove(p_maddr); // !jf! why do this? 
            msg->header(h_Vias).front().transport() = Tuple::toData(destination.transport->transport());  //cache !jf! 
            msg->header(h_Vias).front().sentHost() = DnsUtil::inet_ntop(source);
            msg->header(h_Vias).front().sentPort() = destination.transport->port();
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
   catch (Transport::Exception& e)
   {
      InfoLog (<< "No route to destination " << msg->getTransactionId() << " to " << destination);
      mStateMacFifo.add(new TransportMessage(msg->getTransactionId(), true));
      return;
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
      for (std::map<Tuple,Transport*>::const_iterator i=mTransports.begin();
           i != mTransports.end(); i++)
      {
         i->second->buildFdSet( fdset );
      }
   
      for (HashMap<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
           i != mTlsTransports.end(); i++)
      {
         (i->second)->buildFdSet( fdset );
      }
   }
}

Transport*
TransportSelector::findTransport(const Tuple& search) 
{
   // first search for a s specific transport, then look for transport with any interface
   std::map<Tuple, Transport*>::iterator i = mTransports.find(search);
   if (i != mTransports.end())
   {
      return i->second;
   }
   else
   {
      Tuple tuple(search);
      tuple.setAny();
      
      std::map<Tuple, Transport*>::iterator i = mTransports.find(tuple);
      if (i != mTransports.end())
      {
         return i->second;
      }
      else 
      {
         // now just find a matching transport type
         HashMap<int, Transport*>::iterator i = mDefaultTransports.find(int(tuple.getType()));
         if (i != mDefaultTransports.end())
         {
            return i->second;
         }
      }
   }

   ErrLog(<< "Can't find matching transport " << DnsUtil::inet_ntop(search));
   throw Transport::Exception("No matching transport found",__FILE__,__LINE__);
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
