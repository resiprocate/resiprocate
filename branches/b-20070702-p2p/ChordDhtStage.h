#ifndef CHORDDHTSTAGE_H_
#define CHORDDHTSTAGE_H_

#include "GenericDhtStage.h"
#include "AsyncStage.h"
#include "ForwardingStage.h"
#include "CommandBlock.h"
#include "RoutingStage.h"
#include "StorageStage.h"
#include <memory>
#include <map>
#include <ext/hash_map>
#include <ext/hash_set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HASH_MAP_NAMESPACE __gnu_cxx
#define HashMap __gnu_cxx::hash_map
#define HashSet __gnu_cxx::hash_set

namespace p2p
{

class EventInfo;
class EventState;
class StorageStage;

class ChordDhtStage : public p2p::GenericDhtStage
{
	enum DhtState{BOOTSTRAPPING, JOINING, CONNECTING, ROOT, JOINED};
	
public:
	ChordDhtStage(){};
	ChordDhtStage(AsyncStage * , ForwardingStage * );
	void initialize(p2p::AsyncStage *, p2p::ForwardingStage *);
	virtual ~ChordDhtStage();
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> );
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
	void configure(std::string, int);
	void configure(char *, std::string, int);
	void initializeFingerTable();
	
	int store(Locus inLocus, 
			p2p::DataType inDataType, 
			Time inTime, 
			void * inBuffer, 
			unsigned int inLength);
	int fetch(Locus inLocus, DataType inDataType);
	int ping(unsigned int);
	int ping(Locus inLocus);
	void bootstrap(Address);
	int listen();
	
	ConnectBlock * generateConnectBlock(TransactionId);
	ConnectBlock * generateConnectBlock();
	
	int handleRead(ForwardReadCallback *);	
	int handleCommand(ForwardReadCallback *);	
	int handleNotification(ForwardNotification *);
	int handleAcceptance(ForwardListenCallback *);
	int handleTimer(TimerCallbackEvent *);
	
	int processStore(StoreBlock *);
	int handleStore(ForwardReadCallback *);
	int handlePing(ForwardReadCallback *);
	int handleConnect(ForwardReadCallback * inCallback);
	int handleJoin(ForwardReadCallback *inCallback);
	int handleNotify(ForwardReadCallback *inCallback);
	int handleStabilize(ForwardReadCallback *inCallback);
	int handleDiscovery(PingBlock *, ForwardReadCallback *);
	int handleSuccessors(NotifyBlock * inCmdBlock, ForwardReadCallback *inCallback);
	int handleFailure(PeerInfo * peer);
	int handleFetch(ForwardReadCallback *inCallback);
	int handleRetrieved(ForwardReadCallback *inCallback);
	
	int handleJoinedPeer(PeerInfo * peer);
	void printState();
	
private:	
	class TransactionInfo
	{
	public:
		enum TransactionState{STORING, FINGERING};
		TransactionInfo(){};
		~TransactionInfo(){};
		p2p::CommandBlock * mCmdBlock;
		int mIndex;
		TransactionState mState;
	};
	
	int mSelfListeningPort;
	std::string mSelfListeningHostName;
	Address mSelfListeningAddress;
	Locus mPeerId;
	std::string mUserId;
	DhtState mState;
	RoutingStage mRoutingStage;
	StorageStage mStorageStage;
	AsyncStage * mAsyncStage;
	ForwardingStage * mForwardingStage;

	StabilizeBlock * generateStabilize();
	NotifyBlock * generateNotify();
	int forwardCommand(ForwardReadCallback *);

	std::map<EventId, PeerInfo *> mEventIdMap;
	std::map<TransactionId, PeerInfo *> mTrIdMap;
	int join(PeerInfo *);
	
	friend class StorageStage;
};

}

#endif /*CHORDDHTSTAGE_H_*/
