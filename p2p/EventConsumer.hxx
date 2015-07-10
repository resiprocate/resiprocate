#ifndef P2P_EventConsumer_hxx
#define P2P_EventConsumer_hxx

#include "rutil/ResipAssert.h"

#include "p2p/EventConsumerBase.hxx"

namespace p2p
{

class EventConsumer : public EventConsumerBase
{
   public:
      virtual ~EventConsumer(){};

      virtual void consume(CertDoneEvent& certdone) { resip_assert(0); }
      virtual void consume(ConnectionOpened& m) { resip_assert(0); }
      virtual void consume(ConnectionClosed& m) { resip_assert(0); }
      virtual void consume(MessageArrived& m) { resip_assert(0); }
      virtual void consume(ApplicationMessageArrived& m) { resip_assert(0); }
      virtual void consume(LocalCandidatesCollected& m) { resip_assert(0); }
      
      virtual void consume(PingReq& m) { resip_assert(0); }
      virtual void consume(PingAns& m) { resip_assert(0); }
      virtual void consume(ConnectReq& m) { resip_assert(0); }
      virtual void consume(ConnectAns& m) { resip_assert(0); }
      virtual void consume(TunnelReq& m) { resip_assert(0); }
      virtual void consume(TunnelAns& m) { resip_assert(0); }
      virtual void consume(StoreReq& m) { resip_assert(0); }
      virtual void consume(StoreAns& m) { resip_assert(0); }
      virtual void consume(FetchReq& m) { resip_assert(0); }
      virtual void consume(FetchAns& m) { resip_assert(0); }
      virtual void consume(RemoveReq& m) { resip_assert(0); }
      virtual void consume(RemoveAns& m) { resip_assert(0); }
      virtual void consume(FindAns& m) { resip_assert(0); }
      virtual void consume(JoinReq& m) { resip_assert(0); }
      virtual void consume(JoinAns& m) { resip_assert(0); }
      virtual void consume(LeaveReq& m) { resip_assert(0); }
      virtual void consume(LeaveAns& m) { resip_assert(0); }
      virtual void consume(UpdateReq& m) { resip_assert(0); }
      virtual void consume(UpdateAns& m) { resip_assert(0); }
      virtual void consume(RouteQueryReq& m) { resip_assert(0); }
      virtual void consume(RouteQueryAns& m) { resip_assert(0); }
      virtual void consume(ErrorResponse& m) { resip_assert(0); }

      virtual void consume(const BatchMessages& cm) { resip_assert(0); }

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
