#include "p2p/TransporterMessage.hxx"
#include "p2p/EventConsumer.hxx"

using namespace p2p;
ConnectionOpened::ConnectionOpened(FlowId flowId,
                                   unsigned short application,
                                   resip::TransportType transportType,
                                   bool inbound,
                                   X509 *cert) : 
   mFlowId(flowId),
   mApplication(application),
   mTransportType(transportType),
   mInbound(inbound),
   mCert(cert)
{
}

ConnectionOpened::~ConnectionOpened()
{
}

void
ConnectionOpened::dispatch(EventConsumer& consumer)
{
   consumer.consume(*this);
}

FlowId 
ConnectionOpened::getFlowId() const
{
   return mFlowId;
}

unsigned short
ConnectionOpened::getApplication() const
{
   return mApplication;
}

NodeId
ConnectionOpened::getNodeId() const
{
   return mFlowId.getNodeId();
}

resip::TransportType
ConnectionOpened::getTransportType() const
{
   return mTransportType;
}

bool
ConnectionOpened::isInbound() const
{
   return mInbound;
}

X509*
ConnectionOpened::getCertificate() const
{
   return mCert;
}

ConnectionClosed::ConnectionClosed(FlowId flowId, unsigned short appId) : 
   mFlowId(flowId),
   mApplication(appId)
{
}

NodeId
ConnectionClosed::getNodeId() const
{
   return mFlowId.getNodeId();
}

unsigned short
ConnectionClosed::getApplicationId() const
{
   return mApplication;
}

void
ConnectionClosed::dispatch(EventConsumer& consumer)
{
   consumer.consume(*this);
}

ConnectionClosed::~ConnectionClosed()
{
}


void
MessageArrived::dispatch(EventConsumer& consumer)
{
   consumer.consume(*this);
}

MessageArrived::~MessageArrived()
{
}


void
ApplicationMessageArrived::dispatch(EventConsumer& consumer)
{
   consumer.consume(*this);
}

ApplicationMessageArrived::~ApplicationMessageArrived()
{
}


void
LocalCandidatesCollected::dispatch(EventConsumer& consumer)
{
   consumer.consume(*this);
}

LocalCandidatesCollected::~LocalCandidatesCollected()
{
}


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
