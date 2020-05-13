#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>   // Required for freeaddrinfo implementation in Windows 2000, NT, Me/95/98
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"

#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionState.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/InternalTransport.hxx"
#include "resip/stack/TcpBaseTransport.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/UdpTransport.hxx"
#include "resip/stack/WsTransport.hxx"
#include "resip/stack/Uri.hxx"

#include "rutil/DataStream.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/dns/DnsStub.hxx"
#ifdef USE_NETNS
#   include "rutil/NetNs.hxx"
#endif
#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/SigcompMessage.h>
#endif

#ifdef USE_SSL
#include "resip/stack/ssl/DtlsTransport.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#include "resip/stack/ssl/WssTransport.hxx"
#endif

#ifdef WIN32
#include "rutil/WinCompat.hxx"
#endif

#ifdef __MINGW32__
#define gai_strerror strerror
#endif

#include <utility>
#include <sys/types.h>

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TransportSelector::TransportSelector(Fifo<TransactionMessage>& fifo, Security* security, DnsStub& dnsStub, Compression &compression, bool useDnsVip) :
   mDns(dnsStub, useDnsVip),
   mStateMacFifo(fifo),
   mSecurity(security),
   mCompression(compression),
   mSigcompStack (0),
   mPollGrp(0),
   mAvgBufferSize(1024),
   mInterruptorHandle(0)
{
   memset(&mUnspecified.v4Address, 0, sizeof(sockaddr_in));
   mUnspecified.v4Address.sin_family = AF_UNSPEC;

#ifdef USE_IPV6
   memset(&mUnspecified6.v6Address, 0, sizeof(sockaddr_in6));
   mUnspecified6.v6Address.sin6_family = AF_UNSPEC;
#endif

#ifdef USE_SIGCOMP
   if (mCompression.isEnabled())
   {
      DebugLog (<< "Compression enabled for Transport Selector");
      mSigcompStack = new osc::Stack(mCompression.getStateHandler());
      mCompression.addCompressorsToStack(mSigcompStack);
   }
   else
   {
      DebugLog (<< "Compression disabled for Transport Selector");
   }
#else
   DebugLog (<< "No compression library available");
#endif
}

TransportSelector::~TransportSelector()
{
   mExactTransports.clear();
   mAnyInterfaceTransports.clear();
   mAnyPortTransports.clear();
   mAnyPortAnyInterfaceTransports.clear();
   mTlsTransports.clear();
   mSharedProcessTransports.clear();
   mHasOwnProcessTransports.clear();
   mTypeToTransportMap.clear();
   for(TransportKeyMap::iterator it = mTransports.begin(); it != mTransports.end(); it++)
   {
      delete it->second;
   }
#ifdef USE_SIGCOMP
   delete mSigcompStack;
#endif

   for(HashMap<Data, Socket>::iterator socketIterator = mSockets.begin();
       socketIterator != mSockets.end(); socketIterator++)
   {
      if (socketIterator->second != INVALID_SOCKET)
      {
         closeSocket(socketIterator->second);
         DebugLog(<< "Closing TransportSelector::mSocket[" << socketIterator->first << "]");
      }
   }

   for(HashMap<Data, Socket>::iterator socketIterator = mSocket6s.begin();
       socketIterator != mSocket6s.end(); socketIterator++)
   {
      if (socketIterator->second != INVALID_SOCKET)
      {
         closeSocket(socketIterator->second);
         DebugLog(<< "Closing TransportSelector::mSocket6[" << socketIterator->first << "]");
      }
   }

   setPollGrp(0);
}

void
TransportSelector::shutdown()
{
   for(TransportKeyMap::iterator it = mTransports.begin(); it != mTransports.end(); it++)
   {
       it->second->shutdown();
   }
}

bool
TransportSelector::isFinished() const
{
   for(TransportKeyMap::const_iterator it = mTransports.begin(); it != mTransports.end(); it++)
   {
      if (!it->second->isFinished())
      {
          return false;
      }
   }
   return true;
}

void
TransportSelector::addTransport(std::unique_ptr<Transport> autoTransport, bool isStackRunning)
{
   Transport* transport = autoTransport.release();

   // !bwc! This is a multimap from TransportType/IpVersion to Transport*.
   // Make _extra_ sure that no garbage goes in here.
   if(transport->transport()==TCP)
   {
      resip_assert(dynamic_cast<TcpTransport*>(transport));
   }
#ifdef USE_SSL
   else if(transport->transport()==TLS)
   {
      resip_assert(dynamic_cast<TlsTransport*>(transport));
   }
#endif
   else if(transport->transport()==UDP)
   {
      resip_assert(dynamic_cast<UdpTransport*>(transport));
   }
#ifdef USE_DTLS
#ifdef USE_SSL
   else if(transport->transport()==DTLS)
   {
      resip_assert(dynamic_cast<DtlsTransport*>(transport));
   }
#endif
#endif
   else if(transport->transport()==WS)
   {
      resip_assert(dynamic_cast<WsTransport*>(transport));
   }
#ifdef USE_SSL
   else if(transport->transport()==WSS)
   {
      resip_assert(dynamic_cast<WssTransport*>(transport));
   }
#endif
   else
   {
      resip_assert(0);
   }

   Tuple tuple(transport->interfaceName(), transport->port(),
               transport->ipVersion(), transport->transport(),
               Data::Empty, // Domain
               transport->netNs());
   tuple.mTransportKey = transport->getKey();

   if(!isSecure(transport->transport()))
   {
      if(mExactTransports.find(tuple) == mExactTransports.end() &&
         mAnyInterfaceTransports.find(tuple) == mAnyInterfaceTransports.end())
      {
         DebugLog (<< "Adding transport: " << tuple);

         // Store the transport in the ANY interface maps if the tuple specifies ANY
         // interface. Store the transport in the specific interface maps if the tuple
         // specifies an interface. See TransportSelector::findTransport.
         if (transport->interfaceName().empty() ||
             transport->getTuple().isAnyInterface() ||
             transport->hasSpecificContact() )
         {
            mAnyInterfaceTransports[tuple] = transport;
            mAnyPortAnyInterfaceTransports[tuple] = transport;
         }
         else
         {
            mExactTransports[tuple] = transport;
            mAnyPortTransports[tuple] = transport;
         }
      }
      else
      {
         WarningLog (<< "Can't add transport, overlapping properties with existing transport: " << tuple);
         resip_assert(false); // should never get here - checked in SipStack first
         return;
      }
   }
   else
   {
      tuple.setTargetDomain(transport->tlsDomain());
      TlsTransportKey key(tuple);
      if(mTlsTransports.find(key) == mTlsTransports.end())
      {
         mTlsTransports[key] = transport;
      }
      else
      {
         WarningLog (<< "Can't add transport, overlapping properties with existing transport: " << tuple);
         resip_assert(false); // should never get here - checked in SipStack first
         return;
      }
   }

   if (transport->shareStackProcessAndSelect())
   {
      if ( mPollGrp )
      {
         transport->setPollGrp(mPollGrp);
      }
      if(isStackRunning)
      {
         // When stack is running we need to add to mSharedProcessTransports from 
         // within TransportSelectorThread in order to be thread safe
         mTransportsToAddRemove.add(transport); 
      }
      else
      {
         // Stack isn't running yet - just add to mSharedProcess directly - this
         // ensures the mSharedProcessTransports list is propulated when the stack
         // starts to run.  When in multi-threaded stack mode the TransportSelectorThread 
         // will assume ownership (FdPollGrp control) of all the transports using this list.
         mSharedProcessTransports.push_back(transport);
      }
   }
   else
   {
      mHasOwnProcessTransports.push_back(transport);
      mHasOwnProcessTransports.back()->startOwnProcessing();
   }

   mTypeToTransportMap.insert(TypeToTransportMap::value_type(tuple,transport));
   mDns.addTransportType(transport->transport(), transport->ipVersion());
   mTransports[transport->getKey()] = transport;

   InfoLog(<< "TransportSelector::addTransport:  added transport for tuple=" << tuple << ", key=" << transport->getKey());
}

void
TransportSelector::removeTransport(unsigned int transportKey)
{
   Transport* transportToRemove = 0;

   // Find transport in global map and remove it
   // Note: it is important that this map is removed from before rebuildAnyPortTransportMaps is called,
   // since rebuildAnyPortTransportMaps uses this map.
   TransportKeyMap::iterator it = mTransports.find(transportKey);
   if(it != mTransports.end())
   {
       transportToRemove = it->second;
       mTransports.erase(it);
   }

   // If we found the transport - continue removal from other maps
   if(transportToRemove)
   {
      // notify transport to shutdown
      transportToRemove->shutdown();

      if(!isSecure(transportToRemove->transport()))
      {
         // Ensure transport is removed from all containers
         mExactTransports.erase(transportToRemove->getTuple());
         mAnyInterfaceTransports.erase(transportToRemove->getTuple());

         // In the AnyPort maps 2 transports can end up overwriting each other in these maps - then when we remove one, there may be none left - even though we should have an
         // entry.  The rebuilt method will dig through all transports again and rebuild these maps.
         rebuildAnyPortTransportMaps();
      }
      else
      {
         Tuple tlsRemoveTuple = transportToRemove->getTuple();
         tlsRemoveTuple.setTargetDomain(transportToRemove->tlsDomain());
         TlsTransportKey tlsKey(tlsRemoveTuple);
         mTlsTransports.erase(tlsKey);
      }

      // mTypeToTransportMap is a multimap - make sure to delete only this instance by looking up transportKey, instead of using 
      // mTypeToTransportMap.erase(transportToRemove->getTuple()); which might end up deleting more than 1 transport
      for (TypeToTransportMap::iterator itTypeToTransport = mTypeToTransportMap.begin(); itTypeToTransport != mTypeToTransportMap.end(); itTypeToTransport++)
      {
          if (itTypeToTransport->second->getKey() == transportKey)
          {
              mTypeToTransportMap.erase(itTypeToTransport);
              break;
          }
      }

      // Remove transport types from Dns list of supported protocols
      // Note:  DNS tracks use counts so that we will only remove this transport type if this is the last of the type to be removed
      mDns.removeTransportType(transportToRemove->transport(), transportToRemove->ipVersion());

      if (transportToRemove->shareStackProcessAndSelect())
      {
         // Note:  We called shutdown above, therefor the TransportSelectorThread can tell this 
         //        is a remove transpot request vs an addTransport request, by calling isShuttingDown().
         mTransportsToAddRemove.add(transportToRemove); // Need to add to mSharedProcessTransports from within TransportSelectorThread
         // Note:  Transport will be deleted from mTransportsToAddRemove handler (see checkTransportAddRemoveQueue)
      }
      else
      {
         for(TransportList::iterator it = mHasOwnProcessTransports.begin(); it != mHasOwnProcessTransports.end(); it++)
         {
            if((*it)->getKey() == transportKey)
            {
               mHasOwnProcessTransports.erase(it);
               break;
            }
         }

         // Delete transport
         delete transportToRemove;
      }
   }
}

void
TransportSelector::rebuildAnyPortTransportMaps()
{
    // These maps may contain less transports than what exists in the mTransports map, due to the fact that multiple transports can 
    // have the same index.  In these cases the last transport added that matches the custom map compare function is the only one that ends 
    // up in these maps.  Therefor we cannot just simply remove items and expect transprot selection to work as expected.  
    // We will clear these maps here.  Iterate through the master transport list and rebuild them back up.  This isn't very efficient,
    // but it only occurs when a transport is removed.

    mAnyPortTransports.clear();
    mAnyPortAnyInterfaceTransports.clear();

    for (TransportKeyMap::iterator it = mTransports.begin(); it != mTransports.end(); it++)
    {
        if (!isSecure(it->second->transport()))
        {
            // Store the transport in the ANY interface maps if the tuple specifies ANY
            // interface. Store the transport in the specific interface maps if the tuple
            // specifies an interface. See TransportSelector::findTransport.
            if (it->second->interfaceName().empty() ||
                it->second->getTuple().isAnyInterface() ||
                it->second->hasSpecificContact())
            {
                mAnyPortAnyInterfaceTransports[it->second->getTuple()] = it->second;
            }
            else
            {
                mAnyPortTransports[it->second->getTuple()] = it->second;
            }
        }
    }
}

void
TransportSelector::setPollGrp(FdPollGrp *grp)
{
   if(mPollGrp && mInterruptorHandle)
   {
      // unregister our select interruptor
      mPollGrp->delPollItem(mInterruptorHandle);
      mInterruptorHandle=0;
   }

   mPollGrp = grp;

   if(mPollGrp && mSelectInterruptor.get())
   {
      mInterruptorHandle = mPollGrp->addPollItem(mSelectInterruptor->getReadSocket(), FPEM_Read, mSelectInterruptor.get());
   }

   for(TransportList::iterator t=mSharedProcessTransports.begin(); t!=mSharedProcessTransports.end(); ++t)
   {
      (*t)->setPollGrp(mPollGrp);
   }
}

void 
TransportSelector::createSelectInterruptor()
{
   if(!mSelectInterruptor.get())
   {
      mSelectInterruptor.reset(new SelectInterruptor);
      if(mPollGrp)
      {
         mInterruptorHandle = mPollGrp->addPollItem(mSelectInterruptor->getReadSocket(), FPEM_Read, mSelectInterruptor.get());
      }
   }
}

void
TransportSelector::buildFdSet(FdSet& fdset)
{
   for(TransportList::iterator it = mSharedProcessTransports.begin(); it != mSharedProcessTransports.end(); it++)
   {
      (*it)->buildFdSet(fdset);
   }
   if(mSelectInterruptor.get())
   {
      mSelectInterruptor->buildFdSet(fdset);
   }
}

void
TransportSelector::process(FdSet& fdset)
{
   checkTransportAddRemoveQueue();

   for(TransportList::iterator it = mSharedProcessTransports.begin(); it != mSharedProcessTransports.end(); it++)
   {
      try
      {
         (*it)->process(fdset);
      }
      catch (BaseException& e)
      {
         ErrLog (<< "Exception thrown from Transport::process: " << e);
      }
   }

   if(mSelectInterruptor.get())
   {
      mSelectInterruptor->process(fdset);
   }
}

void 
TransportSelector::process()
{
   // This function will only be sufficient if these Transports are hooked into a FdPollGrp.
   checkTransportAddRemoveQueue();

   for(TransportList::iterator it = mSharedProcessTransports.begin(); it != mSharedProcessTransports.end(); it++)
   {
      try
      {
         (*it)->process();
      }
      catch (BaseException& e)
      {
         ErrLog (<< "Exception thrown from Transport::process: " << e);
      }
   }
}

void
TransportSelector::checkTransportAddRemoveQueue()
{
   // This method ensures we add/remove from/to the mSharedProcessTransports 
   // list from the TransportSelectorThread
   
   Transport* t(mTransportsToAddRemove.getNext(-1));
   while(t)
   {
      if(!t->isShuttingDown())
      {
         // If not shutting down then this is an addTransport request
         mSharedProcessTransports.push_back(t);
      }
      else
      {
         // If shutting down then this is a removeTransport request
         for(TransportList::iterator it = mSharedProcessTransports.begin(); it != mSharedProcessTransports.end(); it++)
         {
            if((*it)->getKey() == t->getKey())
            {
               mSharedProcessTransports.erase(it);
               break;
            }
         }

         // Now delete the transport
         delete t;
      }

      t = mTransportsToAddRemove.getNext(-1);
   }
}

void 
TransportSelector::poke()
{
   for(TransportList::iterator it = mHasOwnProcessTransports.begin(); it != mHasOwnProcessTransports.end(); it++)
   {
      try
      {
         (*it)->poke();
      }
      catch (BaseException& e)
      {
         ErrLog (<< "Exception thrown from Transport::process: " << e);
      }
   }

   if(mSelectInterruptor.get() /* && hasDataToSend() */)  // removed hasDataToSend call since it is not thread safe
   {
      mSelectInterruptor->handleProcessNotification();
   }
}

bool
TransportSelector::hasDataToSend() const
{
   for(TransportList::const_iterator it = mSharedProcessTransports.begin(); it != mSharedProcessTransports.end(); it++)
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
      else if (msg->exists(h_Routes) && !msg->const_header(h_Routes).empty())
      {
         // put this into the target, in case the send later fails, so we don't
         // lose the target
         msg->setForceTarget(msg->const_header(h_Routes).front().uri());
         DebugLog (<< "Looking up dns entries (from route) for " << msg->getForceTarget());
         mDns.lookup(result, msg->getForceTarget());
      }
      else
      {
         DebugLog (<< "Looking up dns entries for " << msg->const_header(h_RequestLine).uri());
         mDns.lookup(result, msg->const_header(h_RequestLine).uri());
      }
   }
   else if (msg->isResponse())
   {
      ErrLog(<<"unimplemented response dns");
      resip_assert(0);
   }
   else
   {
      resip_assert(0);
   }
}

// FIXME: maybe this should move to rutil/TransportType?
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
      case WS:
      case WSS:
         return   false;

      default:
         resip_assert(unknown_transport);
         return unknown_transport;  // !kh! just to make it compile wo/warning.
   }
}

Tuple
TransportSelector::getFirstInterface(bool is_v4, TransportType type)
{
// !kh! both getaddrinfo() and IPv6 are not supported by cygwin, yet.
#ifdef __CYGWIN__
   resip_assert(0);
   return Tuple();
#else
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

   return source;
#endif
}


/**
   Check the msg's top Via header for a source host&port that indicates a particular Transport.
   Do NOT do this for a response, as it would allow malicious downstream to insert bogus host 
   in via header that we would then use.
**/
Transport*
TransportSelector::findTransportByVia(SipMessage* msg, const Tuple& target, Tuple& source) const
{
   resip_assert(msg->exists(h_Vias));
   resip_assert(!msg->const_header(h_Vias).empty());
   const Via& via = msg->const_header(h_Vias).front();

   if (via.sentHost().empty() && via.transport().empty())
   {
      return 0;
   }

   // XXX: Is there better way to do below (without the copy)?
   source = Tuple(via.sentHost(), via.sentPort(), target.ipVersion(), 
      via.transport().empty() ? target.getType() : toTransportType(via.transport()), // Transport type is pre-populated in via, lock to it
      Data::Empty, target.getNetNs());
   DebugLog(<< "TransportSelector::findTransportByVia: source: " << source);

   if ( target.mFlowKey!=0 && (source.getPort()==0 || source.isAnyInterface()) )
   {
      WarningLog(<< "Sending request with incomplete Via header and FlowKey."
        <<" This code no smart enough to pick the correct Transport."
        <<" Via=" << via);
      resip_assert(0);
   }
   if ( source.isAnyInterface() )
   {
      // INADDR_ANY cannot go out on the wire, so remove it from
      // via header now.
      // transmit() will later use determineSourceInterface() to
      // get the actual interface to populate the Contact & Via headers.
      // Not sure if we should support this case or just assert.
      // .gh. This should be supported, in case only the transport part of the Via is set
      msg->header(h_Vias).front().sentHost().clear();
   }

   Transport *trans;
   if ( (trans = findTransportBySource(source, msg)) == NULL )
   {
      return NULL;
   }
   if(source.getPort()==0)
   {
      source.setPort(trans->port());
   }
   return trans;
}

Tuple
TransportSelector::determineSourceInterface(SipMessage* msg, const Tuple& target) const
{
   resip_assert(msg->exists(h_Vias));
   resip_assert(!msg->header(h_Vias).empty());
   const Via& via = msg->header(h_Vias).front();

   // this case should be handled already for UDP and TCP targets
   resip_assert((!(msg->isRequest() && !via.sentHost().empty())) || isSecure(target.getType()));
   if (1)
   {
      Tuple source(target);
#if defined(WIN32) && !defined(NO_IPHLPAPI)
      try
      {
         GenericIPAddress addr = WinCompat::determineSourceInterface(target.toGenericIPAddress());
         source.setSockaddr(addr);
      }
      catch (WinCompat::Exception& ex)
      {
         ErrLog (<< "Can't find source interface to use: " << ex);
         throw Transport::Exception("Can't find source interface", __FILE__, __LINE__);
      }
#else
      // !kh!
      // The connected UDP technique doesn't work all the time.
      // 1. Might not work on all implementaions as stated in UNP vol.1 8.14.
      // 2. Might not work under unspecified condition on Windows,
      //    search "getsockname" in MSDN library.
      // 3. We've experienced this issue on our production software.

      // this process will determine which interface the kernel would use to
      // send a packet to the target by making a connect call on a udp socket.
      Socket tmp = INVALID_SOCKET;
      Data netNs = target.getNetNs();
      // One IPV4 and IPV6 socket per namespace.  Even if we do not support netns,
      // we still have the default namespace of "" (empty string).
      if (target.isV4())
      {
         // If socket does not exist for namespace, create one
         if (mSockets.find(netNs) == mSockets.end() || mSockets[netNs] == INVALID_SOCKET)
         {
#ifdef USE_NETNS
            NetNs::setNs(netNs);
#endif
            mSockets[netNs] = InternalTransport::socket(UDP, V4); // may throw
         }
         tmp = mSockets[netNs];
      }
      else
      {
         // If socket does not exist for namespace, create one
         if (mSocket6s.find(netNs) == mSocket6s.end() || mSocket6s[netNs] == INVALID_SOCKET)
         {
#ifdef USE_NETNS
            NetNs::setNs(netNs);
#endif
            mSocket6s[netNs] = InternalTransport::socket(UDP, V6); // may throw
         }
         tmp = mSocket6s[netNs];
      }

#ifdef USE_NETNS
      // Not sure if connect has to be done in netns context or just the socket create
      NetNs::setNs(netNs);
#endif

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
//should never reach here in WIN32 w/ V6 support
#if defined(USE_IPV6) && !defined(WIN32)
         if (source.isAnyInterface())  //!dcm! -- when could this happen?
         {
            source = getFirstInterface(false, target.getType());
         }
# endif
      }
      // Unconnect.
      // !jf! This is necessary, but I am not sure what we can do if this
      // fails. I'm not sure the stack can recover from this error condition.
      if (target.isV4())
      {
         ret = connect(mSockets[netNs],
                       (struct sockaddr*)&mUnspecified.v4Address,
                       sizeof(mUnspecified.v4Address));
      }
#ifdef USE_IPV6
      else
      {
         ret = connect(mSocket6s[netNs],
                       (struct sockaddr*)&mUnspecified6.v6Address,
                       sizeof(mUnspecified6.v6Address));
      }
#else
      else
      {
         resip_assert(0);
      }
#endif

      if ( ret<0 )
      {
         int e =  getErrno();
         //.dcm. OS X 10.5 workaround, we could #ifdef for specific OS X version.
         if  (!(e ==EAFNOSUPPORT || e == EADDRNOTAVAIL))
         {
            ErrLog(<< "Can't disconnect socket :  " << strerror(e) );
            Transport::error(e);
            throw Transport::Exception("Can't disconnect socket", __FILE__,__LINE__);
         }
      }
#endif

      // This is the port that the request will get sent out from. By default,
      // this value will be 0, since the Helper that creates the request will not
      // assign it. In this case, the stack will pick an arbitrary (but appropriate)
      // transport. If it is non-zero, it will only match transports that are bound to
      // the specified port (and fail if none are available)

      if(msg->isRequest())
      {
         source.setPort(via.sentPort());
      }
      else
      {
         source.setPort(0);
      }

      DebugLog (<< "Looked up source for destination: " << target
                << " -> " << source
                << " sent-by=" << via.sentHost()
                << " sent-port=" << via.sentPort());

      return source;
   }
}

// !jf! there may be an extra copy of a tuple here. can probably get rid of it
// but there are some const issues.
TransportSelector::TransmitState
TransportSelector::transmit(SipMessage* msg, Tuple& target, SendData* sendData)
{
   resip_assert(msg);

   if(msg->mIsDecorated)
   {
      msg->rollbackOutboundDecorators();
   }

   try
   {
      TransportFailure::FailureReason transportFailureReason = TransportFailure::NoTransport;

      // !ah! You NEED to do this for responses too -- the transport doesn't
      // !ah! know it's IP addres(es) in all cases, AND it's function of the dest.
      // (imagine a synthetic message...)

      Tuple source;
      // .bwc. We need 3 things here:
      // 1) A Transport* to call send() on.
      // 2) A complete Tuple to pass in this call (target).
      // 3) A host, port, and protocol for filling out the topmost via, and
      //    possibly stuff like Contact, Referred-By, and other headers that
      //    must specify a hostname that the TU was unable to supply, because
      //    it didn't know what interface/port the message would be sent on.
      //    (source)
      /*
         Our Transport* might be found in target. If so, we can skip this block
         of stuff.
         Alternatively, we might not have the transport to start with. However,
         given a connection id, we will be able to find the Connection we
         should use, we can get the Transport we want. If we have no connection
         id, but we know we are using TLS, DTLS or WSS and have a tls hostname, we
         can use the hostname to find the appropriate transport. If all else
         fails, we must resort to the connected UDP trick to fill out source,
         which in turn is used to look up a matching transport.

         Given the transport, it is always possible to get the port/protocol,
         and usually possible to get the host (if it is bound to INADDR_ANY, we
         can't tell). However, if we had to fill out source in order to find
         the transport in the first place, this is not an issue.
      */

      Data remoteSigcompId;

      Transport* transport=0;

      if (msg->isRequest())
      {
         transport = findTransportByVia(msg, target, source);
         if (!transport)
         {
            if ((transport = findTransportByDest(target)) != NULL)
            {
               source = transport->getTuple();
               DebugLog(<< "Found transport: " << source);
            }
         }
         else
         {
            DebugLog(<< "Found transport: " << source);
         }
         
         if(!transport && target.mFlowKey && target.onlyUseExistingConnection)
         {
            // .bwc. Connection info was specified, and use of this connection
            // was mandatory, but connection is no longer around. We fail.
            transportFailureReason = TransportFailure::TransportNoExistConn;
         }
         else if (transport)// .bwc. Here we use transport to find source.
         {
            resip_assert( source.getType()!=0 );

            // .bwc. If the transport has an ambiguous interface, we need to
            //look a little closer.
            if(source.isAnyInterface())
            {
               Tuple temp = determineSourceInterface(msg,target);

               // .bwc. determineSourceInterface() can give us a port, if the TU
               // put one in the topmost Via.
               resip_assert(source.ipVersion()==temp.ipVersion() &&
                        source.getType()==temp.getType());
               source=temp;

               /* determineSourceInterface might return an arbitrary port
                  here, so use the port specified in transport->port().
               */
               if(source.getPort()==0)
               {
                  source.setPort(transport->port());
               }
            }
         }
         // .bwc. Here we use source to find transport.
         else
         {
            source = determineSourceInterface(msg, target);
            transport = findTransportBySource(source, msg);
            DebugLog(<< "Found transport: " << source);

            // .bwc. determineSourceInterface might give us a port
            if(transport && source.getPort()==0)
            {
               source.setPort(transport->port());
            }
         }

         target.mTransportKey = transport ? transport->getKey() : 0;

         // .bwc. Topmost Via is only filled out in the request case. Also, if
         // we don't have a transport at this point, we're going to fail,
         // so don't bother doing the work.
         if(target.mTransportKey)
         {
            Via& topVia(msg->header(h_Vias).front());
            topVia.remove(p_maddr); // !jf! why do this?

            // insert the via
            if (topVia.transport().empty())
            {
               topVia.transport() = Tuple::toData(source.getType());
            }
            if (!topVia.sentHost().size())
            {
               topVia.sentHost() = Tuple::inet_ntop(source);
            }
            if (!topVia.sentPort())
            {
               topVia.sentPort() = source.getPort();
            }

            if (mCompression.isEnabled())
            {
               // Indicate support for SigComp, if appropriate.
               if (!topVia.exists(p_comp))
               {
                  topVia.param(p_comp) = "sigcomp";
               }
               if (!topVia.exists(p_sigcompId))
               {
                  topVia.param(p_sigcompId) = mCompression.getSigcompId();
               }

               // Figure out remote identifier (from Route header
               // field, if present; otherwise, from Request-URI).
               // XXX rohc-sip-sigcomp-03 says to use +sip.instance,
               // but this is impossible to actually do if you're
               // not actually the registrar, and really hard even
               // if you are.
               Uri destination;// (*reinterpret_cast<Uri*>(0)); // !bwc! What?

               if(msg->exists(h_Routes) &&
                  !msg->const_header(h_Routes).empty())
               {
                  destination = msg->const_header(h_Routes).front().uri();
               }
               else
               {
                  destination = msg->const_header(h_RequestLine).uri();
               }

               if (destination.exists(p_comp) &&
                   destination.param(p_comp) == "sigcomp")
               {
                  if (destination.exists(p_sigcompId))
                  {
                      remoteSigcompId = destination.param(p_sigcompId);
                  }
                  else
                  {
                      remoteSigcompId = destination.host();
                  }
               }
               // Squirrel the compartment away in the branch so that
               // we can figure out (pre-transaction-matching) what
               // compartment incoming requests are associated with.
               topVia.param(p_branch).setSigcompCompartment(remoteSigcompId);
            }
         }
      }
      else if (msg->isResponse())
      {
         // We assume that all stray responses have been discarded, so we always
         // know the transport that the corresponding request was received on
         // and this has been copied by TransactionState::sendToWire into transport
         transport=findTransportByDest(target);

         // Transport used to receive the request, may have been removed.
         if(transport)
         {
            source = transport->getTuple();
            DebugLog(<< "Found transport: " << source);

            // .bwc. If the transport has an ambiguous interface, we need to
            //look a little closer.
            if(source.isAnyInterface())
            {
               Tuple temp = source;
               source = determineSourceInterface(msg,target);
               resip_assert(source.ipVersion()==temp.ipVersion() &&
                        source.getType()==temp.getType());

               /* determineSourceInterface might return an arbitrary port here,
                  so use the port specified in transport->port().
               */
               if(source.getPort()==0)
               {
                  source.setPort(transport->port());
               }
            }
            if (mCompression.isEnabled())
            {
               // Figure out remote identifier (from Via header field).
               Via& topVia(msg->header(h_Vias).front());

               if(topVia.exists(p_comp) && topVia.param(p_comp) == "sigcomp")
               {
                  if (topVia.exists(p_sigcompId))
                  {
                      remoteSigcompId = topVia.param(p_sigcompId);
                  }
                  else
                  {
                     // XXX rohc-sigcomp-sip-03 says "sent-by", but this should probably 
                     //     be "received" if present, and "sent-by" otherwise.
                     // XXX Also, the spec is ambiguous about whether to include the port 
                     //     in this identifier.
                     remoteSigcompId = topVia.sentHost();
                  }
               }
            }
         }
      }
      else
      {
         resip_assert(0);
      }

      // .bwc. At this point, source, transport, and target should be
      // _fully_ specified.

      if (transport)
      {
         // !bwc! TODO This filling in of stuff really should be handled with
         // the callOutboundDecorators() callback. (Or, at the very least,
         // we should allow this code to be turned off through configuration.
         // There are plenty of cases where this stuff is not at all necessary.)
         // There is a contact header and it contains exactly one entry
         if (msg->exists(h_Contacts) && msg->header(h_Contacts).size()==1)
         {
            for (NameAddrs::iterator i=msg->header(h_Contacts).begin(); i != msg->header(h_Contacts).end(); i++)
            {
               const NameAddr& c_contact = *i;
               NameAddr& contact = *i;
               // No host specified, so use the ip address and port of the
               // transport used. Otherwise, leave it as is.
               if (c_contact.uri().host().empty())
               {
                  contact.uri().host() = (transport->hasSpecificContact() ? 
                                          transport->interfaceName() : 
                                          Tuple::inet_ntop(source) );
                  contact.uri().port() = transport->port();

                  if (transport->transport() != UDP && !contact.uri().exists(p_gr))
                  {
                     contact.uri().param(p_transport) = Tuple::toDataLower(transport->transport());
                  }

                  // Add comp=sigcomp to contact URI
                  // Also, If no +sip.instance on contact HEADER,
                  // add sigcomp-id="<urn>" to contact URI.
                  if (mCompression.isEnabled())
                  {
                     if (!contact.uri().exists(p_comp))
                     {
                        contact.uri().param(p_comp) = "sigcomp";
                     }
                     if (!contact.exists(p_Instance) &&
                         !contact.uri().exists(p_sigcompId))
                     {
                        contact.uri().param(p_sigcompId) = mCompression.getSigcompId();
                     }
                  }
               }
               else
               {
                  if (c_contact.uri().exists(p_addTransport))
                  {
                     if (target.getType() != UDP)
                     {
                        contact.uri().param(p_transport) = Tuple::toDataLower(target.getType());
                     }
                     contact.uri().remove(p_addTransport);
                  }
               }
            }
         }

         // !bwc! TODO make this configurable
         // Fix the Referred-By header if no host specified.
         // If malformed, leave it alone.
         if (msg->exists(h_ReferredBy)
               && msg->const_header(h_ReferredBy).isWellFormed())
         {
            if (msg->const_header(h_ReferredBy).uri().host().empty())
            {
               msg->header(h_ReferredBy).uri().host() = Tuple::inet_ntop(source);
               msg->header(h_ReferredBy).uri().port() = transport->port();
            }
         }

         // !bwc! TODO make this configurable
         // .bwc. Only try fiddling with this is if the Record-Route is well-
         // formed. If the topmost Record-Route is malformed, we have no idea
         // whether it came from something the TU synthesized or from the wire.
         // We shouldn't touch it. Frankly, I take issue with the method we have
         // chosen to signal to the stack that we want it to fill out various
         // header-field-values.
         if (msg->exists(h_RecordRoutes)
               && !msg->const_header(h_RecordRoutes).empty() 
               && msg->const_header(h_RecordRoutes).front().isWellFormed())
         {
            const NameAddr& c_rr = msg->const_header(h_RecordRoutes).front();
            NameAddr& rr = msg->header(h_RecordRoutes).front();
            if (c_rr.uri().host().empty())
            {
               rr.uri().host() = Tuple::inet_ntop(source);
               rr.uri().port() = transport->port();
               if (target.getType() != UDP && !rr.uri().exists(p_transport))
               {
                  rr.uri().param(p_transport) = Tuple::toDataLower(target.getType());
               }
               // Add comp=sigcomp and sigcomp-id="<urn>" to Record-Route
               // XXX This isn't quite right -- should be set only
               // on routes facing the client. Doing this correctly
               // requires double-record-routing
               if (mCompression.isEnabled())
               {
                  if (!rr.uri().exists(p_comp))
                  {
                     rr.uri().param(p_comp) = "sigcomp";
                  }
                  if (!rr.uri().exists(p_sigcompId))
                  {
                     rr.uri().param(p_sigcompId) = mCompression.getSigcompId();
                  }
               }
            }
         }
         
         // !bwc! TODO make this configurable
         // See draft-ietf-sip-identity
         if (mSecurity && msg->exists(h_Identity) && msg->const_header(h_Identity).value().empty())
         {
            DateCategory now;
            msg->header(h_Date) = now;
#if defined(USE_SSL)
            try
            {
               const Data& domain = msg->const_header(h_From).uri().host();
               msg->header(h_Identity).value() = mSecurity->computeIdentity( domain,
                                                                             msg->getCanonicalIdentityString());
            }
            catch (Security::Exception& e)
            {
               InfoLog (<< "Couldn't add identity header: " << e);
               msg->remove(h_Identity);
               if (msg->exists(h_IdentityInfo))
               {
                  msg->remove(h_IdentityInfo);
               }
            }
#endif
         }

         resip_assert(target.mTransportKey == transport->getKey());

         // Call back anyone who wants to perform outbound decoration
         msg->callOutboundDecorators(source, target,remoteSigcompId);

         Transport::SipMessageLoggingHandler* handler = transport->getSipMessageLoggingHandler();
         if(handler)
         {
            handler->outboundMessage(source, target, *msg);
         }

         std::unique_ptr<SendData> send(new SendData(target,
                                                   resip::Data::Empty,
                                                   msg->getTransactionId(),
                                                   remoteSigcompId));

         send->data.reserve(mAvgBufferSize + mAvgBufferSize/4);

         DataStream str(send->data);
         msg->encode(str);
         str.flush();

         // !bwc! Moving average of message size. (Used to intelligently
         // predict how much space to reserve in the buffer, to minimize
         // dynamic resizing.)
         mAvgBufferSize = (255*mAvgBufferSize + send->data.size()+128)/256;

         resip_assert(!send->data.empty());
         DebugLog (<< "Transmitting to " << target
                   << " tlsDomain=" << msg->getTlsDomain()
                   << " via " << source
                   << std::endl << std::endl << send->data.escaped()
                   << "sigcomp id=" << remoteSigcompId);

         if(sendData)
         {
            *sendData = *send;
         }

         transport->send(std::move(send));
         return Sent;
      }
      else
      {
         InfoLog (<< "tid=" << msg->getTransactionId() << " failed to find a transport to " << target);
         mStateMacFifo.add(new TransportFailure(msg->getTransactionId(), transportFailureReason));
         return Unsent;
      }
   }
   catch (Transport::Exception& )
   {
      InfoLog (<< "tid=" << msg->getTransactionId() << " no route to target: " << target);
      mStateMacFifo.add(new TransportFailure(msg->getTransactionId(), TransportFailure::NoRoute));
      return Unsent;
   }
}

void
TransportSelector::retransmit(const SendData& data)
{
   resip_assert(data.destination.mTransportKey);
   Transport* transport = findTransportByDest(data.destination);

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

   if(transport)
   {
      // If this is not true, it means the transport has been removed.
      Transport::SipMessageLoggingHandler* handler = transport->getSipMessageLoggingHandler();
      if(handler)
      {
         handler->outboundRetransmit(transport->getTuple(), data.destination, data);
      }
       
      transport->send(std::unique_ptr<SendData>(data.clone()));
   }
}

void 
TransportSelector::closeConnection(const Tuple& peer)
{
   Transport* t = findTransportByDest(peer);
   if(t)
   {
      SendData* close=new SendData(peer, 
                                   resip::Data::Empty,
                                   resip::Data::Empty,
                                   resip::Data::Empty);
      close->command = SendData::CloseConnection;
      t->send(std::unique_ptr<SendData>(close));
   }
}

unsigned int
TransportSelector::sumTransportFifoSizes() const
{
   unsigned int sum = 0;
   for(TransportKeyMap::const_iterator it = mTransports.begin(); it != mTransports.end(); it++)
   {
      sum += it->second->getFifoSize();
   }
   return sum;
}

void 
TransportSelector::terminateFlow(const resip::Tuple& flow)
{
   closeConnection(flow);
}

void 
TransportSelector::enableFlowTimer(const resip::Tuple& flow)
{
   Transport* t = findTransportByDest(flow);
   if(t)
   {
      SendData* enableFlowTimer=new SendData(flow, 
                                    resip::Data::Empty,
                                    resip::Data::Empty,
                                    resip::Data::Empty);
      enableFlowTimer->command = SendData::EnableFlowTimer;
      t->send(std::unique_ptr<SendData>(enableFlowTimer));
   }
}

void 
TransportSelector::invokeAfterSocketCreationFunc(TransportType type)
{
    for (TransportKeyMap::iterator it = mTransports.begin(); it != mTransports.end(); it++)
    {
        if (type == UNKNOWN_TRANSPORT || type == it->second->transport())
        {
            it->second->invokeAfterSocketCreationFunc();
        }
    }
    if (type == UNKNOWN_TRANSPORT)
    {
        // !slg! TODO - invoke for DNS?
        //mDns.
    }
}

Transport*
TransportSelector::findTransportByDest(const Tuple& target)
{
   if(target.mTransportKey)
   {
      TransportKeyMap::iterator it = mTransports.find(target.mTransportKey);
      if(it != mTransports.end())
      {
          return it->second;
      }
   }
   else
   {
      std::pair<TypeToTransportMap::iterator, TypeToTransportMap::iterator> range(mTypeToTransportMap.equal_range(target));

      if(range.first != range.second) // At least one match
      {
         TypeToTransportMap::iterator i=range.first;
         ++i;
         if(i==range.second) // Exactly one match
         {
            return range.first->second;
         }
      }
   }

   // .bwc. No luck here. Maybe findTransportBySource will end up working.
   return 0;
}

/**
   Search for Transport on any loopback interface matching {search}.
   WATCHOUT: This is O(N) walk thru (nearly) all transports.
**/
Transport*
TransportSelector::findLoopbackTransportBySource(bool ignorePort, Tuple& search) const
{
   //When we are sending to a loopback address, the kernel makes an
   //(effectively) arbitrary choice of which loopback address to send
   //from. (Since any loopback address can be used to send to any other
   //loopback address) This choice may not agree with our idea of what
   //address we should be sending from, so we need to just choose the
   //loopback address we like, and ignore what the kernel told us to do.
   ExactTupleMap::const_iterator i;
   for (i=mExactTransports.begin();i != mExactTransports.end();i++)
   {
      DebugLog(<<"search: " << search << " elem: " << i->first);
      if(i->first.ipVersion()==V4)
      {
         //Compare only the first byte (the 127)
         if(i->first.isEqualWithMask(search,8,ignorePort))
         {
            // Not sure if this should go here or in Tuple::isEqualWithMask
            if(i->first.getNetNs() == search.getNetNs())
            {
               search=i->first;
               DebugLog(<<"Match!");
               return i->second;
            }
         }
      }
#ifdef USE_IPV6
      else if(i->first.ipVersion()==V6)
      {
         //What to do?
      }
#endif
      else
      {
         resip_assert(0);
      }
   }
   
   return 0;
} // of findLoopbackTransport

Transport*
TransportSelector::findTransportBySource(Tuple& search, const SipMessage* msg) const
{
   DebugLog(<< "findTransportBySource(" << search << ")");

   if(msg && 
      !msg->getTlsDomain().empty() && 
      isSecure(search.getType()))
   {
      // We should not be willing to attempt sending on a TLS/DTLS/WSS transport 
      // that does not have the cert we're attempting to use, even if the 
      // IP/port/proto match. If we have not specified which identity we want
      // to use, then proceed with the code below.
      return findTlsTransport(msg->getTlsDomain(),search.getType(),search.ipVersion());
   }

   bool ignorePort = (search.getPort() == 0);
   DebugLog(<< "should port be ignored: " << ignorePort);

   if (!ignorePort)
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

      // 2. search for matching port on any loopback interface
      if (search.isLoopback())
      {
         Transport *trans = findLoopbackTransportBySource( /*ignorePort*/false, search);
         if (trans)
         {
            DebugLog(<< "findLoopbackTransportBySource(" << search << ")");
            return trans;
         }
      }

      // 3. search for specific port on ANY interface
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
            DebugLog(<< "findTransport (any port, specific interface) => " << *(i->second) << " key: " << (i->first) << " search: " << search);
            return i->second;
         }
      }

      // 2. search for ANY port on any loopback interface
      if (search.isLoopback())
      {
         Transport *trans = findLoopbackTransportBySource( /*ignorePort*/true, search);
         if (trans)
         {
            return trans;
         }
      }

      // 3. search for ANY port on ANY interface
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
TransportSelector::findTlsTransport(const Data& domainname, TransportType type, IpVersion version) const
{
   resip_assert(isSecure(type));
   DebugLog(<< "Searching for " << toData(type) << " transport for domain='"
                  << domainname << "'" << " have " << mTlsTransports.size());

   if (domainname == Data::Empty)
   {
      for(TlsTransportMap::const_iterator i=mTlsTransports.begin(); i != mTlsTransports.end();++i)
      {
         if(i->first.mTuple.getType() == type && i->first.mTuple.ipVersion() == version)
         {
            DebugLog(<<"Found a default transport.");
            return i->second;
         }
      }
   }
   else
   {
      TlsTransportKey key(domainname, type, version);
      TlsTransportMap::const_iterator i=mTlsTransports.find(key);

      if(i!=mTlsTransports.end())
      {
         DebugLog(<< "Found a transport.");
         return i->second;
      }
   }

   DebugLog(<<"No transport found.");
   return 0;
}

unsigned int
TransportSelector::getTimeTillNextProcessMS()
{
   return hasDataToSend() ? 0 : INT_MAX;
}

void
TransportSelector::registerMarkListener(MarkListener* listener)
{
   mDns.getMarkManager().registerMarkListener(listener);
}

void TransportSelector::unregisterMarkListener(MarkListener* listener)
{
   mDns.getMarkManager().unregisterMarkListener(listener);
}

EncodeStream&
resip::operator<<(EncodeStream& ostrm, const TransportSelector::TlsTransportKey& tlsTransportKey)
{
   ostrm << tlsTransportKey.mTuple;
   
   return ostrm;
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
 * vi: set shiftwidth=3 expandtab:
 */
