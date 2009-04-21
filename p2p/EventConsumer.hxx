#ifndef P2P_EventConsumer_hxx
#define P2P_EventConsumer_hxx

#include <cassert>

#include "p2p/EventConsumerBase.hxx"

namespace p2p
{

class EventConsumer : public EventConsumerBase
{
   public:
      virtual ~EventConsumer(){};

      virtual void consume(CertDoneEvent& certdone) { assert(0); }
      virtual void consume(ConnectionOpened& m) { assert(0); }
      virtual void consume(ConnectionClosed& m) { assert(0); }
      virtual void consume(MessageArrived& m) { assert(0); }
      virtual void consume(ApplicationMessageArrived& m) { assert(0); }
      virtual void consume(LocalCandidatesCollected& m) { assert(0); }
      
      virtual void consume(PingReq& m) { assert(0); }
      virtual void consume(PingAns& m) { assert(0); }
      virtual void consume(ConnectReq& m) { assert(0); }
      virtual void consume(ConnectAns& m) { assert(0); }
      virtual void consume(TunnelReq& m) { assert(0); }
      virtual void consume(TunnelAns& m) { assert(0); }
      virtual void consume(StoreReq& m) { assert(0); }
      virtual void consume(StoreAns& m) { assert(0); }
      virtual void consume(FetchReq& m) { assert(0); }
      virtual void consume(FetchAns& m) { assert(0); }
      virtual void consume(RemoveReq& m) { assert(0); }
      virtual void consume(RemoveAns& m) { assert(0); }
      virtual void consume(FindAns& m) { assert(0); }
      virtual void consume(JoinReq& m) { assert(0); }
      virtual void consume(JoinAns& m) { assert(0); }
      virtual void consume(LeaveReq& m) { assert(0); }
      virtual void consume(LeaveAns& m) { assert(0); }
      virtual void consume(UpdateReq& m) { assert(0); }
      virtual void consume(UpdateAns& m) { assert(0); }
      virtual void consume(RouteQueryReq& m) { assert(0); }
      virtual void consume(RouteQueryAns& m) { assert(0); }
      virtual void consume(ErrorResponse& m) { assert(0); }

      virtual void consume(const BatchMessages& cm) { assert(0); }

      virtual resip::Data brief() const { return "EventConsumer"; }

};

}

#endif // P2P_EventConsumer_hxx

/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
