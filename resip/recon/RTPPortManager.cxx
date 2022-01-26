
#include <rutil/ResipAssert.h>
#include "RTPPortManager.hxx"

using namespace recon;
using namespace std;

RTPPortManager::RTPPortManager(int portRangeMin, int portRangeMax)
 : mPortRangeMin(portRangeMin),
   mPortRangeMax(portRangeMax)
{
   mPortRangeMin = portRangeMin;
   mPortRangeMax = portRangeMax;
   mRTPPortFreeList.clear();
   for(unsigned int i = mPortRangeMin; i <= mPortRangeMax;)
   {
      mRTPPortFreeList.push_back(i);
      i=i+2;  // only add even ports - note we are assuming rtpPortRangeMin is even
   }
}

unsigned int
RTPPortManager::allocateRTPPort()
{
   unsigned int port = 0;
   if(!mRTPPortFreeList.empty())
   {
      port = mRTPPortFreeList.front();
      mRTPPortFreeList.pop_front();
   }
   return port;
}

void
RTPPortManager::freeRTPPort(unsigned int port)
{
   resip_assert(port >= mPortRangeMin && port <= mPortRangeMax);
   mRTPPortFreeList.push_back(port);
}


/* ====================================================================

 Copyright 2014 Daniel Pocock http://danielpocock.com

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
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
