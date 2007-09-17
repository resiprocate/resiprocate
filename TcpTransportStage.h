#ifndef TCPTRANSPORTSTAGE_H_
#define TCPTRANSPORTSTAGE_H_

#include <memory>
#include <vector>
#include "NetworkTransportStage.h"
//#include "ForwardingStage.h"
namespace p2p
{

class ForwardingStage;


class TcpTransportStage : public p2p::NetworkTransportStage
{
public:
	enum TcpTransportState{DISCONNECTED, CONNECTED, CONNECTING, LISTENING};
	TcpTransportStage();
	TcpTransportStage(AsyncStage *, ForwardingStage *);
	virtual ~TcpTransportStage();
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> );
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
	
	//Events issue by above layers
	int handleWriteRequest(TransportWriteRequest *);
	int handleConnectRequest(TransportConnectRequest *);
	int handleDisconnectRequest(TransportDisconnectRequest *);
	int handleListenRequest(TransportListenRequest *);
	
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
		int mIndex;
		int mLength;
		EventId mEventId;
		GenericStage * mSource;
	};

	std::vector <TransportBufferInfo * > mOutgoingBuffers;
	std::vector <TransportBufferInfo * > mIncomingBuffers;
	TcpTransportState mState;

};

}

#endif /*TCPTRANSPORTSTAGE_H_*/
