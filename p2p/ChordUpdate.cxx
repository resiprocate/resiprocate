#include "p2p/ChordUpdate.hxx"
#include "rutil/ResipAssert.h"

using namespace p2p;
using namespace s2c;

ChordUpdate::ChordUpdate() :	
	mUpdateType(NotSpecified)
{
	
}

ChordUpdate::ChordUpdate(const resip::Data &chordUpdateBody) :
	mUpdateBody(chordUpdateBody),
	mUpdateType(NotSpecified)
{
	parse();
}

bool 
ChordUpdate::operator == (const ChordUpdate &chord) const
{
	return 
		(mUpdateType == chord.mUpdateType) &&
		(mPredecessors == chord.mPredecessors) &&
		(mSuccessors == chord.mSuccessors) &&
		(mFingers == chord.mFingers);
}

void 
ChordUpdate::setUpdateType(UpdateType updateType)
{
	mUpdateType = updateType;
}

void 
ChordUpdate::clear()
{
	mPredecessors.clear();
	mSuccessors.clear();
	mFingers.clear();
}

const std::vector<NodeId> &
ChordUpdate::getFingers()
{
	resip_assert(mUpdateType == Full);
	return mFingers;
}

const std::vector<NodeId> &
ChordUpdate::getSuccessors()
{
	resip_assert(mUpdateType == Neighbors || mUpdateType == Full);
	return mSuccessors;
}

const std::vector<NodeId> &
ChordUpdate::getPredecessors()
{
	resip_assert(mUpdateType == Neighbors || mUpdateType == Full);
	return mPredecessors;	
}

void 
ChordUpdate::setFingers(const std::vector<NodeId> &fingers)
{
	mFingers = fingers;
}

void 
ChordUpdate::setSuccessors(const std::vector<NodeId> &successors)
{
	mSuccessors = successors;
}

void 
ChordUpdate::setPredecessors(const std::vector<NodeId> &predecessors)
{
	mPredecessors = predecessors;
}

resip::Data
ChordUpdate::encode()
{
	resip::Data encodedData;

	{
		resip::DataStream encodedStream(encodedData);

		mType = static_cast<ChordUpdateType>(mUpdateType);
		s2c::ChordUpdateStruct::encode(encodedStream);
	}

	return encodedData;
}

void
ChordUpdate::parse()
{
	resip::DataStream encodedStream(mUpdateBody);
	decode(encodedStream);

	mUpdateType = static_cast<UpdateType>(mType);
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



