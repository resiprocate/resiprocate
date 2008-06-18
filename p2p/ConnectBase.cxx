#include "p2p/ConnectBase.hxx"
#include "p2p/Message.hxx"

using namespace p2p;
using namespace s2c;


ConnectBase::ConnectBase()
{

}

void 
ConnectBase::decodePayload(resip::DataStream &dataStream)
{
	decode(dataStream);
}

void 
ConnectBase::getEncodedPayload(resip::DataStream &dataStream) 
{
	encode(dataStream);
}

ConnectBase::ConnectBase(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates) 
{
	mUfrag = frag;
	mPassword = password;
	mApplication = application;
	mRole = role;

	for (std::vector<resip::Data>::const_iterator iter = candidates.begin(); iter != candidates.end(); iter++) 
	{
		IceCandidateStruct *iceStruct = new IceCandidateStruct;
		iceStruct->mCandidate = *iter;
		mCandidates.push_back(iceStruct);
	}
}

