#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "resiprocate/NameAddr.hxx"
#include "resiprocate/Uri.hxx"

#include "resiprocate/Security.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/InternalTransport.hxx"
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

TransportSelector::TransportSelector(Fifo<TransactionMessage>& fifo, Security* security) :
   mStateMacFifo(fifo),
   mSecurity(security),
   mSocket( INVALID_SOCKET ),
   mSocket6( INVALID_SOCKET ),
   mWindowsVersion(WinCompat::getVersion())
{
   memset(&mUnspecified.v4Address, 0, sizeof(sockaddr_in));
   mUnspecified.v4Address.sin_family = AF_UNSPEC;

#ifdef USE_IPV6
   memset(&mUnspecified6.v6Address, 0, sizeof(sockaddr_in6));
   mUnspecified6.v6Address.sin6_family = AF_UNSPEC;
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


//!dcm! Refactor wrt factory addtransport; do DtlsTransport/TlsTransport maps
//need to be specially typed. Deal w/ transports that desire their own thread,
//thread shutdown via destructor?
void
TransportSelector::addTransport( std::auto_ptr<Transport> tAuto)
{
   Transport* transport = tAuto.release();   
   mDns.addTransportType(transport->transport(), transport->ipVersion());
   switch (transport->transport())
   {
      case UDP:
      case TCP:
      {
         Tuple key(transport->interfaceName(), transport->port(), 
                   transport->ipVersion(), transport->transport());
         
         assert(mExactTransports.find(key) == mExactTransports.end() &&
                mAnyInterfaceTransports.find(key) == mAnyInterfaceTransports.end());

         //DebugLog (<< "Adding transport: " << key);

         // Store the transport in the ANY interface maps if the tuple specifies ANY
         // interface. Store the transport in the specific interface maps if the tuple
         // specifies an interface. See TransportSelector::findTransport.
         if (transport->interfaceName().empty())
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
         mTlsTransports[transport->tlsDomain()] = transport;
      }
      break;
#ifdef USE_DTLS
      case DTLS:
      {
         mDtlsTransports[transport->tlsDomain()] = transport;
      }
      break;
#endif
      default:
         assert(0);
         break;
   }

   if (transport->shareStackProcessAndSelect())
   {
      mSharedProcessTransports.push_back(transport);
   }
   else
   {
      mHasOwnProcessTransports.push_back(transport);
      transport->startOwnProcessing();
   }
}
  
void
TransportSelector::buildFdSet(FdSet& fdset) const
{
   mDns.buildFdSet(fdset);
   for(TransportList::const_iterator it = mSharedProcessTransports.begin(); 
       it != mSharedProcessTransports.end(); it++)
   {
      (*it)->buildFdSet(fdset);
   }
}

void
TransportSelector::process(FdSet& fdset)
{
   mDns.process(fdset);
   for(TransportList::iterator it = mSharedProcessTransports.begin(); 
       it != mSharedProcessTransports.end(); it++)
   {
      try
      {
         (*it)->process(fdset);
      }
      catch (BaseException& e)
      {
         InfoLog (<< "Exception thrown from Transport::process: " << e);
      }
   }
}

bool
TransportSelector::hasDataToSend() const
{
   for(TransportList::const_iterator it = mSharedProcessTransports.begin();
       it != mSharedProcessTransports.end(); it++)
   {
      if ((*it)->hasDataToSend())
      {
         return true;
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

namespace
{
   bool isDgramTransport (TransportType type)
   {
      static const bool unknown_transport = false;
      switch(type)
      {
      case UDP:
      case DTLS:
      case DCCP:
      case SCTP:
         return   true;

      case TCP:
      case TLS:
         return   false;

      default:
         assert(unknown_transport);
         return unknown_transport;  // !kh! just to make it compile wo/warning.
      }
   }

   Tuple
   getFirstInterface(bool is_v4, TransportType type)
   {
// !kh! both getaddrinfo() and IPv6 are supported by cygwin, yet.
#if !defined(__CYGWIN__)
      // !kh!
      // 1. Query local hostname.
      char hostname[256] = "";
      if(gethostname(hostname, sizeof(hostname)) != 0)
      {
         int e = getErrno();
         Transport::error( e );
         InfoLog(<< "Can't query local hostname : [" << e << "] " << strerror(e) );
         throw Transport::Exception("Can't query local hostname", __FILE__, __LINE__);
      }
      InfoLog(<< "Local hostname is [" << hostname << "]");

      // !kh!
      // 2. Resolve address(es) of local hostname for specified transport.
      const bool is_dgram = isDgramTransport(type);
      addrinfo hint;
      memset(&hint, 0, sizeof(hint));
      hint.ai_family    = is_v4 ? PF_INET : PF_INET6;
      hint.ai_flags     = AI_PASSIVE;
      hint.ai_socktype  = is_dgram ? SOCK_DGRAM : SOCK_STREAM;

      addrinfo* results;
      int ret = getaddrinfo(
         hostname,
         0,
         &hint,
         &results);

      if(ret != 0)
      {
         Transport::error( ret ); // !kh! is this the correct sematics? ret is not errno.
         InfoLog(<< "Can't resolve " << hostname << "'s address : [" << ret << "] " << gai_strerror(ret) );
         throw Transport::Exception("Can't resolve hostname", __FILE__,__LINE__);
      }

      // !kh!
      // 3. Use first address resolved if there are more than one.
      // What should I do if there are more than one address?
      // i.e. results->ai_next != 0.
      Tuple source(*(results->ai_addr), type);
      InfoLog(<< "Local address is " << source);
      addrinfo* ai = results->ai_next;
      for(; ai; ai = ai->ai_next)
      {
         Tuple addr(*(ai->ai_addr), type);
         InfoLog(<<"Additional address " << addr);
      }
      freeaddrinfo(results);

      return   source;
#else
      static const bool cygwin_not_supported = false;
      assert(cygwin_not_supported);
      return   Tuple();
#endif
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
      return Tuple(via.sentHost(), via.sentPort(), target.ipVersion(), target.getType());
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
            // !kh!
            // The connected UDP technique doesn't work all the time.
            // 1. Might not work on all implementaions as stated in UNP vol.1 8.14.
            // 2. Might not work under unspecified condition on Windows,
            //    search "getsockname" in MSDN library.
            // 3. We've experienced this issue on our production software.

            // this process will determine which interface the kernel would use to
            // send a packet to the target by making a connect call on a udp socket.
            Socket tmp = INVALID_SOCKET;
            if (target.isV4())
            {
               if (mSocket == INVALID_SOCKET)
               {
                  mSocket = InternalTransport::socket(UDP, V4); // may throw
               }
               tmp = mSocket;
            }
            else
            {
               if (mSocket6 == INVALID_SOCKET)
               {
                  mSocket6 = InternalTransport::socket(UDP, V6); // may throw
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

            // !kh! test if connected UDP technique results INADDR_ANY, i.e. 0.0.0.0.
            // if it does, assume the first avaiable interface.
            if(source.isV4())
            {
               long src = (reinterpret_cast<const sockaddr_in*>(&source.getSockaddr())->sin_addr.s_addr);
               if(src == INADDR_ANY)
               {
                  InfoLog(<< "Connected UDP failed to determine source address, use first address instaed.");
                  source = getFirstInterface(true, target.getType());
               }
            }
            else  // IPv6
            {
#if defined(USE_IPV6)
#  if defined(_MSC_VER)
                  static const bool ipv6_support_not_completed = false;
                  assert(ipv6_support_not_completed);
#  else
                  // !kh! this is compiled out on windows,
                  // for some reason VC dosen't recognize INADDR6_ANY,
                  // and I don't have time yet to find out why.
                  if (source.isAnyInterface())
                  {
                     source = getFirstInterface(false, target.getType());
                  }
#  endif
#else
               assert(0);
#endif
            }
            //if(source.getSockaddr() ==

            // Unconnect.
            // !jf! This is necessary, but I am not sure what we can do if this
            // fails. I'm not sure the stack can recover from this error condition.
            if (target.isV4())
            {
               ret = connect(mSocket,
                             (struct sockaddr*)&mUnspecified.v4Address,
                             sizeof(mUnspecified.v4Address));
            }
#ifdef USE_IPV6
            else
            {
               ret = connect(mSocket6,
                             (struct sockaddr*)&mUnspecified6.v6Address,
                             sizeof(mUnspecified6.v6Address));
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
               //target.transport = findTlsTransport(msg->header(h_From).uri().host());
            }
#if defined( USE_DTLS )
            else if (target.getType() == DTLS)
            {
               target.transport = findDTlsTransport(msg->getTlsDomain());
               //target.transport = findDtlsTransport(msg->header(h_From).uri().host());
            }
#endif
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
               const Data& domain = msg->header(h_From).uri().host();
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
         DataStream encodeStream(encoded);
         msg->encode(encodeStream);
         encodeStream.flush();

         assert(!msg->getEncoded().empty());
         DebugLog (<< "Transmitting to " << target
		   << " via " << source << '\n'
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

   // !kh!
   // My speculation for blocking is; when network is disconnected, WinSock sets
   // a timer (to give up) and tries to defer reporting the error, in hope of
   // the network would be recovered. Before the timer fires, applications could
   // still select() (or the likes) their socket descriptors and find they are
   // writable if there is still room in buffer (per socket). If the send()s or
   // sendto()s made during this time period overflows the buffer, it blocks.
   // But I somewhat doubt this would be noticed, because block would be brief,
   // once the timer fires, the blocked call would return error.
   // Note that the block applies to both UDP and TCP sockets.
   // Quote from Linux man page:
   // When the message does not fit into the send buffer of the socket,  send
   // normally  blocks, unless the socket has been placed in non-blocking I/O
   // mode. In non-blocking mode it would return EAGAIN in this case.
   // Quote from MSDN library:
   // If no buffer space is available within the transport system to hold the
   // data to be transmitted, sendto will block unless the socket has been
   // placed in a nonblocking mode.

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
   DebugLog (<< "Searching for TLS transport for domain='" << domainname << "'" << " have " << mTlsTransports.size());
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

#if defined( USE_DTLS )
Transport*
TransportSelector::findDtlsTransport(const Data& domainname)

{
   DebugLog (<< "Searching for DTLS transport for domain='" << domainname << "'");
   // If no domainname specified and there is only 1 TLS transport, use it.
   if (domainname == Data::Empty && mDtlsTransports.size() == 1)
   {
      DebugLog (<< "Found default DTLS transport for domain=" << mDtlsTransports.begin()->first);
      return (Transport*)mDtlsTransports.begin()->second;
   }
   else if (mDtlsTransports.count(domainname))
   {
      DebugLog (<< "Found DTLS transport for domain=" << mDtlsTransports.begin()->first);
      return (Transport*)mDtlsTransports[domainname];
   }
   else  // don't know which one to use
   {
      DebugLog (<< "No DTLS transport found");
      return 0;
   }
}
#endif


unsigned int 
TransportSelector::getTimeTillNextProcessMS() const
{
   if (mDns.requiresProcess())
   {
      return 50;      
   }
   else
   {
      return INT_MAX;
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
