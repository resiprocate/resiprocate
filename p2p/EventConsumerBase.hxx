#ifndef P2P_EventConsumerBase_hxx
#define P2P_EventConsumerBase_hxx

#include "rutil/ResipAssert.h"

#include "p2p/Postable.hxx"

namespace p2p
{

class BatchMessages;
class CertDoneEvent;
class ConnectionOpened;
class ConnectionClosed;
class Event;
class MessageArrived;
class ApplicationMessageArrived;
class LocalCandidatesCollected;

class PingReq;
class PingAns;
class ConnectReq;
class ConnectAns;
class TunnelReq;
class TunnelAns;
class StoreReq;
class StoreAns;
class FetchReq;
class FetchAns;
class RemoveReq;
class RemoveAns;
class FindReq;
class FindAns;
class JoinReq;
class JoinAns;
class LeaveReq;
class LeaveAns;
class UpdateReq;
class UpdateAns;
class RouteQueryReq;
class RouteQueryAns;
class ErrorResponse;

template<class T> class EventWrapper;

/**
 * Base class for consuming double dispatced events.
 * Events can be sent to the consumer as a Postable.
 */
class EventConsumerBase : public Postable<Event>
{
   public:
      virtual ~EventConsumerBase(){};

      virtual void consume(CertDoneEvent& certdone) = 0;
      virtual void consume(ConnectionOpened& m) = 0;
      virtual void consume(ConnectionClosed& m) = 0;
      virtual void consume(MessageArrived& m) = 0;
      virtual void consume(ApplicationMessageArrived& m) = 0;
      virtual void consume(LocalCandidatesCollected& m) = 0;

      virtual void consume(PingReq& m) = 0;
      virtual void consume(PingAns& m) = 0;
      virtual void consume(ConnectReq& m) = 0;
      virtual void consume(ConnectAns& m) = 0;
      virtual void consume(TunnelReq& m) = 0;
      virtual void consume(TunnelAns& m) = 0;
      virtual void consume(StoreReq& m) = 0;
      virtual void consume(StoreAns& m) = 0;
      virtual void consume(FetchReq& m) = 0;
      virtual void consume(FetchAns& m) = 0;
      virtual void consume(RemoveReq& m) = 0;
      virtual void consume(RemoveAns& m) = 0;
      virtual void consume(FindAns& m) = 0;
      virtual void consume(JoinReq& m) = 0;
      virtual void consume(JoinAns& m) = 0;
      virtual void consume(LeaveReq& m) = 0;
      virtual void consume(LeaveAns& m) = 0;
      virtual void consume(UpdateReq& m) = 0;
      virtual void consume(UpdateAns& m) = 0;
      virtual void consume(RouteQueryReq& m) = 0;
      virtual void consume(RouteQueryAns& m) = 0;
      virtual void consume(ErrorResponse& m) = 0;

      virtual void consume(const BatchMessages& cm) = 0;
};

}

#endif

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
