#if !defined(P2P_JOIN_HXX)
#define P2P_JOIN_HXX

#include "p2p/Message.hxx"
#include "p2p/EventWrapper.hxx"

namespace p2p 
{

class JoinReq;

class JoinAns : public Message, private s2c::JoinAnsStruct
{
public:
    friend class Message;
    
    JoinAns(p2p::JoinReq *request, const resip::Data &overlaySpecific = resip::Data::Empty);
    virtual MessageType getType() const { return Message::JoinAnsType; }
    
    virtual void getEncodedPayload(resip::DataStream &data);
    virtual resip::Data brief() const { return "JoinAns Message"; }
    
    std::auto_ptr<Event> event() {return wrap(this);}

protected:
	virtual void decodePayload(resip::DataStream &dataStream);
	JoinAns();
};

class JoinReq : public Message, private s2c::JoinReqStruct
{
public:
	JoinReq(const DestinationId &dest, const NodeId &nodeId, const resip::Data &overlaySpecific=resip::Data::Empty);
      
	virtual MessageType getType() const { return Message::JoinReqType; }
	NodeId getNodeId() const;
      
	virtual void getEncodedPayload(resip::DataStream &data);
	virtual resip::Data brief() const { return "JoinReq Message"; }

    std::auto_ptr<Event> event() {return wrap(this);}

protected:
	virtual void decodePayload(resip::DataStream &dataStream);
	JoinReq();

	NodeId mNodeId;
	resip::Data mOverlaySpecific;

	friend class Message;
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
