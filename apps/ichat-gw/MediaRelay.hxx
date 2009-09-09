#if !defined(MediaRelay_hxx)
#define MediaRelay_hxx 

#include <map>
#include <deque>
#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/ThreadIf.hxx>
#include <rutil/TransportType.hxx>
#include <resip/stack/Tuple.hxx>

#include "MediaRelayPort.hxx"

namespace gateway
{

class MediaRelay : public resip::ThreadIf
{      
public:
   MediaRelay(bool isV6Avail, unsigned short portRangeMin, unsigned short portRangeMax);
   virtual ~MediaRelay();

   bool createRelay(unsigned short& port);
   void destroyRelay(unsigned short port);

   void primeNextEndpoint(unsigned short& port, resip::Tuple& destinationIPPort);

protected:

private:
   virtual void thread();

   void buildFdSet(resip::FdSet& fdset);
   void checkKeepalives(MediaRelayPort* relayPort);  
   void process(resip::FdSet& fdset);
   bool processWrites(resip::FdSet& fdset, MediaRelayPort* relayPort);  // return true if all writes are complete
   void processReads(resip::FdSet& fdset, MediaRelayPort* relayPort);  

   bool createRelayImpl(unsigned short& port);
   resip::Socket createRelaySocket(resip::Tuple& tuple);

   typedef std::map<unsigned short, MediaRelayPort*> RelayPortList;
   RelayPortList mRelays;
   resip::Mutex mRelaysMutex;
   bool mIsV6Avail;

   std::deque<unsigned int> mFreeRelayPortList;
};

}

#endif  

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

