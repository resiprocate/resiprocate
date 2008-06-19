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
	std::cout << "ATTEMPTING TO DECODE" << std::endl;
	decode(dataStream);

	print(std::cout, 2);
}

void 
ConnectBase::getEncodedPayload(resip::DataStream &dataStream) 
{
	encode(dataStream);
}

ConnectBase::ConnectBase(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates) 
{
	mUfrag = frag;
	mPassword = password;
	mApplication = application;
	mRole = role;

	std::cout << "RIGHT HERE" << std::endl;

	for (std::vector<Candidate>::const_iterator iter = candidates.begin(); iter != candidates.end(); iter++) 
	{
		std::cout << "ADDED TO THE THING" << std::endl;
		IceCandidateStruct *iceStruct = new IceCandidateStruct;
		iceStruct->mCandidate = iter->getIceString();
		mCandidates.push_back(iceStruct);
	}
}

const resip::Data& 
ConnectBase::getUfrag()
{
   return mUfrag;
}

const resip::Data& 
ConnectBase::getPassword()
{
   return mPassword;
}

UInt16 
ConnectBase::getApplication()
{
   return mApplication;
}

const resip::Data&
ConnectBase::getRole()
{
   return mRole;
}

std::vector<Candidate>
ConnectBase::getCandidates()
{
   std::vector<Candidate> retCandidates;
   std::vector<IceCandidateStruct*>::iterator it = mCandidates.begin();
   for(;it != mCandidates.end(); it++)
   {
      retCandidates.push_back(Candidate((*it)->mCandidate));
   }
   return retCandidates;
}
