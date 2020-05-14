#include "rutil/ResipAssert.h"

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/Timer.hxx>
#include <resip/stack/Symbols.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/Random.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/ParseBuffer.hxx>
#include <resip/stack/Transport.hxx>

#include "AppSubsystem.hxx"
#include "MediaRelay.hxx"
#include <rutil/WinLeakCheck.hxx>

#include <utility>

using namespace gateway;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

#ifdef WIN32
   #define sleepMs(t) Sleep(t)
#else
   #define sleepMs(t) usleep(t*1000)
#endif

typedef struct 
{
   unsigned short versionExtPayloadTypeAndMarker;
   unsigned short sequenceNumber;	
   unsigned int timestamp;
   unsigned int ssrc;
} RtpHeader;


MediaRelay::MediaRelay(bool isV6Avail, unsigned short portRangeMin, unsigned short portRangeMax) : 
   mIsV6Avail(isV6Avail)
{   
   if(portRangeMax < portRangeMin) portRangeMax = portRangeMin;  // saftey check
   for(unsigned int i = portRangeMin; i <= portRangeMax; i++)
   {
      mFreeRelayPortList.push_back(i);
   }
}

MediaRelay::~MediaRelay()
{
   Lock lock(mRelaysMutex);
   RelayPortList::iterator it = mRelays.begin();
   for(;it != mRelays.end(); it++)
   {
      delete it->second;
   }
}

#define NUM_CREATE_TRIES 10  // Number of times to try to allocate a port, since port may be in use by another application
bool 
MediaRelay::createRelay(unsigned short& port)
{
   for(unsigned int i = 0; i < NUM_CREATE_TRIES; i++)
   {
      if(createRelayImpl(port)) return true;
   }
   return false;
}

bool 
MediaRelay::createRelayImpl(unsigned short& port)
{
   Lock lock(mRelaysMutex);

   if(mFreeRelayPortList.empty()) return false;

   unsigned short trialPort = mFreeRelayPortList.front();
   mFreeRelayPortList.pop_front();

   Tuple v4tuple(Data::Empty,trialPort,V4,UDP,Data::Empty);
   resip::Socket v4fd = createRelaySocket(v4tuple);

   if(v4fd == INVALID_SOCKET)
   {
      mFreeRelayPortList.push_back(trialPort);
      return false;
   }

   if(mIsV6Avail)
   {
      Tuple v6tuple(Data::Empty,trialPort,V6,UDP,Data::Empty);
      resip::Socket v6fd = createRelaySocket(v6tuple);

      if(v6fd != INVALID_SOCKET)
      {
         port = trialPort;

         mRelays[port] = new MediaRelayPort(v4fd, v4tuple, v6fd, v6tuple);
         InfoLog(<< "MediaRelay::createRelayImpl - Media relay started for port " << port);
   
         return true;
      }

#if defined(WIN32)
      closesocket(v4fd);
#else
      close(v4fd); 
#endif

      mFreeRelayPortList.push_back(trialPort);
      return false;
   }
   else
   {
      // Only V4 is available
      port = trialPort;

      mRelays[port] = new MediaRelayPort(v4fd, v4tuple);
      InfoLog(<< "MediaRelay::createRelayImpl - Media relay started for port " << port);
   
      return true;
   }
}

void 
MediaRelay::destroyRelay(unsigned short port)
{
   Lock lock(mRelaysMutex);
   RelayPortList::iterator it = mRelays.find(port);
   if(it != mRelays.end())
   {
      InfoLog(<< "MediaRelay::destroyRelay - port=" << port);
      delete it->second;
      mRelays.erase(it);
      mFreeRelayPortList.push_back(port);
   }
}

void 
MediaRelay::primeNextEndpoint(unsigned short& port, resip::Tuple& destinationIPPort)
{
   Lock lock(mRelaysMutex);
   RelayPortList::iterator it = mRelays.find(port);
   if(it != mRelays.end())
   {
      if(it->second->mFirstEndpoint.mTuple.getPort() == 0)
      {
         InfoLog(<< "MediaRelay::primeNextEndpoint - sender=first, port=" << port << ", addr=" << destinationIPPort);
         it->second->mFirstEndpoint.mTuple = destinationIPPort;
      }
      else
      {
         InfoLog(<< "MediaRelay::primeNextEndpoint - sender=second, port=" << port << ", addr=" << destinationIPPort);
         it->second->mSecondEndpoint.mTuple = destinationIPPort;
      }
   }
}

resip::Socket 
MediaRelay::createRelaySocket(resip::Tuple& tuple)
{
   resip::Socket fd;

#ifdef USE_IPV6
   fd = ::socket(tuple.ipVersion() == V4 ? PF_INET : PF_INET6, SOCK_DGRAM, 0);
#else
   fd = ::socket(PF_INET, SOCK_DGRAM, 0);
#endif
   
   if ( fd == INVALID_SOCKET )
   {
      int e = getErrno();
      ErrLog (<< "MediaRelay::createRelaySocket - Failed to create socket: " << strerror(e));
      return INVALID_SOCKET;
   }

   DebugLog (<< "MediaRelay::createRelaySocket - Creating fd=" << (int)fd 
             << (tuple.ipVersion() == V4 ? " V4" : " V6") 
             << ", Binding to " << Tuple::inet_ntop(tuple));
   
   if ( ::bind( fd, &tuple.getSockaddr(), tuple.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         ErrLog (<< "MediaRelay::createRelaySocket - " << tuple << " already in use ");
      }
      else
      {
         ErrLog (<< "MediaRelay::createRelaySocket - Could not bind to " << tuple << ", error=" << e);
      }
      return INVALID_SOCKET;
   }
   
   if(tuple.getPort() == 0)
   {
      // If we used port 0, then query what port the OS allocated for us
      socklen_t len = tuple.length();
      if(::getsockname(fd, &tuple.getMutableSockaddr(), &len) == SOCKET_ERROR)
      {
         int e = getErrno();
         ErrLog (<<"MediaRelay::createRelaySocket - getsockname failed, error=" << e);
         return INVALID_SOCKET;
      }
   }

   bool ok = makeSocketNonBlocking(fd);
   if ( !ok )
   {
      ErrLog (<< "MediaRelay::createRelaySocket - Could not make socket non-blocking");
      return INVALID_SOCKET;
   }  
   return fd;
}

void 
MediaRelay::thread()
{
   while (!isShutdown())
   {
      try
      {
         if(mRelays.size() == 0)
         {
            // No relays yet - just wait
            sleepMs(50);
         }
         else
         {
            resip::FdSet fdset;
            buildFdSet(fdset);            
            int ret = fdset.selectMilliSeconds(50);
            if (ret > 0)
            {
               process(fdset);
            }
         }
      }
      catch (BaseException& e)
      {
         ErrLog (<< "MediaRelay::thread - Unhandled exception: " << e);
      }
   }
   WarningLog (<< "MediaRelay::thread - shutdown");
}

void 
MediaRelay::buildFdSet(FdSet& fdset)
{ 
   Lock lock(mRelaysMutex);

   RelayPortList::iterator it = mRelays.begin();
   for(;it != mRelays.end(); it++)
   {
      // Add read fd's
      fdset.setRead(it->second->mV4Fd);
      if(mIsV6Avail) fdset.setRead(it->second->mV6Fd);
      fdset.setExcept(it->second->mV4Fd);
      if(mIsV6Avail) fdset.setExcept(it->second->mV6Fd);

      checkKeepalives(it->second);

      // Add write fd's if there is data to write
      if((it->second->mFirstEndpoint.mRelayDatagram.get() != 0 && it->second->mFirstEndpoint.mTuple.ipVersion() == V4) ||
         (it->second->mSecondEndpoint.mRelayDatagram.get() != 0 && it->second->mSecondEndpoint.mTuple.ipVersion() == V4))
      {
         fdset.setWrite(it->second->mV4Fd);
      }
      if(mIsV6Avail &&
         ((it->second->mFirstEndpoint.mRelayDatagram.get() != 0 && it->second->mFirstEndpoint.mTuple.ipVersion() == V6) ||
          (it->second->mSecondEndpoint.mRelayDatagram.get() != 0 && it->second->mSecondEndpoint.mTuple.ipVersion() == V6)))
      {
         fdset.setWrite(it->second->mV6Fd);
      }
   }
}

#define KEEPALIVETIMEOUTMS 1000 
#define KEEPALIVEMS 20  
#define STALEENDPOINTTIMEOUTMS 2000
void 
MediaRelay::checkKeepalives(MediaRelayPort* relayPort)
{
   UInt64 now = Timer::getTimeMs();

   // See if First Endpoint is stale (ie. hasn't received data in STALEENDPOINTTIMEOUTMS ms)
   if(relayPort->mFirstEndpoint.mTuple.getPort() != 0 &&
      (now - relayPort->mFirstEndpoint.mRecvTimeMs) > STALEENDPOINTTIMEOUTMS)
   {
      relayPort->mFirstEndpoint.reset();
      InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", haven't recevied data from first endpoint in " << STALEENDPOINTTIMEOUTMS << "ms - reseting endpoint.");
   }

   // See if Second Endpoint is stale (ie. hasn't received data in STALEENDPOINTTIMEOUTMS ms)
   if(relayPort->mSecondEndpoint.mTuple.getPort() != 0 &&
      (now - relayPort->mSecondEndpoint.mRecvTimeMs) > STALEENDPOINTTIMEOUTMS)
   {
      relayPort->mSecondEndpoint.reset();
      InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", haven't recevied data from second endpoint in " << STALEENDPOINTTIMEOUTMS << "ms - reseting endpoint.");
   }

   //if(relayPort->mFirstEndpoint.mTuple.getPort() != 0)
   //{
   //InfoLog(<< "MediaRelay::checkKeepalives - port=" << relayPort->mFirstEndpoint.mTuple.getPort() << ", dataToSend=" << 
   //   (relayPort->mFirstEndpoint.mRelayDatagram.get() == 0 ? "no" : "yes") << ", time since last send=" <<
   //    (now - relayPort->mFirstEndpoint.mSendTimeMs));
   //}

   // See if keepalive needs to be sent to First Endpoint
   if(relayPort->mFirstEndpoint.mTuple.getPort() != 0 &&
      relayPort->mFirstEndpoint.mRelayDatagram.get() == 0 &&
      ((!relayPort->mFirstEndpoint.mKeepaliveMode && (now - relayPort->mFirstEndpoint.mSendTimeMs) > KEEPALIVETIMEOUTMS) ||
       (relayPort->mFirstEndpoint.mKeepaliveMode && (now - relayPort->mFirstEndpoint.mSendTimeMs) > KEEPALIVEMS)))
   {
      RtpHeader keepalive;  // Create an empty G711 packet to keep iChat happy
      keepalive.versionExtPayloadTypeAndMarker = htons(0x8000);
      keepalive.sequenceNumber = 0;
      keepalive.timestamp = 0;
      keepalive.ssrc = relayPort->mFirstEndpoint.mSsrc;

      if(!relayPort->mFirstEndpoint.mKeepaliveMode)
      {
         InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", dispatching initial RTP keepalive to first sender!"); 
         relayPort->mFirstEndpoint.mKeepaliveMode = true;
      }
      //else
      //{
      //   InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", dispatching subsequent RTP keepalive to first sender!"); 
      //}

      // Add message to buffer
      std::unique_ptr<char[]> buffer(new char[sizeof(RtpHeader)]);
      memcpy(buffer.get(), &keepalive, sizeof(RtpHeader));
      
      relayPort->mFirstEndpoint.mRelayDatagram = std::move(buffer);
      relayPort->mFirstEndpoint.mRelayDatagramLen = sizeof(RtpHeader);
   }

   // See if keepalive needs to be sent to Second Endpoint
   if(relayPort->mSecondEndpoint.mTuple.getPort() != 0 &&
      relayPort->mSecondEndpoint.mRelayDatagram.get() == 0 &&
      ((!relayPort->mSecondEndpoint.mKeepaliveMode && (now - relayPort->mSecondEndpoint.mSendTimeMs) > KEEPALIVETIMEOUTMS) ||
       (relayPort->mSecondEndpoint.mKeepaliveMode && (now - relayPort->mSecondEndpoint.mSendTimeMs) > KEEPALIVEMS)))
   {
      RtpHeader keepalive;  // Create an empty G711 packet to keep iChat happy
      keepalive.versionExtPayloadTypeAndMarker = htons(0x8000);
      keepalive.sequenceNumber = 0;
      keepalive.timestamp = 0;
      keepalive.ssrc = relayPort->mSecondEndpoint.mSsrc;

      if(!relayPort->mSecondEndpoint.mKeepaliveMode)
      {
         InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", dispatching initial RTP keepalive to second sender!"); 
         relayPort->mSecondEndpoint.mKeepaliveMode = true;
      }
      //else
      //{
      //   InfoLog(<< "MediaRelay::checkKeepalives: port=" << relayPort->mLocalV4Tuple.getPort() << ", dispatching subsequent RTP keepalive to second sender!"); 
      //}

      // Add message to buffer
      std::unique_ptr<char[]> buffer(new char[sizeof(RtpHeader)]);
      memcpy(buffer.get(), &keepalive, sizeof(RtpHeader));
      
      relayPort->mSecondEndpoint.mRelayDatagram = std::move(buffer);
      relayPort->mSecondEndpoint.mRelayDatagramLen = sizeof(RtpHeader);
   }
}

void 
MediaRelay::process(FdSet& fdset)
{
   Lock lock(mRelaysMutex);

   RelayPortList::iterator it = mRelays.begin();
   for(;it != mRelays.end(); it++)
   {
      if(processWrites(fdset, it->second))
      {
         // If all writes have been processed, process reads
         processReads(fdset, it->second);
      }
   }
}

bool
MediaRelay::processWrites(FdSet& fdset, MediaRelayPort* relayPort)
{
   resip::Socket fd = INVALID_SOCKET;
   Tuple tuple;
   std::unique_ptr<char[]> buffer;
   int len;

   // If we have data to write to first sender then check if readyToWrite
   if(relayPort->mFirstEndpoint.mRelayDatagram.get() != 0)
   {
      if(relayPort->mFirstEndpoint.mTuple.ipVersion() == V4 &&
         fdset.readyToWrite(relayPort->mV4Fd))
      {
         fd = relayPort->mV4Fd;
         tuple = relayPort->mFirstEndpoint.mTuple;
         buffer = std::move(relayPort->mFirstEndpoint.mRelayDatagram);
         len = relayPort->mFirstEndpoint.mRelayDatagramLen;               
      }
      else if(mIsV6Avail &&
              relayPort->mFirstEndpoint.mTuple.ipVersion() == V6 &&
              fdset.readyToWrite(relayPort->mV6Fd))
      {
         fd = relayPort->mV6Fd;
         tuple = relayPort->mFirstEndpoint.mTuple;
         buffer = std::move(relayPort->mFirstEndpoint.mRelayDatagram);
         len = relayPort->mFirstEndpoint.mRelayDatagramLen;               
      }
   }

   // If anything to send to first sender then do it
   if (fd != INVALID_SOCKET)
   {
      int count;
      count = sendto(fd, 
                     buffer.get(), 
                     len,  
                     0, // flags
                     &tuple.getMutableSockaddr(), tuple.length());
      if ( count == SOCKET_ERROR )
      {
         int e = getErrno();
         InfoLog (<< "MediaRelay::processWrites: port=" << relayPort->mLocalV4Tuple.getPort() << ", Failed (" << e << ") sending to " << tuple);
      }
      else
      {
         // InfoLog(<< len << " bytes of data sent to " << tuple);
         relayPort->mFirstEndpoint.mSendTimeMs = Timer::getTimeMs();
      }
   }

   // check if we have data to write to second sender then check if readyToWrite
   fd = INVALID_SOCKET; // reset
   if(relayPort->mSecondEndpoint.mRelayDatagram.get() != 0)
   {
      if(relayPort->mSecondEndpoint.mTuple.ipVersion() == V4 &&
         fdset.readyToWrite(relayPort->mV4Fd))
      {
         fd = relayPort->mV4Fd;
         tuple = relayPort->mSecondEndpoint.mTuple;
         buffer = std::move(relayPort->mSecondEndpoint.mRelayDatagram);
         len = relayPort->mSecondEndpoint.mRelayDatagramLen;               
      }
      else if(mIsV6Avail && 
              relayPort->mSecondEndpoint.mTuple.ipVersion() == V6 &&
              fdset.readyToWrite(relayPort->mV6Fd))
      {
         fd = relayPort->mV6Fd;
         tuple = relayPort->mSecondEndpoint.mTuple;
         buffer = std::move(relayPort->mSecondEndpoint.mRelayDatagram);
         len = relayPort->mSecondEndpoint.mRelayDatagramLen;               
      }
   }

   // If anything to send to sender sender then do it
   if (fd != INVALID_SOCKET)
   {
      int count;
      count = sendto(fd, 
                     buffer.get(), 
                     len,  
                     0, // flags
                     &tuple.getMutableSockaddr(), tuple.length());
      if ( count == SOCKET_ERROR )
      {
         int e = getErrno();
         InfoLog (<< "MediaRelay::processWrites: port=" << relayPort->mLocalV4Tuple.getPort() << ", Failed (" << e << ") sending to " << tuple);
      }
      else
      {
         //InfoLog(<< len << " bytes of data sent to " << tuple);
         relayPort->mSecondEndpoint.mSendTimeMs = Timer::getTimeMs();
      }
   }

   return relayPort->mFirstEndpoint.mRelayDatagram.get() == 0 &&
          relayPort->mSecondEndpoint.mRelayDatagram.get() == 0;
}

#define UDP_BUFFER_SIZE 1000
void 
MediaRelay::processReads(FdSet& fdset, MediaRelayPort* relayPort)
{
   resip::Socket fd = INVALID_SOCKET;
   Tuple tuple;

   if(fdset.readyToRead(relayPort->mV4Fd))
   {
      //InfoLog(<<"V4 socket is ready to read.");
      fd = relayPort->mV4Fd;
      tuple = relayPort->mLocalV4Tuple;
   }
   else if(mIsV6Avail && fdset.readyToRead(relayPort->mV6Fd))
   {
      //InfoLog(<<"V6 socket is ready to read.");
      fd = relayPort->mV6Fd;
      tuple = relayPort->mLocalV6Tuple;
   }
   if (fd != INVALID_SOCKET)
   {
      std::unique_ptr<char[]> buffer(new char[UDP_BUFFER_SIZE+1]);

      socklen_t slen = tuple.length();
      int len = recvfrom( fd,
                          buffer.get(),
                          UDP_BUFFER_SIZE,
                          0 /*flags */,
                          &tuple.getMutableSockaddr(), 
                          &slen);
      if ( len == SOCKET_ERROR )
      {
         int err = getErrno();
         if ( err != EWOULDBLOCK  )
         {
            ErrLog (<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", Error calling recvfrom: " << err);
         }
         buffer.reset();
      }

      if (len == 0)
      {
         ErrLog (<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", No data calling recvfrom: len=" << len);
         buffer.reset();
      }

      if (len+1 >= UDP_BUFFER_SIZE)
      {
         InfoLog(<<"MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", Datagram exceeded max length "<<UDP_BUFFER_SIZE);
         buffer.reset();
      }

      if(buffer.get() != 0)
      {
         UInt64 now = Timer::getTimeMs();
         RtpHeader* rtpHeader = (RtpHeader*)buffer.get();
         //InfoLog(<< "Received a datagram of size=" << len << " from=" << tuple);
         MediaEndpoint* pReceivingEndpoint = 0;
         MediaEndpoint* pSendingEndpoint = 0;

         // First check if packet is from first endpoint
         if(tuple == relayPort->mFirstEndpoint.mTuple)
         {
            pReceivingEndpoint = &relayPort->mFirstEndpoint;
            pSendingEndpoint = &relayPort->mSecondEndpoint;
         }
         // Next check if packet is from second endpoint
         else if(tuple == relayPort->mSecondEndpoint.mTuple)
         {
            pReceivingEndpoint = &relayPort->mSecondEndpoint;
            pSendingEndpoint = &relayPort->mFirstEndpoint;
         }
         else
         {
            // See if we can store this new sender in First Endpoint
            if(relayPort->mFirstEndpoint.mTuple.getPort() == 0)
            {
               InfoLog(<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", First packet received from First Endpoint " << tuple);
               pReceivingEndpoint = &relayPort->mFirstEndpoint;
               pSendingEndpoint = &relayPort->mSecondEndpoint;
            }
            else if(relayPort->mSecondEndpoint.mTuple.getPort() == 0)
            {
               InfoLog(<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", First packet received from Second Endpoint " << tuple);
               pReceivingEndpoint = &relayPort->mSecondEndpoint;
               pSendingEndpoint = &relayPort->mFirstEndpoint;
            }
            else  // We already have 2 endpoints - this would be a third
            {
               // We have a third sender - for now drop, if one of the other senders stops sending data for 2 seconds then we will start picking up this sender
               WarningLog(<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", MediaRelay on " << relayPort->mLocalV4Tuple.getPort() << " has seen a third sender " << tuple << " - not implemented yet - ignoring packet");
            }
            if(pReceivingEndpoint)
            {
               pReceivingEndpoint->mTuple = tuple;
               pReceivingEndpoint->mSendTimeMs = now;
               pReceivingEndpoint->mRecvTimeMs = now;
            }
         }

         if(pReceivingEndpoint)
         {
            pReceivingEndpoint->mRecvTimeMs = now;
            if(pSendingEndpoint && pSendingEndpoint->mTuple.getPort() != 0)
            {
               if(ntohs(rtpHeader->versionExtPayloadTypeAndMarker) & 0x8000)  // RTP Version 2
               {
                  // Adjust ssrc
                  rtpHeader->ssrc = pSendingEndpoint->mSsrc;

                  // relay packet to second sender
                  resip_assert(pSendingEndpoint->mRelayDatagram.get() == 0);
                  pSendingEndpoint->mRelayDatagram = std::move(buffer);
                  pSendingEndpoint->mRelayDatagramLen = len;
                  if(pSendingEndpoint->mKeepaliveMode)
                  {
                     InfoLog(<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", received packet to forward, turning off keepalive mode for " << pSendingEndpoint->mTuple);
                     pSendingEndpoint->mKeepaliveMode = false;
                  }

                  //InfoLog(<< "Relaying packet from=" << tuple << " to " << pSendingEndpoint->mTuple);
               }
               //else
               //{
               //   InfoLog(<< "MediaRelay::processReads: port=" << relayPort->mLocalV4Tuple.getPort() << ", discarding received packet with unknown RTP version from " << tuple);
               //}
            }
         }
      }
   }
}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

