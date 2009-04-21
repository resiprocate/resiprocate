#ifndef P2P_CHORDUPDATE_HXX
#define P2P_CHORDUPDATE_HXX

#include "rutil/Data.hxx"
#include "p2p/NodeId.hxx"
#include "p2p/MessageStructsGen.hxx"

namespace p2p
{

class ChordUpdate : private s2c::ChordUpdateStruct
{
public:
	enum UpdateType
	{
		Reserved = 0,
		PeerReady = 1,
		Neighbors = 2,
		Full = 3,
		NotSpecified = 0xff
	};
	
	ChordUpdate();

	// parses immediately
	ChordUpdate(const resip::Data &chordUpdateBody);
	
	UpdateType getUpdateType() const { return mUpdateType; }
	void setUpdateType(UpdateType updateType);
	
	// empties NodeId vectors
	void clear();
	
	// these will assert if mUpdateType is incorrect, assumes it's already ben parsed (handled in ctor)
	const std::vector<NodeId> &getFingers();
	const std::vector<NodeId> &getSuccessors();
	const std::vector<NodeId> &getPredecessors();
	
	void setFingers(const std::vector<NodeId> &fingers);  
	void setSuccessors(const std::vector<NodeId> &successors);
	void setPredecessors(const std::vector<NodeId> &predecessors);
	
	resip::Data encode(); // encodes into a blob

	bool operator == (const ChordUpdate &chord) const;
protected:
	resip::Data mUpdateBody;
	UpdateType mUpdateType;
	
	std::vector<NodeId> mPredecessors;
	std::vector<NodeId> mSuccessors;
	std::vector<NodeId> mFingers;
private:
	void parse();
	
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

