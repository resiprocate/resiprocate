#ifndef EVENTINFO_H_
#define EVENTINFO_H_
#include <map>
#include <vector>
#include <map> 
#include <set>
#include <string>
#include <cstdlib>
#include <stack>
#include "GenericStage.h"
#include "Socket.h"
#include "Locus.h"
#include "ForwardBlock.h"
#include "CommandBlock.h"
//#include "NetworkTransportStage.h"
#include "FdSet.h"

namespace p2p
{
class GenericStage;
class ForwardBlock;
//class CommandBlock;
class NetworkTransportStage;

typedef long long EventId;

class EventInfo
{
public:
	static std::set<EventId> mEventIdSet;
	EventInfo(){};
	EventInfo( GenericStage *);
	virtual ~EventInfo();
	int mEventType;
	EventId mEventId;
	p2p::GenericStage * mEventSource;

	static EventId generateEventId(){
		EventId locRand = rand();
		while(p2p::EventInfo::mEventIdSet.find(locRand) != 
				p2p::EventInfo::mEventIdSet.end())
		{
			locRand = rand();
		}
		return locRand;		
	}	
};

class TimerRequestEvent: public p2p::EventInfo
{
public:
	enum TimerType{STABILIZE, RETRANSMIT, FINGER_PROBE, STORE_EXPIRATION, UDP_TIMEOUT};
	TimerRequestEvent(GenericStage * inEventSource){
			mEventSource = inEventSource;
	}
	timeval mExpireTime;
	TimerType mTimerType;
};

class TimerCallbackEvent: public p2p::EventInfo{
public:
	TimerCallbackEvent(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
	TimerRequestEvent::TimerType mTimerType;
};

class NetworkRequestEvent: public p2p::EventInfo
{
public:
	enum NetworkRequestType{CONNECT, WRITE, READ, LISTEN};
	enum NetworkRequestResult{COMPLETE, INCOMPLETE, FAIL};

	NetworkRequestEvent(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
	Socket mSocket;
};

class NetworkCallbackEvent: public p2p::EventInfo
{
public:
	enum NetworkCallbackType{CONNECT, WRITE, READ, LISTEN};
	NetworkCallbackEvent(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
};


class TransportWriteRequest: public p2p::EventInfo
{
public:
	~TransportWriteRequest(){
		if (mBuffer)
			delete []mBuffer;
	}
	char * mBuffer;
	int mLength;
	unsigned int mFlowId;
};

class FdRequestEvent: public p2p::EventInfo
{
public:
	enum FdRequestType{READ, WRITE};
	enum FdRequestResult{COMPLETE, INCOMPLETE, FAIL};
	int mFd;
	FdRequestEvent(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
};

class FdCallbackEvent: public p2p::EventInfo
{
public:
	enum FdCallbackType{WRITE, READ};
	FdCallbackEvent(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
	int mFd;
};

class TransportConnectRequest: public p2p::EventInfo
{
public:
	Address mAddress;
};

class TransportListenRequest: public p2p::EventInfo
{
public:
	Address mAddress;
};

class TransportDisconnectRequest: public p2p::EventInfo
{
public:
	Address mAddress;
};


class TransportNotification: public p2p::EventInfo
{
public:
	enum TransportNotificationType{CONNECT_COMPLETE, WRITE_COMPLETE, FAILURE};
	TransportNotification(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
	Address mAddress;
	unsigned int mFlowId;

};


class TransportListenCallback: public p2p::EventInfo
{
public:
	TransportListenCallback(GenericStage * inEventSource){
		mEventSource = inEventSource;
	};
	NetworkTransportStage * mTransportStage;
	Address mAddress;
	unsigned int mFlowId;
};

class TransportReadCallback: public p2p::EventInfo
{
public:
	TransportReadCallback(GenericStage * inEventSource){
		mEventSource = inEventSource;
		mBuffer = NULL;
	}
	~TransportReadCallback(){
		if (mBuffer)
			delete [] mBuffer;
	}
	unsigned int mFlowId;
	Locus mLocus;
	int mLength;
	char * mBuffer;
	Address mAddress;
};


class ForwardConnectRequest: public p2p::EventInfo
{
public:
	enum ForwardConnectType{TCP, UDP, TLS, DTLS};
	Locus mLocus;
	Address mAddress;
	ForwardConnectType mType;
};

class ForwardDisconnectRequest: public p2p::EventInfo
{
public:
	Locus mLocus;
	Address mAddress;
	unsigned int mFlowId;
};

class ForwardListenRequest: public p2p::EventInfo
{
public:
	ForwardListenRequest(){};
	enum ForwardListenType{TCP, UDP, TLS, DTLS};
	Address mAddress;
	Locus mLocus;
	ForwardListenType mType;
};

const unsigned char DEFAULT_TTL = 16;

class ForwardSendRequest: public p2p::EventInfo
{
public:
	ForwardSendRequest(){
		mTransport = NULL;
		mTtl = DEFAULT_TTL;
	}
	~ForwardSendRequest(){
		if (mBuffer)
			delete [] mBuffer;
	}
	unsigned int mFlowId;
	Locus mLocus;
	unsigned char mTtl;
	std::vector<unsigned int> mDestLabelStack;
	std::vector<unsigned int> mSrcLabelStack;
	char * mBuffer;
	int mLength;
	GenericStage * mTransport;
	
};

class ForwardReadCallback: public p2p::EventInfo
{
public:
	ForwardReadCallback(GenericStage * inEventSource){
		mEventSource = inEventSource;
	};
	~ForwardReadCallback(){
		if (mCmdBuffer)
			delete [] mCmdBuffer;
	}
	char * mCmdBuffer;
	int mCmdLength;
	std::vector<unsigned int> mDestLabelStack;
	std::vector<unsigned int> mSrcLabelStack;
	unsigned char mTtl;
	Address mAddress;
	unsigned int mFlowId;
};

class ForwardListenCallback: public p2p::EventInfo
{
public:
	Address mAddress;
	unsigned int mFlowId;
};

class ForwardNotification: public p2p::EventInfo
{
public:
	enum ForwardNotificationType{CONNECT_COMPLETE, WRITE_COMPLETE, FAILURE};
	ForwardNotification(GenericStage * inEventSource){
		mEventSource = inEventSource;
	}
	Address mAddress;
	Locus mLocus;
	unsigned int mFlowId;
};

}


#endif /*EVENTINFO_H_*/
