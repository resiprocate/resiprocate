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

#if !defined(WIN32)
#if !defined(__CYGWIN__)
#include <arpa/nameser.h>
#endif
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
       mChannel(chan), mFd(fd), mUsingTcp(using_tcp), mServerIdx(server_idx)
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
         mPollHandle = mPollGrp->addPollItem(mFd, FPEM_Read, this);
      }
   }

   AresDns&	mAres;
   ares_channel	mChannel;
   int mFd;
   int mUsingTcp;
   int mServerIdx;

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
      grp->modPollItem(olditem->mPollHandle, FPEM_Read|FPEM_Write);
      break;
   case ARES_POLLACTION_WRITEOFF:
      resip_assert( olditem );
      resip_assert( it != ares->mPollItems.end() );
      grp->modPollItem(olditem->mPollHandle, FPEM_Read);
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
   AresDnsPollItem(FdPollGrp *grp, int fd, FdPollEventMask mask,
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
         mPollGrp->modPollItem(mPollHandle, mMask);
      }
   }

   ares_channel mChannel;
   FdPollEventMask mMask;

   static void sockstate_cb(void* data, ares_socket_t sock, int read, int write);
};

};

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
        mPollGrp);

    if (ret != Success)
        return ret;

   if ( mPollGrp )
   {
      // Ensure map starts empty, since init may be called more than once
      clearPollItems();
   }

#ifdef WIN32
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
#if defined(USE_ARES)
      // Sends ARES_EDESTRUCTION to callbacks so that all requests in progress fail
      ares_destroy(*channel);
#elif defined(USE_CARES)
      // Callbacks will be supressed by looking for the ARES_EDESTRUCTION
      // sentinel status
      ares_destroy(*channel);
#endif
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
      opt.sock_state_cb = AresDnsPollItem::sockstate_cb;
      opt.sock_state_cb_data = this;
      optmask |= ARES_OPT_SOCK_STATE_CB;
   }
#endif

   if (additionalNameservers.empty())
   {
#if defined(USE_ARES)
      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);
#elif defined(USE_CARES)
      // TODO: Does the socket function matter?
      status = ares_init_options(channel, &opt, optmask);
#endif
   }
   else
   {
      optmask |= ARES_OPT_SERVERS;
      opt.nservers = (int)additionalNameservers.size();

#if defined(USE_IPV6) && defined(USE_ARES)
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
      // If we're only supporting IPv4 or we are using c-ares, we can't
      // support additional nameservers that are IPv6 right now.
      opt.servers = new in_addr[additionalNameservers.size()];
      for (size_t i =0; i < additionalNameservers.size(); i++)
      {
         if(additionalNameservers[i].isVersion4())
         {
            opt.servers[i] = additionalNameservers[i].v4Address.sin_addr;
         }
         else
         {
#if defined(USE_CARES)
           WarningLog (<< "Ignoring non-IPv4 additional name server (not yet supported with c-ares)");
#elif defined(USE_ARES)
           WarningLog (<< "Ignoring non-IPv4 additional name server (IPv6 support was not enabled)");
#endif
         }
      }
#endif

#if defined(USE_ARES)
      status = ares_init_options_with_socket_function(channel, &opt, optmask, socketfunc);
#elif defined(USE_CARES)
      // TODO: Does the socket function matter?
      status = ares_init_options(channel, &opt, optmask);
#endif

      delete [] opt.servers;
      opt.servers = 0;
   }
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
         // tell ares to let us know when things change
         ares_process_set_poll_cb(*channel, AresDnsPollItem::socket_poll_cb, this);
      }

#elif defined(USE_CARES)
      {
         // Log which version of c-ares we're using
         InfoLog(<< "DNS initialization: using c-ares v"
                 << ::ares_version(NULL));

         // Ask for the current configuration so we can print the servers found
         struct ares_options options;
         std::memset(&options, 0, sizeof(options));
         int ignored;
         if(ares_save_options(*channel, &options, &ignored) == ARES_SUCCESS)
         {
            InfoLog(<< "DNS initialization: found "
                    << options.nservers << " name servers");

            // Log them all
            for (int i = 0; i < options.nservers; ++i)
            {
               InfoLog(<< " name server: "
                       << DnsUtil::inet_ntop(options.servers[i]));
            }
            ares_destroy_options(&options);
         }
      }
#endif

      return Success;
   }
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
      // Get the options, including the server list, from the old and the
      // current (i.e. just read) configuration.
      struct ares_options old;
      struct ares_options updated;
      std::memset(&old, 0, sizeof(old));
      std::memset(&updated, 0, sizeof(updated));
      int ignored;

      // Can we get the configuration?
      if(ares_save_options(mChannel, &old, &ignored) != ARES_SUCCESS
         || ares_save_options(channel, &updated, &ignored) != ARES_SUCCESS)
      {
         // It failed, so call it different
         bRet = true;
      }
      else
      {
         // Compare the two lists.  Are they different sizes?
         if(old.nservers != updated.nservers)
         {
            // Yes, so they're different
            bRet = true;
         }
         else
         {
            // Compare them one-by-one
            for (int i = 0; i < old.nservers; ++i)
            {
               if (old.servers[i].s_addr != updated.servers[i].s_addr)
               {
                  bRet = true;
                  break;
               }
            }
         }

         // Free any ares_options contents we have created.
         ares_destroy_options(&old);
         ares_destroy_options(&updated);
      }

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
#if defined(USE_ARES)
   ares_destroy_suppress_callbacks(mChannel);
#elif defined(USE_CARES)
   ares_destroy(mChannel);
#endif
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
   Payload* p = reinterpret_cast<Payload*>(arg);
   ExternalDnsHandler *thisp = reinterpret_cast<ExternalDnsHandler*>(p->first);
   return thisp;
}

ExternalDnsRawResult
AresDns::makeRawResult(void *arg, int status, unsigned char *abuf, int alen)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   void* userArg = reinterpret_cast<void*>(p->second);

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
   int newsize = ares_fds(mChannel, &read, &write);
   if ( newsize > size )
   {
      size = newsize;
   }
}

void
AresDns::processTimers()
{
   resip_assert( mPollGrp!=0 );
#ifdef USE_CARES
   ares_process_fd(mChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
#else
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
   ares_process(mChannel, &read, &write);
}

char*
AresDns::errorMessage(long errorCode)
{
   const char* aresMsg = ares_strerror(errorCode);

   size_t len = strlen(aresMsg);
   char* errorString = new char[len+1];

   strncpy(errorString, aresMsg, len);
   errorString[len] = '\0';
   return errorString;
}

void
AresDns::lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData)
{
   ares_query(mChannel, target, C_IN, type,
#if defined(USE_ARES)
              resip_AresDns_aresCallback,
#elif defined(USE_CARES)
              resip_AresDns_caresCallback,
#endif
              new Payload(handler, userData));
}

void
resip_AresDns_aresCallback(void *arg, int status, unsigned char *abuf, int alen)
{
#if defined(USE_CARES)
   // If this is destruction, skip it.  We do this here for completeness.
   if(status == ARES_EDESTRUCTION)
   {
      return;
   }
#endif

   resip::AresDns::getHandler(arg)->handleDnsRaw(resip::AresDns::makeRawResult(arg, status, abuf, alen));
   resip::AresDns::Payload* p = reinterpret_cast<resip::AresDns::Payload*>(arg);
   delete p;
}

void
resip_AresDns_caresCallback(void *arg, int status, int timeouts,
                       unsigned char *abuf, int alen)
{
   // Simply ignore the timeouts argument
   return ::resip_AresDns_aresCallback(arg, status, abuf, alen);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
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
