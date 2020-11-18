#ifndef P2P_UPDATE_HXX
#define P2P_UPDATE_HXX

#include "p2p/Message.hxx"
#include "p2p/EventWrapper.hxx"

namespace p2p
{

class UpdateReq;

class UpdateAns : public Message
{
public:
	virtual MessageType getType() const { return Message::UpdateAnsType; }
	virtual void getEncodedPayload(resip::DataStream &data);
	virtual resip::Data brief() const { return "UpdateAns Message"; }


	std::unique_ptr<Event> event()
	{
		return wrap(this);
	}
protected:
	resip::Data mOverlaySpecificData;

	friend class Message;

	virtual void decodePayload(resip::DataStream &dataStream);

	UpdateAns();
	UpdateAns(UpdateReq *request, const resip::Data &overlaySpecificData);
};


class UpdateReq : public Message
{
public:
	UpdateReq(const DestinationId &dest, const resip::Data &overlaySpecificBlob);
	virtual MessageType getType() const { return Message::UpdateReqType; }

	virtual void getEncodedPayload(resip::DataStream &data);
	virtual resip::Data brief() const { return "UpdateReq Message"; }

	std::unique_ptr<Event> event()
	{
		return wrap(this);
	}

protected:
	friend class Message;

	resip::Data mOverlaySpecificData;

	virtual void decodePayload(resip::DataStream &dataStream);
	UpdateReq();
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
