#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/TcpTransport.hxx"
#include "resiprocate/TlsTransport.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/ReliabilityMessage.hxx"
#include "resiprocate/ParserCategories.hxx"

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/DnsUtil.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION


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
      hostname = DnsUtil::getLocalHostName();
   }
   
   Transport* transport=0;
   switch ( protocol )
   {
      case Transport::UDP:
         transport = new UdpTransport(hostname, port, nic, mStack.mStateMacFifo);
         break;
      case Transport::TCP:
         transport = new TcpTransport(hostname, port, nic, mStack.mStateMacFifo);
         break;
      default:
         assert(0);
         break;
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
   if (port == 0)
   {
      list<DnsUtil::Srv> records = DnsUtil::lookupSRVRecords(domainName);
      for (list<DnsUtil::Srv>::iterator i=records.begin(); i!=records.end(); i++)
      {
         if (i->transport == DnsUtil::TLS)
         {
            TlsTransport* transport = new TlsTransport(domainName, 
                                                       hostname, i->port, 
                                                       keyDir, privateKeyPassPhrase,
                                                       nic, mStack.mStateMacFifo);
            mTlsTransports[domainName] = transport;
            break;
         }
      }
   }
   else
   {
      TlsTransport* transport = new TlsTransport(domainName, 
                                                 hostname, port, 
                                                 keyDir, privateKeyPassPhrase,
                                                 nic, mStack.mStateMacFifo);      
      mTlsTransports[domainName] = transport;
   }
   
#else
   CritLog (<< "TLS not supported in this stack. Maybe you don't have openssl");
   assert(0);
#endif
}

void 
TransportSelector::process(FdSet& fdset)
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


bool 
TransportSelector::hasDataToSend() const
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

   return false;
}


void
TransportSelector::dnsResolve( SipMessage* msg, const Data& tid)
{
   // Picking the target destination:
   //   - for request, use forced target if set
   //     otherwise use loose routing behaviour (route or, if none, request-uri)
   //   - for response, use forced target if set, otherwise look at via  

   if (msg->isRequest())
   {
      // If this is an ACK we need to fix the tid to reflect that
      if (msg->hasTarget())
      {
         mStack.mDnsResolver.lookup(tid, msg->getTarget());
      }
      else if (msg->exists(h_Routes) && !msg->header(h_Routes).empty())
      {
         // put this into the target, in case the send later fails, so we don't
         // lose the target
         msg->setTarget(msg->header(h_Routes).front().uri());
         //msg->header(h_Routes).pop_front();
         mStack.mDnsResolver.lookup(tid, msg->getTarget());
      }
      else
      {
         mStack.mDnsResolver.lookup(tid, msg->header(h_RequestLine).uri());
      }
   }
   else if (msg->isResponse())
   {
      assert (!msg->header(h_Vias).empty());
      if (msg->hasTarget())
      {
         mStack.mDnsResolver.lookup(tid, msg->getTarget());
      }
      else
      {
         mStack.mDnsResolver.lookup(tid, msg->header(h_Vias).front());
      }
   }
   else
   {
      assert(0);
   }
}

// !jf! there may be an extra copy of a tuple here. can probably get rid of it
// but there are some const issues.  
void 
TransportSelector::send( SipMessage* msg, Transport::Tuple destination, const Data& tid, bool isResend )
{
   assert( &destination != 0 );

   if (destination.transport == 0)
   {
      if (destination.transportType == Transport::TLS)
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
         msg->header(h_Vias).front().transport() = Transport::toData(destination.transport->transport());  //cache !jf! 

         if (msg->header(h_Vias).front().sentHost().empty())
         {
            msg->header(h_Vias).front().sentHost() = destination.transport->hostName(); // use hostname 
         }

         const Via &v(msg->header(h_Vias).front());

         if (!DnsResolver::isIpAddress(v.sentHost()) &&  destination.transport->port() == 5060)
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

      //DebugLog (<< "encoded=" << std::endl << encoded.escaped().c_str() << "EOM");
   
      // send it over the transport
      destination.transport->send(destination, encoded, tid);
      if (! isResend)
      {
          mStack.mStateMacFifo.add(new ReliabilityMessage(tid, destination.transport->isReliable()));
      }
   }
   else
   {
      mStack.mStateMacFifo.add(new TransportMessage(tid, true));
   }
}

void
TransportSelector::retransmit(SipMessage* msg, Transport::Tuple& destination)
{
   assert(destination.transport);
   destination.transport->send(destination, msg->getEncoded(), msg->getTransactionId());
}


void 
TransportSelector::buildFdSet( FdSet& fdset )
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

Transport*
TransportSelector::findTransport(const Transport::Tuple& tuple) const
{
    return findTransport(tuple.transportType);
}

Transport*
TransportSelector::findTransport(const Transport::Type type) const
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
