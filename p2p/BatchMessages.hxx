#ifndef P2P_BatchMessages_hxx
#define P2P_BatchMessages_hxx

#include <memory>
#include <vector>

#include "EventConsumerBase.hxx"
#include "Event.hxx"

namespace p2p
{

class Dispatcher;
class Message;

class BatchMessages : public EventConsumerBase, public Event
{
      BatchMessages(Dispatcher& dispatcher,
                    std::vector<std::auto_ptr<Message> >& messages,
                    Postable<Event>& postable);
      virtual ~BatchMessages() = 0;
      
      virtual void onFailure() = 0;
      virtual void onSuccess() = 0;
      // called by creator on consumption
      void completed();

      virtual void consume(PingAns& m);
      virtual void consume(ConnectAns& m);
      virtual void consume(TunnelAns& m);
      virtual void consume(StoreAns& m);
      virtual void consume(FetchAns& m);
      virtual void consume(RemoveAns& m);
      virtual void consume(FindAns& m);
      virtual void consume(JoinAns& m);
      virtual void consume(LeaveAns& m);
      virtual void consume(UpdateAns& m);
      virtual void consume(RouteQueryAns& m);
      virtual void consume(ErrorResponse& m);

      // other consumer methods
      virtual void consume(CertDoneEvent& certdone) { resip_assert(0); }
      virtual void consume(ConnectionOpened& m) { resip_assert(0); }
      virtual void consume(ConnectionClosed& m) { resip_assert(0); }
      virtual void consume(MessageArrived& m) { resip_assert(0); }
      virtual void consume(ApplicationMessageArrived& m) { resip_assert(0); }
      virtual void consume(LocalCandidatesCollected& m) { resip_assert(0); }
      virtual void consume(PingReq& m) { resip_assert(0); }
      virtual void consume(ConnectReq& m) { resip_assert(0); }
      virtual void consume(TunnelReq& m) { resip_assert(0); }
      virtual void consume(StoreReq& m) { resip_assert(0); }
      virtual void consume(FetchReq& m) { resip_assert(0); }
      virtual void consume(RemoveReq& m) { resip_assert(0); }
      virtual void consume(JoinReq& m) { resip_assert(0); }
      virtual void consume(LeaveReq& m) { resip_assert(0); }
      virtual void consume(UpdateReq& m) { resip_assert(0); }
      virtual void consume(RouteQueryReq& m) { resip_assert(0); }
      virtual void consume(const BatchMessages& cm) { resip_assert(0); }

      virtual resip::Data brief() const
      { 
         return "BatchMessages"; 
      }

   private:
      void countDown(Message& m);
      Postable<Event>* mPostable;
      int mResponseCount;
      bool mSucceeded;
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
