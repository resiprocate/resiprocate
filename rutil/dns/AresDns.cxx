#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(WIN32)
#include <sys/types.h>
#endif
#include <time.h>

#include "rutil/dns/AresDns.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/Timer.hxx"

#include "AresCompat.hxx"
#if !defined(USE_CARES)
#include "ares_private.h"
#endif

#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/TransportType.hxx"

#if !defined(WIN32)
#if !defined(__CYGWIN__)
#include <arpa/nameser.h>
#endif
#endif

#ifndef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#else
#pragma warning(push)
#pragma warning(3 : 4996)
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

/**********************************************************************
 *
 *		class AresDnsPollItem
 *
 * This is callback class used for epoll-based systems.
 *
 **********************************************************************/

#if defined(USE_ARES)
namespace resip
{

class AresDnsPollItem : public FdPollItemBase
{
  public:
   AresDnsPollItem(FdPollGrp *grp, int fd, int using_tcp, AresDns& aresObj,
     ares_channel chan, int server_idx)
     : FdPollItemBase(grp, fd, FPEM_Read), mAres(aresObj),
       mChannel(chan), mFd(fd), mUsingTcp(using_tcp), mServerIdx(server_idx),
       mMask(FPEM_Read)
   {
   }

   virtual void	processPollEvent(FdPollEventMask mask);
   void resetPollGrp(FdPollGrp *grp)
   {
      if(mPollGrp)
      {
         mPollGrp->delPollItem(mPollHandle);
      }
      mPollHandle = 0;
      mPollGrp = grp;
      if(mPollGrp)
      {
         mPollHandle = mPollGrp->addPollItem(mFd, mMask, this);
      }
   }
   void updateMask(FdPollEventMask mask)
   {
      mMask = mask;
      if(mPollGrp)
      {
         mPollGrp->modPollItem(mPollHandle, mMask);
      }
   }

   AresDns&	mAres;
   ares_channel	mChannel;
   int mFd;
   int mUsingTcp;
   int mServerIdx;
   FdPollEventMask mMask;

   static void socket_poll_cb(void *cb_data,
                              ares_channel channel, int server_idx,
	                           int fd, int using_tcp, ares_poll_action_t act);
};

};

void
AresDnsPollItem::processPollEvent(FdPollEventMask mask)
{
   resip_assert( (mask&(FPEM_Read|FPEM_Write))!= 0 );

   time_t nowSecs;
   time(&nowSecs);	/// maybe nice if this was passed into us?

   ares_process_poll(mChannel, mServerIdx,
     (mask&FPEM_Read)?(int)mPollSocket:-1, (mask&FPEM_Write)?(int)mPollSocket:-1,
     nowSecs);
}

/**
   C-function called by ares whenever it opens, closes or changes
   interest in writability.
**/

void
AresDnsPollItem::socket_poll_cb(void *cb_data,
  	ares_channel channel, int server_idx,
  	int fd, int using_tcp, ares_poll_action_t act)
{
   AresDns *ares = static_cast<AresDns*>(cb_data);
   //assert( ares );
   FdPollGrp *grp = ares->mPollGrp;
   //assert( grp );
   AresDns::PollItemsMap::iterator it = ares->mPollItems.find(fd);
   AresDnsPollItem* olditem(NULL);
   if ( it != ares->mPollItems.end() )
   {
      resip_assert( it->second->mChannel==channel );
      resip_assert( it->second->mServerIdx==server_idx );
      resip_assert( it->second->mUsingTcp==using_tcp );
      olditem = it->second;
   }
   switch ( act )
   {
   case ARES_POLLACTION_OPEN:
      resip_assert( !olditem );
      resip_assert( it == ares->mPollItems.end() );
      resip_assert( fd!=INVALID_SOCKET );
      ares->mPollItems[fd] = new AresDnsPollItem(grp, fd, using_tcp, *ares, channel, server_idx);
      break;
   case ARES_POLLACTION_CLOSE:
      resip_assert( olditem );
      resip_assert( it != ares->mPollItems.end() );
      ares->mPollItems.erase(it);
      delete olditem;	// destructor removes from poll
      break;
   case ARES_POLLACTION_WRITEON:
      resip_assert( olditem );
      resip_assert( it != ares->mPollItems.end() );
      olditem->updateMask(FPEM_Read|FPEM_Write);
      break;
   case ARES_POLLACTION_WRITEOFF:
      resip_assert( olditem );
      resip_assert( it != ares->mPollItems.end() );
      olditem->updateMask(FPEM_Read);
      break;
   default:
      resip_assert( 0 );
   }
}

#elif defined(USE_CARES)
namespace resip
{

class AresDnsPollItem : public FdPollItemBase
{
  public:
   AresDnsPollItem(FdPollGrp *grp, Socket fd, FdPollEventMask mask,
      ares_channel channel)
     : FdPollItemBase(grp, fd, mask), mChannel(channel), mMask(mask)
   {
   }

   virtual void   processPollEvent(FdPollEventMask mask);
   void resetPollGrp(FdPollGrp *grp)
   {
      if(mPollGrp)
      {
         mPollGrp->delPollItem(mPollHandle);
      }
      mPollHandle = 0;
      mPollGrp = grp;
      if(mPollGrp)
      {
         mPollHandle = mPollGrp->addPollItem(mPollSocket, mMask, this);
      }
   }
   void updateMask(FdPollEventMask mask)
   {
      if (mMask != mask)
      {
         mMask = mask;
         if(mPollGrp)
         {
            mPollGrp->modPollItem(mPollHandle, mMask);
         }
      }
   }

   ares_channel mChannel;
   FdPollEventMask mMask;

   static void sockstate_cb(void* data, ares_socket_t sock, int read, int write);
};

}

void
AresDnsPollItem::processPollEvent(FdPollEventMask mask)
{
   resip_assert( (mask&(FPEM_Read|FPEM_Write|FPEM_Error))!= 0 );

   if (mask & FPEM_Error)
   {
      /* An error happened. Just pretend that the socket is both readable and */
      /* writable. */
      ares_process_fd(mChannel, mPollSocket, mPollSocket);
      return;
   }

   ares_process_fd(mChannel,
      (mask & FPEM_Read) ? mPollSocket : ARES_SOCKET_BAD,
      (mask & FPEM_Write) ? mPollSocket : ARES_SOCKET_BAD);
}

void
AresDnsPollItem::sockstate_cb(void* data, ares_socket_t sock, int read, int write)
{
   AresDns *ares = static_cast<AresDns*>(data);
   AresDns::PollItemsMap::iterator it = ares->mPollItems.find(sock);
   if (read || write)
   {
      FdPollEventMask mask(
         (read ? FPEM_Read : 0) | (write ? FPEM_Write : 0) | FPEM_Error);
      if (it == ares->mPollItems.end()) // new socket
      {
         ares->mPollItems[sock] =
            new AresDnsPollItem( ares->mPollGrp, sock, mask, ares->mChannel);
      }
      else
      {
         resip_assert( it->second->mChannel == ares->mChannel);
         it->second->updateMask(mask);
      }
   }
   else
   {
      /* read == 0 and write == 0 this is c-ares's way of notifying us that */
      /* the socket is now closed */
      resip_assert( it != ares->mPollItems.end());
      resip_assert( it->second->mChannel == ares->mChannel);
      AresDnsPollItem* item = it->second;
      ares->mPollItems.erase(it);
      delete item;
   }
}

// The name servers a channel uses, IPv4 and IPv6 with their ports, as a
// comma separated list.  Empty if they cannot be read.
static Data
caresServers(ares_channel channel)
{
#if ARES_VERSION >= 0x011800
   // ares_get_servers_csv() was added in c-ares 1.24, which deprecated
   // ares_get_servers_ports()
   char* csv = ares_get_servers_csv(channel);
   if (!csv)
   {
      return Data::Empty;
   }
   Data servers(csv);
   ares_free_string(csv);
   return servers;
#else
   Data servers;
   struct ares_addr_port_node* head = 0;
   if (ares_get_servers_ports(channel, &head) != ARES_SUCCESS)
   {
      return servers;
   }
   for (struct ares_addr_port_node* node = head; node; node = node->next)
   {
      if (!servers.empty())
      {
         servers += ",";
      }
      if (node->family == AF_INET6)
      {
#if defined(USE_IPV6)
         in6_addr addr6;
         memcpy(&addr6, &node->addr.addr6, sizeof(addr6));
         servers += "[" + DnsUtil::inet_ntop(addr6) + "]";
#else
         // DnsUtil formats no IPv6 here; dropIpv6Servers() only needs to
         // see that this one is IPv6
         servers += "[IPv6]";
#endif
      }
      else
      {
         servers += DnsUtil::inet_ntop(node->addr.addr4);
      }
      servers += ":" + Data(node->udp_port);
   }
   ares_free_data(head);
   return servers;
#endif
}

// Replaces the channel's name servers.  A port of 0 keeps the default, 53.
// Without USE_IPV6, IPv6 name servers are ignored, as the bundled ares does;
// if that leaves none, the channel keeps the ones c-ares found.
static int
setCaresServers(ares_channel channel, const std::vector<GenericIPAddress>& nameservers)
{
   // host[:port], with an IPv6 address in brackets when it has a port
   Data csv;
   for (size_t i = 0; i < nameservers.size(); ++i)
   {
      const bool v4 = nameservers[i].isVersion4();
#if defined(USE_IPV6)
      const Data address = v4 ? DnsUtil::inet_ntop(nameservers[i].v4Address.sin_addr)
                              : DnsUtil::inet_ntop(nameservers[i].v6Address.sin6_addr);
#else
      if (!v4)
      {
         WarningLog (<< "Ignoring non-IPv4 additional name server (IPv6 support was not enabled)");
         continue;
      }
      const Data address = DnsUtil::inet_ntop(nameservers[i].v4Address.sin_addr);
#endif
      const unsigned short port = ntohs(v4 ? nameservers[i].v4Address.sin_port
                                           : nameservers[i].v6Address.sin6_port);
      if (!csv.empty())
      {
         csv += ",";
      }
      if (port == 0)
      {
         csv += address;
      }
      else
      {
         csv += (v4 ? address : "[" + address + "]") + ":" + Data(port);
      }
   }
   if (csv.empty())
   {
      return ARES_SUCCESS;
   }
   return ares_set_servers_ports_csv(channel, csv.c_str());
}

#if !defined(USE_IPV6)
// This build does no IPv6, so the resolver uses IPv4 name servers only, as the
// bundled ares does: drop any IPv6 ones c-ares found in the system
// configuration.  If none are left, use 127.0.0.1, c-ares's own default.
static int
dropIpv6Servers(ares_channel channel)
{
   const std::string servers(caresServers(channel).c_str());
   std::string ipv4;
   bool dropped = false;
   for (size_t start = 0; start < servers.size(); )
   {
      size_t end = servers.find(',', start);
      if (end == std::string::npos)
      {
         end = servers.size();
      }
      const std::string server = servers.substr(start, end - start);
      start = end + 1;
      // "[addr]:port", or a bare address with more than one ':', is IPv6
      const size_t firstColon = server.find(':');
      if (server.empty() || server[0] == '[' ||
          (firstColon != std::string::npos && server.find(':', firstColon + 1) != std::string::npos))
      {
         dropped = dropped || !server.empty();
         continue;
      }
      ipv4 += (ipv4.empty() ? "" : ",") + server;
   }
   if (!dropped)
   {
      return ARES_SUCCESS;
   }
   InfoLog(<< "DNS initialization: ignoring IPv6 name servers (IPv6 support was not enabled)");
   return ares_set_servers_ports_csv(channel, ipv4.empty() ? "127.0.0.1" : ipv4.c_str());
}
#endif

// The socket function is called as for SIP TCP connections: before the
// socket connects, and again once it is connected if
// ExternalDns::setEnablePostConnectSocketFuncCall() is on.
int
AresDns::caresSocketConfigure(Socket s, int type, void* data)
{
   AresDns* ares = static_cast<AresDns*>(data);
   if (ares->mSocketFunc)
   {
      ares->mSocketFunc(s, type == SOCK_STREAM ? TCP : UDP, __FILE__, __LINE__);
   }
   return ARES_SUCCESS;
}

int
AresDns::caresSocketConnected(Socket s, int type, void* data)
{
   AresDns* ares = static_cast<AresDns*>(data);
   if (ares->mSocketFunc && getEnablePostConnectSocketFuncCall())
   {
      ares->mSocketFunc(s, type == SOCK_STREAM ? TCP : UDP, __FILE__, __LINE__);
   }
   return ARES_SUCCESS;
}

#endif

/**********************************************************************
 *
 *		class AresDns
 *
 **********************************************************************/

volatile bool AresDns::mHostFileLookupOnlyMode = false;

void
AresDns::setPollGrp(FdPollGrp *grp)
{
   for(PollItemsMap::iterator i=mPollItems.begin();
         i!=mPollItems.end(); ++i)
   {
      i->second->resetPollGrp(grp);
   }
   mPollGrp = grp;
}

void
AresDns::clearPollItems()
{
   PollItemsMap items;
   items.swap(mPollItems);
   for(PollItemsMap::iterator i=items.begin();
         i!=items.end(); ++i)
   {
      delete i->second;
   }
}

int
AresDns::init(const std::vector<GenericIPAddress>& additionalNameservers,
              AfterSocketCreationFuncPtr socketfunc,
              int timeout,
              int tries,
              unsigned int features)
{
   mAdditionalNameservers = additionalNameservers;
   mSocketFunc = socketfunc;

   return init(timeout, tries, features);
}

int 
AresDns::init(int dnsTimeout, int dnsTries, unsigned int features)
{
    mFeatures = features;

    int ret = internalInit(mAdditionalNameservers,
        mSocketFunc,
        features,
        &mChannel,
        dnsTimeout,
        dnsTries,
        true /* useStateFunc */);

    if (ret != Success)
        return ret;

    // Ensure map starts empty, since init may be called more than once
    clearPollItems();

#if defined(WIN32) && defined(USE_ARES)
    // For windows OSs it is uncommon to run a local DNS server.  Therefor if there
    // are no defined DNS servers in windows networking and ARES just returned the
    // loopback address (ie. default localhost server / named)
    // then put resip DNS resolution into hostfile lookup only mode
    if (mChannel->nservers == 1 &&
        mChannel->servers[0].default_localhost_server)
    {
        // enable hostfile only lookup mode
        mHostFileLookupOnlyMode = true;
    }
    else
    {
        // disable hostfile only lookup mode
        mHostFileLookupOnlyMode = false;
    }
#endif

    return Success;
}

int
AresDns::internalInit(const std::vector<GenericIPAddress>& additionalNameservers,
                      AfterSocketCreationFuncPtr socketfunc,
                      unsigned int features,
                      ares_channeldata** channel,
                      int timeout,
                      int tries,
                      bool useStateFunc)
{
   if(*channel)
   {
      // Fails all requests in progress with ARES_EDESTRUCTION, which their
      // handlers are told about (see resip_AresDns_aresCallback)
      ares_destroy(*channel);
      *channel = 0;
   }

#if defined(USE_ARES)

#ifdef USE_IPV6
   int requiredCap = ARES_CAP_IPV6;
#else
   int requiredCap = 0;
#endif

   // Only the contrib/ares has this function
   int cap = ares_capabilities(requiredCap);
   if (cap != requiredCap)
   {
      ErrLog (<< "Build mismatch (ipv4/ipv6) problem in ares library"); // !dcm!
      return BuildMismatch;
   }
#endif

   int status;
   ares_options opt;
   int optmask = 0;

   memset(&opt, '\0', sizeof(opt));

#if defined(USE_ARES)
   // TODO: What is this and how does it map to c-ares?
   if ((features & ExternalDns::TryServersOfNextNetworkUponRcode3))
   {
      optmask |= ARES_OPT_FLAGS;
      opt.flags |= ARES_FLAG_TRY_NEXT_SERVER_ON_RCODE3;
   }
#endif

   if ((features & ExternalDns::ReturnAllResults))
   {
      optmask |= ARES_OPT_FLAGS;
      opt.flags |= ARES_FLAG_NOCHECKRESP;
   }

#if defined(USE_CARES)
   // In c-ares, we can actually set the timeout and retries via the API
   if (timeout > 0)
   {
      opt.timeout = timeout;
      optmask |= ARES_OPT_TIMEOUT;
   }

   if (tries > 0)
   {
      opt.tries = tries;
      optmask |= ARES_OPT_TRIES;
   }

   if ( useStateFunc )
   {
      // Always track c-ares sockets, so that a FdPollGrp set after init()
      // (see setPollGrp) still picks them up.
      opt.sock_state_cb = AresDnsPollItem::sockstate_cb;
      opt.sock_state_cb_data = this;
      optmask |= ARES_OPT_SOCK_STATE_CB;
   }
#endif

#if defined(USE_ARES)
#if !defined(USE_IPV6)
   // This build does no IPv6: use only the IPv4 name servers, and if none are
   // left, the ones ares finds itself
   std::vector<in_addr> ipv4Nameservers;
   for (size_t i = 0; i < additionalNameservers.size(); i++)
   {
      if (additionalNameservers[i].isVersion4())
      {
         ipv4Nameservers.push_back(additionalNameservers[i].v4Address.sin_addr);
      }
      else
      {
         WarningLog (<< "Ignoring non-IPv4 additional name server (IPv6 support was not enabled)");
      }
   }
   if (ipv4Nameservers.empty())
#else
   if (additionalNameservers.empty())
#endif
   {
      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);
   }
   else
   {
      optmask |= ARES_OPT_SERVERS;

#if defined(USE_IPV6)
      opt.nservers = (int)additionalNameservers.size();
      // With contrib/ares, you can configure IPv6 addresses for the
      // nameservers themselves.
      opt.servers = new multiFamilyAddr[additionalNameservers.size()];
      for (size_t i =0; i < additionalNameservers.size(); i++)
      {
         if (additionalNameservers[i].isVersion4())
         {
            opt.servers[i].family = AF_INET;
            opt.servers[i].addr = additionalNameservers[i].v4Address.sin_addr;
         }
         else
         {
            opt.servers[i].family = AF_INET6;
            opt.servers[i].addr6 = additionalNameservers[i].v6Address.sin6_addr;
         }
      }
#else
      opt.nservers = (int)ipv4Nameservers.size();
      opt.servers = new in_addr[ipv4Nameservers.size()];
      for (size_t i = 0; i < ipv4Nameservers.size(); i++)
      {
         opt.servers[i] = ipv4Nameservers[i];
      }
#endif

      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);

      delete [] opt.servers;
      opt.servers = 0;
   }
#elif defined(USE_CARES)
   status = ares_init_options(channel, &opt, optmask);
   if (status == ARES_SUCCESS && !additionalNameservers.empty())
   {
      status = setCaresServers(*channel, additionalNameservers);
   }
#if !defined(USE_IPV6)
   if (status == ARES_SUCCESS)
   {
      status = dropIpv6Servers(*channel);
   }
#endif
   if (status == ARES_SUCCESS && socketfunc)
   {
      ares_set_socket_configure_callback(*channel, &AresDns::caresSocketConfigure, this);
      ares_set_socket_callback(*channel, &AresDns::caresSocketConnected, this);
   }
#endif
   if (status != ARES_SUCCESS)
   {
      ErrLog (<< "Failed to initialize DNS library (status=" << status << ")");
      return status;
   }
   else
   {

#if defined(USE_ARES)

      InfoLog(<< "DNS initialization: found  " << (*channel)->nservers << " name servers");
      for (int i = 0; i < (*channel)->nservers; ++i)
      {
#ifdef USE_IPV6
         if((*channel)->servers[i].family == AF_INET6)
         {
            InfoLog(<< " name server: " << DnsUtil::inet_ntop((*channel)->servers[i].addr6));
         }
         else
#endif
         {
            InfoLog(<< " name server: " << DnsUtil::inet_ntop((*channel)->servers[i].addr));
         }
      }

      // In ares, we must manipulate these directly
      if (timeout > 0)
      {
         (*channel)->timeout = timeout;
      }

      if (tries > 0)
      {
         (*channel)->tries = tries;
      }

      if ( useStateFunc )
      {
         // tell ares to let us know when things change; installed even
         // without a group, so that a FdPollGrp set after init() (see
         // setPollGrp) still picks up the sockets
         ares_process_set_poll_cb(*channel, AresDnsPollItem::socket_poll_cb, this);
      }

#elif defined(USE_CARES)
      {
         // Log which version of c-ares we're using
         InfoLog(<< "DNS initialization: using c-ares v"
                 << ::ares_version(NULL));

         InfoLog(<< "DNS initialization: name servers " << caresServers(*channel));
      }
#endif

      return Success;
   }
}

void
AresDns::changeNameServers(const std::vector<GenericIPAddress>& additional)
{
   mAdditionalNameservers = additional;
}

bool AresDns::checkDnsChange()
{
   // We must return 'true' if there are changes in the list of DNS servers
   struct ares_channeldata* channel = 0;
   bool bRet = false;
   int result = internalInit(mAdditionalNameservers, 0, mFeatures, &channel);
   if(result != Success || channel == 0)
   {
      // It has changed because it failed, I suppose
      InfoLog(<< " DNS server list changed");
      return true;
   }

#if defined(USE_ARES)
   {
      // Compare the two lists.  Are they different sizes?
      if(mChannel->nservers != channel->nservers)
      {
         // Yes, so they're different
         bRet = true;
      }
      else
      {
         // Compare them one-by-one
         for (int i = 0; i < mChannel->nservers; ++i)
         {
#ifdef USE_IPV6
            if(channel->servers[i].family == AF_INET6)
            {
               if(memcmp(&mChannel->servers[i].addr6, &channel->servers[i].addr6, sizeof(mChannel->servers[i].addr6)) != 0)
               {
                  bRet = true;
                  break;
               }
            }
            else
#endif
            {
               if (mChannel->servers[i].addr.s_addr != channel->servers[i].addr.s_addr)
               {
                  bRet = true;
                  break;
               }
            }
         }
      }

      // Destroy the secondary configuration we read
      ares_destroy_suppress_callbacks(channel);
   }
#elif defined(USE_CARES)
   {
      // Compare the server lists, IPv4 and IPv6 with their ports, of the old
      // and the current (i.e. just read) configuration.  If either cannot be
      // read, call it different.
      const Data old = caresServers(mChannel);
      const Data updated = caresServers(channel);
      bRet = old.empty() || updated.empty() || old != updated;

      // Destroy the secondary configuration we read
      ares_destroy(channel);
   }
#endif

   // Report on the results
   if(!bRet)
   {
      InfoLog(<< " No changes in DNS server list");
   }
   else
   {
      InfoLog(<< " DNS server list changed");
   }

   return bRet;
}

AresDns::~AresDns()
{
   // Fails the queries in flight with ARES_EDESTRUCTION; with mDestroying set
   // the callback frees their payloads without reporting them (see
   // resip_AresDns_aresCallback)
   mDestroying = true;
   ares_destroy(mChannel);
   clearPollItems();
}

bool AresDns::hostFileLookup(const char* target, in_addr &addr)
{
   resip_assert(target);

   hostent *hostdata = 0;

   // Look this up
   int status =
#if defined(USE_ARES)
     hostfile_lookup(target, &hostdata)
#elif defined(USE_CARES)
     ares_gethostbyname_file(mChannel, target, AF_INET, &hostdata)
#endif
     ;

   if (status != ARES_SUCCESS)
   {
      DebugLog(<< "hostFileLookup failed for " << target);
      return false;
   }
   sockaddr_in saddr;
   memset(&saddr,0,sizeof(saddr));  /* Initialize sockaddr fields. */
   saddr.sin_family = AF_INET;
   memcpy((char *)&(saddr.sin_addr.s_addr),(char *)hostdata->h_addr_list[0], (size_t)hostdata->h_length);
   addr = saddr.sin_addr;
#if defined(USE_ARES)
   // for resip-ares, the hostdata (and its contents) is dynamically allocated
   ares_free_hostent(hostdata);
#endif

   DebugLog(<< "hostFileLookup succeeded for " << target);
   return true;
}

ExternalDnsHandler*
AresDns::getHandler(void* arg)
{
   return reinterpret_cast<Payload*>(arg)->handler;
}

ExternalDnsRawResult
AresDns::makeRawResult(void *arg, int status, unsigned char *abuf, int alen)
{
   void* userArg = reinterpret_cast<Payload*>(arg)->userData;

   if (status != ARES_SUCCESS)
   {
      return ExternalDnsRawResult(status, abuf, alen, userArg);
   }
   else
   {
      return ExternalDnsRawResult(abuf, alen, userArg);
   }
}

unsigned int
AresDns::getTimeTillNextProcessMS()
{
   struct timeval tv;
   unsigned maxSystemTime = resip::Timer::getMaxSystemTimeWaitMs();
   tv.tv_sec = maxSystemTime / 1000;
   tv.tv_usec = 1000 * (maxSystemTime % 1000);
   ares_timeout(mChannel, NULL, &tv);
   return tv.tv_sec*1000 + tv.tv_usec / 1000;
}

void
AresDns::buildFdSet(fd_set& read, fd_set& write, int& size)
{
#if defined(USE_CARES)
   // ares_fds() is deprecated.  c-ares reports every socket it opens, and
   // what it waits for, through sock_state_cb (see AresDnsPollItem), which
   // is installed with or without a FdPollGrp.
   for (PollItemsMap::const_iterator i = mPollItems.begin(); i != mPollItems.end(); ++i)
   {
      const Socket fd = i->first;
      if (i->second->mMask & FPEM_Read)
      {
         FD_SET(fd, &read);
      }
      if (i->second->mMask & FPEM_Write)
      {
         FD_SET(fd, &write);
      }
      if ((int)fd + 1 > size)
      {
         size = (int)fd + 1;
      }
   }
#else
   int newsize = ares_fds(mChannel, &read, &write);
   if ( newsize > size )
   {
      size = newsize;
   }
#endif
}

void
AresDns::processTimers()
{
#ifdef USE_CARES
   // Process timeouts only; socket events arrive via AresDnsPollItem
   ares_process_fd(mChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
#else
   resip_assert( mPollGrp!=0 );
   time_t timeSecs;
   time(&timeSecs);
   ares_process_poll(mChannel, /*server*/-1, /*rd*/-1, /*wr*/-1, timeSecs);
#endif
}

void
AresDns::process(FdSet& fdset)
{
   process(fdset.read, fdset.write);
}

void 
AresDns::buildFdSet(FdSet& fdset)
{
   buildFdSet(fdset.read, fdset.write, fdset.size);
}

void
AresDns::process(fd_set& read, fd_set& write)
{
#if defined(USE_CARES)
   // ares_process() is deprecated: hand each ready socket to
   // ares_process_fd(), then let it handle timeouts.  Collect the sockets
   // first, since processing one can close others and change mPollItems.
   std::vector<std::pair<Socket, FdPollEventMask> > ready;
   for (PollItemsMap::const_iterator i = mPollItems.begin(); i != mPollItems.end(); ++i)
   {
      FdPollEventMask mask = 0;
      if (FD_ISSET(i->first, &read))
      {
         mask |= FPEM_Read;
      }
      if (FD_ISSET(i->first, &write))
      {
         mask |= FPEM_Write;
      }
      if (mask)
      {
         ready.push_back(std::make_pair(i->first, mask));
      }
   }
   for (size_t i = 0; i < ready.size(); ++i)
   {
      ares_process_fd(mChannel,
                      (ready[i].second & FPEM_Read) ? ready[i].first : ARES_SOCKET_BAD,
                      (ready[i].second & FPEM_Write) ? ready[i].first : ARES_SOCKET_BAD);
   }
   ares_process_fd(mChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
#else
   ares_process(mChannel, &read, &write);
#endif
}

char*
AresDns::errorMessage(long errorCode)
{
   const char* aresMsg = ares_strerror(errorCode);

   size_t len = strlen(aresMsg);
   char* errorString = new char[len+1];

   snprintf(errorString, len+1, "%s", aresMsg);
   errorString[len] = '\0';
   return errorString;
}

#if defined(USE_CARES) && ARES_VERSION >= 0x011c00
// ares_query_dnsrec() gives the reply parsed; DnsStub parses the raw reply
// itself, so write it back out, as c-ares's own ares_query() does.  The
// callback runs for every query, failures included.
static void
resip_AresDns_caresDnsrecCallback(void* arg, ares_status_t status, size_t timeouts,
                                  const ares_dns_record_t* dnsrec)
{
   unsigned char* abuf = 0;
   size_t alen = 0;
   if (dnsrec)
   {
      ares_status_t writeStatus = ares_dns_write(dnsrec, &abuf, &alen);
      if (writeStatus != ARES_SUCCESS)
      {
         status = writeStatus;
      }
   }
   resip_AresDns_caresCallback(arg, (int)status, (int)timeouts, abuf, (int)alen);
   ares_free_string(abuf);
}
#endif

void
AresDns::lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData)
{
#if defined(USE_CARES) && ARES_VERSION >= 0x011c00
   // ares_query() was deprecated in c-ares 1.28, which added ares_query_dnsrec()
   ares_query_dnsrec(mChannel, target, ARES_CLASS_IN, (ares_dns_rec_type_t)type,
                     resip_AresDns_caresDnsrecCallback, new Payload(handler, userData, this), 0);
#else
   ares_query(mChannel, target, C_IN, type,
#if defined(USE_ARES)
              resip_AresDns_aresCallback,
#elif defined(USE_CARES)
              resip_AresDns_caresCallback,
#endif
              new Payload(handler, userData, this));
#endif
}

void
resip_AresDns_aresCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   resip::AresDns::Payload* p = reinterpret_cast<resip::AresDns::Payload*>(arg);

   // A destroyed channel fails its queries with ARES_EDESTRUCTION.  When the
   // AresDns itself is being destroyed, its user (DnsStub) has already let go
   // of them, so only the payload is freed.  When init() replaces the channel
   // (the DNS servers were reloaded), they are still wanted, so the failure
   // is reported like any other, as the bundled ares does.
   if (status == ARES_EDESTRUCTION && p->owner->mDestroying)
   {
      delete p;
      return;
   }

   p->handler->handleDnsRaw(resip::AresDns::makeRawResult(arg, status, abuf, alen));
   delete p;
}

void
resip_AresDns_caresCallback(void *arg, int status, int timeouts,
                       unsigned char *abuf, int alen)
{
   // Simply ignore the timeouts argument
   return ::resip_AresDns_aresCallback(arg, status, abuf, alen);
}

const char* 
AresDns::dnsRRTypeToString(int rrtype)
{
   if (rrtype == T_A)
   {
      return "A";
   }
#if defined(USE_IPV6)
   else if (rrtype == T_AAAA)
   {
      return "AAAA";
   }
#endif
   else if (rrtype == T_NAPTR)
   {
      return "NAPTR";
   }
   else if (rrtype == T_SRV)
   {
      return "SRV";
   }
   else if (rrtype == T_CNAME)
   {
      return "CNAME";
   }
   else
   {
      return "Unknown";
   }
}


/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. http://www.sipspectrum.com
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
 * vi: shiftwidth=3 expandtab:
 */
