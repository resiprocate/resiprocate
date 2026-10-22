#include <algorithm>
#include <functional>

#include "ConnectionManager.hxx"
#include "TurnManager.hxx"

namespace reTurn {

void 
ConnectionManager::start(ConnectionPtr c, const asio::ip::address& remoteAddress)
{
  if(!mTurnManager.addConnection(remoteAddress))
  {
     c->stop();
     return;
  }
  mConnections[c] = remoteAddress;
  c->start();
}

void 
ConnectionManager::stop(ConnectionPtr c)
{
  std::map<ConnectionPtr, asio::ip::address>::iterator it = mConnections.find(c);
  if(it != mConnections.end())
  {
     mTurnManager.removeConnection(it->second);
     mConnections.erase(it);
  }
  c->stop();
}

void 
ConnectionManager::stopAll()
{
   std::map<ConnectionPtr, asio::ip::address>::iterator it = mConnections.begin();

   for(; it != mConnections.end(); it++)
   {
      mTurnManager.removeConnection(it->second);
      it->first->stop();
   }
   mConnections.clear();
}

} 


/* ====================================================================

 Copyright (c) 2008-2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
