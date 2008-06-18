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

ConnectBase::ConnectBase(const resip::Data &frag, const resip::Data &password, UInt16 port, const resip::Data &role, const std::vector<resip::Data> &candidates)
{
	mUfrag = MessageHelper::convert(frag);
	mPassword = MessageHelper::convert(password);
	mApplication = port;
	mRole = MessageHelper::convert(role);

	for (std::vector<resip::Data>::const_iterator iter = candidates.begin(); iter != candidates.end(); iter++) 
	{
		IceCandidateStruct *iceStruct = new IceCandidateStruct;
		iceStruct->mCandidate = MessageHelper::convert(*iter);
		mCandidates.push_back(iceStruct);
	}
}

