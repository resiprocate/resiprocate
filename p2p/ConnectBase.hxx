#ifndef P2P_CONNECTBASE_HXX
#define P2P_CONNECTBASE_HXX

#include "p2p/Message.hxx"
#include "p2p/MessageHelper.hxx"
#include "p2p/Candidate.hxx"

namespace p2p
{

class ConnectBase : public Message, protected s2c::ConnectReqAnsStruct
{
public:
	friend class Message;

	ConnectBase(const resip::Data &frag, const resip::Data &password, uint16_t application, const resip::Data &role, const std::vector<Candidate> &candidates);
	virtual void getEncodedPayload(resip::DataStream &dataStream);

   const resip::Data& getUfrag();
   const resip::Data& getPassword();
   uint16_t getApplication();
   const resip::Data& getRole();
   std::vector<Candidate> getCandidates();

   void setCandidates(const std::vector<Candidate> &candidates);

protected:
   virtual void decodePayload(resip::DataStream &dataStream);
	ConnectBase();
};

}

#endif
