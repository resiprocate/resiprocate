#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "resiprocate/NameAddr.hxx"
#include "resiprocate/Uri.hxx"

#include "resiprocate/Security.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TcpTransport.hxx"
#include "resiprocate/TlsTransport.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/DtlsTransport.hxx"
#include "resiprocate/Uri.hxx"

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/WinCompat.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

#include <sys/types.h>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TransportSelector::TransportSelector(bool multithreaded, Fifo<TransactionMessage>& fifo, Security* security) :
   mMultiThreaded(multithreaded),
   mStateMacFifo(fifo),
   mSecurity(security),
   mSocket( INVALID_SOCKET ),
   mSocket6( INVALID_SOCKET ),
   mWindowsVersion(WinCompat::getVersion())
{
   memset(&mUnspecified, 0, sizeof(sockaddr_in));
   mUnspecified.sin_family = AF_UNSPEC;

#ifdef USE_IPV6
   memset(&mUnspecified6, 0, sizeof(sockaddr_in6));
   mUnspecified6.sin6_family = AF_UNSPEC;
#endif
}

template<class T> void 
deleteMap(T& m)
{
   for (typename T::iterator it = m.begin(); it != m.end(); it++)
   {
      delete it->second;
   }
   m.clear();
}

TransportSelector::~TransportSelector()
{
   deleteMap(mExactTransports);
   deleteMap(mAnyInterfaceTransports);
#if defined( USE_SSL )  
   deleteMap(mTlsTransports);
#endif
#if defined( USE_DTLS )
   deleteMap(mDtlsTransports);
#endif
}

void
TransportSelector::shutdown()
{
   for (ExactTupleMap::iterator i=mExactTransports.begin(); i!=mExactTransports.end(); ++i)
   {
      i->second->shutdown();
   }
   for (AnyInterfaceTupleMap::iterator i=mAnyInterfaceTransports.begin(); i!=mAnyInterfaceTransports.end(); ++i)
   {
      i->second->shutdown();
   }
}

bool
TransportSelector::isFinished() const
{
   for (ExactTupleMap::const_iterator i=mExactTransports.begin(); i!=mExactTransports.end(); ++i)
   {
      if (!i->second->isFinished()) return false;
   }
   for (AnyInterfaceTupleMap::const_iterator i=mAnyInterfaceTransports.begin(); i!=mAnyInterfaceTransports.end(); ++i)
   {
      if (!i->second->isFinished()) return false;
   }
   return true;
}


// !jf! Note that it uses ipv6 here but ipv4 in the Transport classes (ugggh!)
bool
TransportSelector::addTransport( TransportType protocol,
                                 int port, 
                                 IpVersion version,
                                 const Data& ipInterface , 
                                 const Data& sipDomainname ,
                                 const Data& privateKeyPassPhrase ,
                                 SecurityTypes::SSLType sslType  )
{
   assert( port >  0 );
   Transport* transport=0;

   try
   {
      switch (protocol)
      {
         case UDP:
            transport = new UdpTransport(mStateMacFifo, port, version, ipInterface);
            break;
         case TCP:
            transport = new TcpTransport(mStateMacFifo, port, version, ipInterface);
            break;
         case TLS:
#if defined( USE_SSL )  
            assert(mTlsTransports.count(sipDomainname) == 0);
            transport = new TlsTransport(mStateMacFifo, 
                                         port, 
                                         version, 
                                         ipInterface,
                                         *mSecurity, 
                                         sipDomainname,
                                         sslType);
#else
            CritLog (<< "TLS not supported in this stack. You don't have openssl");
            assert(0);
            return false;
#endif
            break;
         case DTLS:
#if defined( USE_DTLS )
            assert(mDtlsTransports.count(sipDomainname) == 0);
            transport = new DtlsTransport(mStateMacFifo, 
                                          port,
                                          version,
                                          ipInterface,
                                          *mSecurity, 
                                          sipDomainname);
#else
            CritLog (<< "TLS not supported in this stack. You don't have openssl");
            assert(0);
            return false;
#endif
            break;
         default:
            assert(0);
            break;
      }
   }
   catch (Transport::Exception& )
   {
      ErrLog(<< "Failed to create transport: " 
             << (version == V4 ? "V4" : "V6") << " "
             << Tuple::toData(protocol) << " " << port << " on "
             << (ipInterface.empty() ? "ANY" : ipInterface.c_str()));
      return false;
   }

   if (mMultiThreaded)
   {
      transport->run();
   }
    
   switch (protocol)
   {
      case UDP:
      case TCP:
      {
         Tuple key(ipInterface, port, version == V4, protocol);
         assert(mExactTransports.find(key) == mExactTransports.end() &&
                mAnyInterfaceTransports.find(key) == mAnyInterfaceTransports.end());
         
         InfoLog (<< "Adding transport: " << key);
         
         // Store the transport in the ANY interface maps if the tuple specifies ANY
         // interface. Store the transport in the specific interface maps if the tuple
         // specifies an interface. See TransportSelector::findTransport.
         if (ipInterface.empty())
         {
            mAnyInterfaceTransports[key] = transport;
            mAnyPortAnyInterfaceTransports[key] = transport;
         }
         else
         {
            mExactTransports[key] = transport;
            mAnyPortTransports[key] = transport;
         }
      }
      break;
      case TLS:
      {  
         assert(  dynamic_cast<TlsTransport*>(transport) );
         mTlsTransports[sipDomainname] = dynamic_cast<TlsTransport*>(transport);
      }
      break;
#ifdef USE_DTLS
      case DTLS:
      {
         assert( dynamic_cast<DtlsTransport*>(transport) ) ;
         mDtlsTransports[ sipDomainname ] = 
            dynamic_cast<DtlsTransport *>(transport) ;
      }
      break;
#endif
      default:
         assert(0);
         break;
   }
   
   return true;
}


void 
TransportSelector::process(FdSet& fdset)
{
   mDns.process(fdset);
   
   if (!mMultiThreaded)
   {
      for (ExactTupleMap::const_iterator i = mExactTransports.begin();
           i != mExactTransports.end(); i++)
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
      for (AnyInterfaceTupleMap::const_iterator i = mAnyInterfaceTransports.begin();
           i != mAnyInterfaceTransports.end(); i++)
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

      for (std::map<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
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

#ifdef USE_DTLS
      for (std::map<Data, DtlsTransport*>::const_iterator i=mDtlsTransports.begin(); 
           i != mDtlsTransports.end(); i++)
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
#endif
   }
}

bool 
TransportSelector::hasDataToSend() const
{   
   if (!mMultiThreaded)
   {
      for (ExactTupleMap::const_iterator i = mExactTransports.begin();
           i != mExactTransports.end(); i++)
      {
         if (i->second->hasDataToSend())
         {
            return true;
         }
      }
      for (AnyInterfaceTupleMap::const_iterator i = mAnyInterfaceTransports.begin();
           i != mAnyInterfaceTransports.end(); i++)
      {
         if (i->second->hasDataToSend())
         {
            return true;
         }
      }

      for (std::map<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
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
//!jf! the problem here is that DnsResult is returned after looking
//mDns.lookup() but this can result in a synchronous call to handle() which
//assumes that dnsresult has been assigned to the TransactionState
//!dcm! -- now 2-phase to fix this  
DnsResult* 
TransportSelector::createDnsResult(DnsHandler* handler)
{
   return mDns.createDnsResult(handler);
}

void
TransportSelector::dnsResolve(DnsResult* result, 
                              SipMessage* msg)
{
   // Picking the target destination:
   //   - for request, use forced target if set
   //     otherwise use loose routing behaviour (route or, if none, request-uri)
   //   - for response, use forced target if set, otherwise look at via  

   if (msg->isRequest())
   {
      // If this is an ACK we need to fix the tid to reflect that
      if (msg->hasForceTarget())
      {
          //DebugLog(<< "!ah! RESOLVING request with force target : " << msg->getForceTarget() );
         mDns.lookup(result, msg->getForceTarget());         
      }
      else if (msg->exists(h_Routes) && !msg->header(h_Routes).empty())
      {
         // put this into the target, in case the send later fails, so we don't
         // lose the target
         msg->setForceTarget(msg->header(h_Routes).front().uri());
         DebugLog (<< "Looking up dns entries (from route) for " << msg->getForceTarget());
         mDns.lookup(result, msg->getForceTarget());         
      }
      else
      {
         DebugLog (<< "Looking up dns entries for " << msg->header(h_RequestLine).uri());
         mDns.lookup(result, msg->header(h_RequestLine).uri());         
      }
   }
   else if (msg->isResponse())
   {
       ErrLog(<<"unimplemented response dns");
       assert(0);
   }
   else
   {
      assert(0);
   }
}

Tuple
TransportSelector::determineSourceInterface(SipMessage* msg, const Tuple& target) const
{
   assert(msg->exists(h_Vias));
   assert(!msg->header(h_Vias).empty());
   const Via& via = msg->header(h_Vias).front();
   if (msg->isRequest() && !via.sentHost().empty())
   // hint provided in sent-by of via by application
   {
      return Tuple(via.sentHost(), via.sentPort(), target.isV4(), target.getType());
   }
   else
   {
      Tuple source(target);
      switch (mWindowsVersion)
      {
         case WinCompat::NotWindows:

// Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's if linked in
//        statically.  If you don't care about Win95/98/Me as your target system - then
//        you can define NO_IPHLPAPI so that you are not required to link with this 
//        library. (SLG)
// Note:  WinCompat::determineSourceInterface uses IPHLPAPI and is only required for
//        Win95/98/Me and to work around personal firewall issues.
#ifdef NO_IPHLPAPI  
         default:
#endif
         {
            // this process will determine which interface the kernel would use to
            // send a packet to the target by making a connect call on a udp socket. 
            Socket tmp = INVALID_SOCKET;
            if (target.isV4())
            {
               if (mSocket == INVALID_SOCKET)
               {
                  mSocket = Transport::socket(UDP, true); // may throw
               }
               tmp = mSocket;
            }
            else
            {
               if (mSocket6 == INVALID_SOCKET)
               {
                  mSocket6 = Transport::socket(UDP, false); // may throw
               }
               tmp = mSocket6;
            }
   
            int ret = connect(tmp,&target.getSockaddr(), target.length());
            if (ret < 0)
            {
               int e = getErrno();
               Transport::error( e );
               InfoLog(<< "Unable to route to " << target << " : [" << e << "] " << strerror(e) );
               throw Transport::Exception("Can't find source address for Via", __FILE__,__LINE__);
            }
            
            socklen_t len = source.length();  
            ret = getsockname(tmp,&source.getMutableSockaddr(), &len);
            if (ret < 0)
            {
               int e = getErrno();
               Transport::error(e);
               InfoLog(<< "Can't determine name of socket " << target << " : " << strerror(e) );
               throw Transport::Exception("Can't find source address for Via", __FILE__,__LINE__);
            }

            // Unconnect. 
            // !jf! This is necessary, but I am not sure what we can do if this
            // fails. I'm not sure the stack can recover from this error condition. 
            if (target.isV4())
            {
               ret = connect(mSocket,(struct sockaddr*)&mUnspecified,sizeof(mUnspecified));
            }
#ifdef USE_IPV6
            else
            {
               ret = connect(mSocket6,(struct sockaddr*)&mUnspecified6,sizeof(mUnspecified6));
            }
#else
            else
            {
               assert(0);
            }
#endif
   
            if ( ret<0 )
            {
               int e =  getErrno();
               if  ( e != EAFNOSUPPORT )
               {
                  ErrLog(<< "Can't disconnect socket :  " << strerror(e) );
                  Transport::error(e);
                  throw Transport::Exception("Can't disconnect socket", __FILE__,__LINE__);
               }
            }
            break;
         }

#ifndef NO_IPHLPAPI  
         default:
            try
            {
               // will not work on ipv6
               source = WinCompat::determineSourceInterface(target);
            }
            catch (WinCompat::Exception&)
            {
               ErrLog (<< "Can't find source interface to use");
               throw Transport::Exception("Can't find source interface", __FILE__, __LINE__);
            }
            break;
#endif
      }

      // This is the port that the request will get sent out from. By default,
      // this value will be 0, since the Helper that creates the request will
      // not assign it. In this case, the stack will pick an arbitrary (but
      // appropriate) transport. If it is non-zero, it will only match
      // transports that are bound to the specified port (and fail if none are
      // available) 
      source.setPort(via.sentPort());
      
     
      DebugLog (<< "Looked up source for destination: " << target 
                << " -> " << source 
                << " sent-by=" << via.sentHost() 
                << " sent-port=" << via.sentPort());

      return source;
           
   }
}


// !jf! there may be an extra copy of a tuple here. can probably get rid of it
// but there are some const issues.  
void 
TransportSelector::transmit(SipMessage* msg, Tuple& target)
{
   assert(msg);
   
   try
   {
     // !ah! You NEED to do this for responses too -- the transport doesn't
     // !ah! know it's IP addres(es) in all cases, AND it's function of the dest.
     // (imagine a synthetic message...)

      Tuple source;
      if (msg->isRequest())
      {
         // there must be a via, use the port in the via as a hint of what
         // port to send on
         source = determineSourceInterface(msg, target);

         // would already be specified for ACK or CANCEL
         if (target.transport == 0)
         {
            if (target.getType() == TLS)
            {
               target.transport = findTlsTransport(msg->getTlsDomain());
            }
            else
            {
               target.transport = findTransport(source);
            }
         }
         
         if (target.transport) // findTransport may have failed
         {
            Via& topVia(msg->header(h_Vias).front());
            topVia.remove(p_maddr); // !jf! why do this? 

            // insert the via
            if (topVia.transport().empty())
            {
               topVia.transport() = Tuple::toData(target.transport->transport());
            }
            if (!topVia.sentHost().size())
            {
               msg->header(h_Vias).front().sentHost() = DnsUtil::inet_ntop(source);
            }
            if (!topVia.sentPort())
            {
               msg->header(h_Vias).front().sentPort() = target.transport->port();
            }
         }
      }
      else if (msg->isResponse())
      {
         // We assume that all stray responses have been discarded, so we always
         // know the transport that the corresponding request was received on
         // and this has been copied by TransactionState::sendToWire into target.transport
         assert(target.transport);
         if (target.transport->getTuple().isAnyInterface())
         {
            source = determineSourceInterface(msg, target);
         }
         else
         {
            source = target.transport->getTuple();
         }
      }
      else
      {
         assert(0);
      }

      if (target.transport)
      {
         // There is a contact header and it contains exactly one entry
         if (msg->exists(h_Contacts) && !msg->header(h_Contacts).empty())
         {
            for (NameAddrs::iterator i=msg->header(h_Contacts).begin(); i != msg->header(h_Contacts).end(); i++)
            {
               NameAddr& contact = *i;
               // No host specified, so use the ip address and port of the
               // transport used. Otherwise, leave it as is. 
               if (contact.uri().host().empty())
               {
                  contact.uri().host() = DnsUtil::inet_ntop(source);
                  contact.uri().port() = target.transport->port();

		  if (msg->isRequest() && target.transport->transport() != UDP)
		  {
		     DebugLog(<< "added transport to Contact");
		     contact.uri().param(p_transport) = Tuple::toData(target.transport->transport());
		  }

                  DebugLog(<<"!sipit! Populated Contact: " << contact);
               }
            }
         }
         
         // See draft-ietf-sip-identity
         if (mSecurity && msg->exists(h_Identity) && msg->header(h_Identity).value().empty())
         {
            if (!msg->exists(h_Date))
            {
               DateCategory now;
               msg->header(h_Date) = now;
            }
#if defined(USE_SSL)
            try
            {
               Data domain = msg->header(h_From).uri().host();
               msg->header(h_Identity).value() = mSecurity->computeIdentity( domain, 
                                                                             msg->getCanonicalIdentityString());
            }
            catch (Security::Exception& e)
            {
               ErrLog (<< "Couldn't add identity header: " << e);
               msg->remove(h_Identity);
            }
#endif
         }

         Data& encoded = msg->getEncoded();
         encoded.clear();
         encoded.reserve(1500);
         DataStream encodeStream(encoded);
         msg->encode(encodeStream);
         encodeStream.flush();

         assert(!msg->getEncoded().empty());
         DebugLog (<< "Transmitting to " << target
		   << " via " << source 
                   << encoded.escaped());
         target.transport->send(target, encoded, msg->getTransactionId());
      }
      else
      {
         InfoLog (<< "tid=" << msg->getTransactionId() << " failed to find a transport to " << target);
         mStateMacFifo.add(new TransportMessage(msg->getTransactionId(), true));
      }

   }
   catch (Transport::Exception& )
   {
      InfoLog (<< "tid=" << msg->getTransactionId() << " no route to target: " << target);
      mStateMacFifo.add(new TransportMessage(msg->getTransactionId(), true));
      return;
   }
}

void
TransportSelector::retransmit(SipMessage* msg, Tuple& target)
{
   assert(target.transport);

   // !jf! The previous call to transmit may have blocked or failed (It seems to
   // block in windows when the network is disconnected - don't know why just
   // yet.
   if(!msg->getEncoded().empty())
   {
      //DebugLog(<<"!ah! retransmit to " << target);
      target.transport->send(target, msg->getEncoded(), msg->getTransactionId());
   }
}

unsigned int
TransportSelector::sumTransportFifoSizes() const
{
   unsigned int sum = 0;

   for (AnyPortTupleMap::const_iterator i = mAnyPortTransports.begin();
        i != mAnyPortTransports.end(); ++i)
   {
      sum += i->second->getFifoSize();
   }

   for (AnyPortAnyInterfaceTupleMap::const_iterator i = mAnyPortAnyInterfaceTransports.begin();
        i != mAnyPortAnyInterfaceTransports.end(); ++i)
   {
      sum += i->second->getFifoSize();
   }
   
   for (TlsTransportMap::const_iterator i = mTlsTransports.begin();
        i != mTlsTransports.end(); ++i)
   {
      sum += i->second->getFifoSize();
   }

   return sum;
}

void 
TransportSelector::buildFdSet(FdSet& fdset)
{
   mDns.buildFdSet(fdset);
   
   if (!mMultiThreaded)
   {
      for (ExactTupleMap::const_iterator i = mExactTransports.begin();
           i != mExactTransports.end(); ++i)
      {
         i->second->buildFdSet(fdset);
      }
      for (AnyInterfaceTupleMap::const_iterator i = mAnyInterfaceTransports.begin();
           i != mAnyInterfaceTransports.end(); ++i)
      {
         i->second->buildFdSet(fdset);
      }
   
      for (std::map<Data, TlsTransport*>::const_iterator i=mTlsTransports.begin(); 
           i != mTlsTransports.end(); ++i)
      {
         (i->second)->buildFdSet(fdset);
      }

#ifdef USE_DTLS
      for (std::map<Data, DtlsTransport*>::const_iterator i=mDtlsTransports.begin(); 
           i != mDtlsTransports.end(); ++i)
      {
         (i->second)->buildFdSet(fdset);
      }
#endif
   }
}

Transport*
TransportSelector::findTransport(const Tuple& search) 
{
   DebugLog(<< "findTransport(" << search << ")");

   if (search.getPort() != 0)
   {
      // 1. search for matching port on a specific interface
      {
         ExactTupleMap::const_iterator i = mExactTransports.find(search);
         if (i != mExactTransports.end())
         {
            DebugLog(<< "findTransport (exact) => " << *(i->second));
            return i->second;
         }
      }

      // 2. search for specific port on ANY interface
      {
         AnyInterfaceTupleMap::const_iterator i = mAnyInterfaceTransports.find(search);
         if (i != mAnyInterfaceTransports.end())
         {
            DebugLog(<< "findTransport (any interface) => " << *(i->second));
            return i->second;
         }
      }
   }
   else
   {
      // 1. search for ANY port on specific interface
      {
         AnyPortTupleMap::const_iterator i = mAnyPortTransports.find(search);
         if (i != mAnyPortTransports.end())
         {
            DebugLog(<< "findTransport (any port, specific interface) => " << *(i->second));
            return i->second;
         }
      }

      // 2. search for ANY port on ANY interface
      {
         //CerrLog(<< "Trying AnyPortAnyInterfaceTupleMap " << mAnyPortAnyInterfaceTransports.size());
         AnyPortAnyInterfaceTupleMap::const_iterator i = mAnyPortAnyInterfaceTransports.find(search);
         if (i != mAnyPortAnyInterfaceTransports.end())
         {
            DebugLog(<< "findTransport (any port, any interface) => " << *(i->second));
            return i->second;
         }
      }
   }

   DebugLog (<< "Exact interface / Specific port: " << Inserter(mExactTransports));
   DebugLog (<< "Any interface / Specific port: " << Inserter(mAnyInterfaceTransports));
   DebugLog (<< "Exact interface / Any port: " << Inserter(mAnyPortTransports));
   DebugLog (<< "Any interface / Any port: " << Inserter(mAnyPortAnyInterfaceTransports));
   
   WarningLog(<< "Can't find matching transport " << search);
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
