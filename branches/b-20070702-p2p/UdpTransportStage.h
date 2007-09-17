#ifndef UDPTRANSPORTSTAGE_H_
#define UDPTRANSPORTSTAGE_H_

#include <memory>
#include <vector>
#include <cmath>
#include "NetworkTransportStage.h"
#define HASH_MAP_NAMESPACE __gnu_cxx
#define HashMap __gnu_cxx::hash_map
#define HashSet __gnu_cxx::hash_set

namespace p2p
{

class UdpTransportStage : public p2p::NetworkTransportStage
{
public:
	enum UdpTransportState{DISCONNECTED, CONNECTED, CONNECTING, LISTENING};
	UdpTransportStage();
	UdpTransportStage(AsyncStage *, ForwardingStage *);
	virtual ~UdpTransportStage();
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> );
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
		
	//Events issue by above layers
	int handleWriteRequest(TransportWriteRequest *);
	int handleConnectRequest(TransportConnectRequest *);
	int handleDisconnectRequest(TransportDisconnectRequest *);
	int handleListenRequest(TransportListenRequest *);
	int handleTimer(TimerCallbackEvent * inCallback);
	
	//Events issued by the lower layers
	int handleNetworkCallback(NetworkCallbackEvent *);
	int handleConnectCallback(NetworkCallbackEvent *);
	int handleReadCallback(NetworkCallbackEvent *);
	int handleWriteCallback(NetworkCallbackEvent *);
	int handleListenCallback(NetworkCallbackEvent *);
	int handleRead(char * locReadBuffer, int locBytes);
	Address mPeerAddress;
	Address mSelfAddress;

private:	
	class TransportBufferInfo
	{
	public:
		TransportBufferInfo();
		~TransportBufferInfo();
		char * mBuffer;
		unsigned int mSeqNum;
		unsigned int mMsgId;
		int mLength;
		EventId mEventId;
		GenericStage * mSource;
		timeval mTime;
		Address mAddress;
		bool mRetransmit;
		unsigned int mSentTimes;
		unsigned int mFlowId;
	};
	
	class TransportMessageInfo
	{
	public:
		unsigned int mMessageId;
		unsigned int mMessageSize;
		std::vector<TransportBufferInfo *> mIncomingBuffers;
		~TransportMessageInfo();
	};
	
	class UdpPeerInfo
	{
	public:
		enum UdpConnectionStatus {CONNECTED, FAILED};

		UdpConnectionStatus mConnectionStatus;
		unsigned int mFlowId;
		unsigned int mMsgId;
		unsigned int mSeqNum;
		Address mAddress;
		std::map<unsigned int, TransportMessageInfo * > mIncomingMessages;
		std::map<unsigned int, TransportBufferInfo *> mAckedBuffers;
		
		UdpPeerInfo()
		{
			mConnectionStatus = CONNECTED;
		}
		
		void clearIncomingMessages()
		{
			TransportMessageInfo * msg;
			std::map<unsigned int, TransportMessageInfo * >::iterator mapIter;
			for (mapIter = mIncomingMessages.begin(); mapIter != mIncomingMessages.end();) 
			{
				msg = (mapIter->second);
				mIncomingMessages.erase(mapIter++);
				delete msg;
			}	
		}
		
		void clearIncomingMessage(unsigned int i)
		{
			if (mIncomingMessages.find(i) == mIncomingMessages.end())
				return;
			else
			{
				TransportMessageInfo * msg;
				msg = mIncomingMessages[i];
				mIncomingMessages.erase(i);
				delete msg;
			}
		}
	};

	struct SequenceCompare 
	{
	  bool operator() (TransportBufferInfo *i,TransportBufferInfo * j) { return (i->mSeqNum < j->mSeqNum);}
	} SequenceCompare;

	UdpPeerInfo * lookupByFlowId(unsigned int);
	UdpPeerInfo * lookupByAddress(Address);
	int failUdpPeer(UdpPeerInfo * peer);
	void insertUdpPeer (UdpPeerInfo * );
	void removeRetransmitBuffer(Address inAddress, unsigned int messageId, unsigned int seqNumber);
	void insertRetransmitBuffer(TransportBufferInfo * buffer);

	TransportBufferInfo * generateAck(Address, unsigned int messageId, unsigned int seqNumber);
	HashMap<Address, struct UdpPeerInfo *, p2p::AddressHash,  p2p::AddressCmp> mUdpPeerMap;
	std::vector <TransportBufferInfo * > mOutgoingBuffers;
	std::vector<TransportBufferInfo *> mRetransmitBuffers;
	
	unsigned int computeTimeElapsed(timeval t1, timeval t2){
		  unsigned int dt1, dt2;
		  dt1 = t1.tv_sec * 1000000 + t1.tv_usec;
		  dt2 = t2.tv_sec * 1000000 + t2.tv_usec;
		  return dt1 - dt2;
	}   
	
	UdpTransportState mState;
	unsigned int mPeerSeq;
	unsigned int mSelfSeq;
};

}

#endif /*UDPTRANSPORTSTAGE_H_*/
