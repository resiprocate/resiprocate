#ifndef FORWARDINGSTAGE_H_
#define FORWARDINGSTAGE_H_
#include <memory>
#include <map>
#include "GenericStage.h"
#include "AsyncStage.h"
#include "GenericDhtStage.h"
#include "NetworkTransportStage.h"

//TODO: Move this
#include <ext/hash_map>
#include <ext/hash_set>
#define HASH_MAP_NAMESPACE __gnu_cxx
#define HashMap __gnu_cxx::hash_map
#define HashSet __gnu_cxx::hash_set


//#include "TcpTransportStage.h"
namespace p2p
{

class EventInfo;
class GenericDhtStage;
//class TcpTransportStage;
class NetworkTransportStage;

class ForwardingStage : public p2p::GenericStage
{
	
public:
	ForwardingStage(){
	}
	ForwardingStage(p2p::AsyncStage *, p2p::GenericDhtStage *);
	virtual ~ForwardingStage();
	void initialize(p2p::AsyncStage *, p2p::GenericDhtStage *);
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> );
	int handleEvent(std::auto_ptr<p2p::EventInfo> );

	int handleWriteRequest(ForwardSendRequest *);
	int handleConnectRequest(ForwardConnectRequest *);
	int handleDisconnectRequest(ForwardDisconnectRequest *);
	int handleListenRequest(ForwardListenRequest *);
	
	int handleNotification(TransportNotification *);
	int handleListenCallback(TransportListenCallback *);
	int handleReadCallback(TransportReadCallback *);
	
	AsyncStage * mAsyncStage;
	GenericDhtStage * mDhtStage;
	
	std::map<unsigned int, NetworkTransportStage *> mFlowIdMap;

};
}

#endif /*FORWARDINGSTAGE_H_*/
