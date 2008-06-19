#ifndef __P2P_FORWARDINGLAYER_HXX
#define __P2P_FORWARDINGLAYER_HXX 1

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Transporter.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/TopologyAPI.hxx"
#include "p2p/Message.hxx"

namespace p2p
{
class Dispatcher;
class TransporterMessage;

class ForwardingLayer: public EventConsumer
{
   public:
      ForwardingLayer(const Profile& profile, 
                      Dispatcher& dispatcher, 
                      Transporter& transporter,
                      TopologyAPI& topology)
         : mProfile(profile),
           mDispatcher(dispatcher),
           mTransporter(transporter),
           mTopology(topology) 
           {
              mTransporter.setRxFifo(&mRxFifo);
           }

      void process(int ms);

      // all the consumes methods are for messages coming up from transports to this
      virtual void consume(ConnectionOpened& m);
      virtual void consume(ConnectionClosed& m);
      virtual void consume(MessageArrived& m);
      virtual void consume(ApplicationMessageArrived& m);
      virtual void consume(LocalCandidatesCollected& m);

      // from messages from above or below that need to be forwarded 
      void forward( std::auto_ptr<Message> m );

      //not public api
      virtual void post(std::auto_ptr<Event> event);
      
      virtual resip::Data brief() const
      {
         return "ForwardingLayer";
      }
      

   private:
      const Profile& mProfile;
      Dispatcher& mDispatcher;
      Transporter& mTransporter;
      TopologyAPI& mTopology;

      resip::Fifo<Event> mRxFifo;
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
