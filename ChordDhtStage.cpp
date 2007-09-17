#include "ChordDhtStage.h"

#define DHT_DEBUG 1
#define USE_TCP 0
#define USE_UDP 1

namespace p2p
{
using namespace std;

const int STABILIZE_PERIOD = 5;
bool GLB_SUCCESSOR_CONNECTING = false;
std::vector<ForwardReadCallback *> GLB_BUFFERING;
std::vector<ForwardReadCallback *> GLB_CYCLEDBLOCKS;

const Locus DUMMY_LOCUS;

//Used in Forwarding
bool WrapBetween (Locus l, Locus h, Locus x){
  if (l < h) {
	  return ((x > l) && (x < h));
  }
  else {
	  return ((x > l) || (x < h));
  }
}

ChordDhtStage::ChordDhtStage(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mState = ChordDhtStage::ROOT;
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
	mStorageStage.initialize(mAsyncStage, this);
}

ChordDhtStage::~ChordDhtStage()
{
}

void ChordDhtStage::initialize(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
	mStorageStage.initialize(mAsyncStage, this);
}

void ChordDhtStage::configure(std::string inHostName, int inPort)
{
	//Set up socket related attributes
	mSelfListeningPort = inPort;
	mSelfListeningHostName = inHostName;
	struct hostent *h;
	if ((h=gethostbyname(mSelfListeningHostName.c_str())) == NULL) {
		perror("Could not get ip of self");
		return;
	}
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ((struct in_addr *)h->h_addr)->s_addr;
	address.sin_port = htons(mSelfListeningPort);
	memset(&(address.sin_zero), 0, 8);
	mSelfListeningAddress = address;
	
	//Set up ASP related fields
	Locus locLocus(inHostName, inPort);
	mPeerId = locLocus;
	mUserId = mPeerId.mString;
	cout << "DHT: peer id for this node is =====[" << mPeerId.toString() << "]=====" << endl;
	
	mRoutingStage.initializeFingerTable(mPeerId);
}

void ChordDhtStage::configure(char * argv, std::string inHostName, int inPort){
	//Set up socket related attributes
	mSelfListeningPort = inPort;
	mSelfListeningHostName = inHostName;	
	struct hostent *h;
	if ((h=gethostbyname(mSelfListeningHostName.c_str())) == NULL) {
		perror("Could not get ip of self");
		return;
	}
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ((struct in_addr *)h->h_addr)->s_addr;
	address.sin_port = htons(mSelfListeningPort);
	memset(&(address.sin_zero), 0, 8);
	mSelfListeningAddress = address;
	
	//Set up ASP related fields
	//Assign a particular locus to this node
	int intLocus = atoi(argv);
	string intString(argv);
	BIGNUM * num = BN_new();
	BN_set_word(num, intLocus);
	memset(mPeerId.mBuffer, 0, LOCUS_SIZE);
	unsigned int bytes = BN_num_bytes(num);
	unsigned int diff = LOCUS_SIZE - bytes;
	BN_bn2bin(num, mPeerId.mBuffer+diff);
	BN_free(num);
	mPeerId.mString = intString;
	mUserId = mPeerId.mString;
	cout << "DHT: peer id for this node is =====[" << mPeerId.toString() << "]=====" << endl;
	
	mRoutingStage.initializeFingerTable(mPeerId);
}

int ChordDhtStage::enqueueEvent(std::auto_ptr<p2p::EventInfo> ){

	return 0;
}

int ChordDhtStage::handleEvent(std::auto_ptr<p2p::EventInfo> inEventInfo){
	if (typeid(*inEventInfo) == typeid(p2p::ForwardReadCallback)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardReadCallback * event = (p2p::ForwardReadCallback *) locEventInfo;
		handleRead(event);
	}
	else if(typeid(*inEventInfo) == typeid(p2p::ForwardNotification)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardNotification * event = (p2p::ForwardNotification *) locEventInfo;
		handleNotification(event);
	}
	else if(typeid(*inEventInfo) == typeid(p2p::ForwardListenCallback)){
		EventInfo * locEventInfo = inEventInfo.release();
		handleAcceptance((ForwardListenCallback *) locEventInfo);
	}
	else if(typeid(*inEventInfo) == typeid(p2p::TimerCallbackEvent)){
		EventInfo * locEventInfo = inEventInfo.release();
		handleTimer((TimerCallbackEvent *) locEventInfo);
	}
	
	return 0;
}

		  
int ChordDhtStage::handleRead(ForwardReadCallback * inCallback){

	 //We are the root
	 if (mState == ROOT){
		 handleCommand(inCallback);
	 }
	 //This is the end of the line for this command
	 else if (inCallback->mDestLabelStack.empty()){
		 handleCommand(inCallback);
	 }
	 //We are explicitly defined as final destination and no more labels are on the stack
	 else if (inCallback->mDestLabelStack.front() == 1 &&
			 topLocus(inCallback->mDestLabelStack) == mPeerId &&
			 inCallback->mDestLabelStack.size() == LOCUS_PARTITIONS+1){
		 handleCommand(inCallback);
	 }
	 //We are responsible for this locus
	 else if (!(mRoutingStage.emptyPredecessors()) > 0 && 
			 inCallback->mDestLabelStack.front() == 1 &&
			 WrapBetween(mRoutingStage.frontPredecessor()->mLocus, 
					 	mPeerId, 
					 	topLocus(inCallback->mDestLabelStack)))
	 {
		 handleCommand(inCallback);
	 }
	 else{
		 if (inCallback->mDestLabelStack.front() != 1){
			 cout << "DHT: a flow id is specified, but we have no flow corresponding to this flow id" << endl;
			 delete inCallback;
			 return 0;
		 }
		 Locus locLocus = topLocus(inCallback->mDestLabelStack);
		 if (locLocus == mPeerId){
			 popLocus(inCallback->mDestLabelStack);
			 if (inCallback->mDestLabelStack.front() != 1){
				 cout << "DHT: a flow id is specified, but we have no flow corresponding to this flow id" << endl;
				 delete inCallback;
				 return 0;
			 }
			 locLocus = topLocus(inCallback->mDestLabelStack);
		 }
		 PeerInfo * peer = mRoutingStage.lookupByLocus(locLocus);
		 if (peer == NULL){
			 peer = mRoutingStage.frontSuccessor();
			 if (peer == NULL){
				 cout << "DHT: want to forward command, but do not have successor" << endl;
				 delete inCallback;
				 return 0;
			 }
			 else{
				 cout << "DHT: forwarding block to successor " << peer->mLocus.toString() << endl;
			 }
		 }
		 //Don't want to bounce message back to flow that sent message
		 if (peer->mFlowId == inCallback->mFlowId){
			 peer = mRoutingStage.frontSuccessor();
		 }
		 
		 unsigned int ttl = static_cast<unsigned int>(inCallback->mTtl);
		 cout << "DHT: forwarding block to " << peer->mLocus.toString() << " on flow id " << peer->mFlowId ;
		 cout << " with ttl " << ttl -1  << endl;

		 //Set up send request, pushing the flow id of the transport that caused us to read this packet in
		 ForwardSendRequest * request = new ForwardSendRequest();
		 request->mTtl = inCallback->mTtl - 1;
		 request->mFlowId = peer->mFlowId;
		 request->mDestLabelStack = inCallback->mDestLabelStack;
		 request->mSrcLabelStack = inCallback->mSrcLabelStack;
		 pushLabel(request->mSrcLabelStack, inCallback->mFlowId);
		 char * locBuffer = new char[inCallback->mCmdLength];
		 memcpy(locBuffer, inCallback->mCmdBuffer, inCallback->mCmdLength);
		 request->mBuffer = locBuffer;
		 request->mLength = inCallback->mCmdLength;
		 request->mEventId = EventInfo::generateEventId();
		 auto_ptr<EventInfo> requestPtr(request);
		 mForwardingStage->handleEvent(requestPtr);
		 
		 //Do not have to delete the mCmdBuffer because mForwardingStage will do that!
		 delete inCallback;
	 } 
	return 0;
}

int ChordDhtStage::handleCommand(ForwardReadCallback * inCallback){
	 CommandBlock * locCmdBlock = p2p::parseCommandBlock(inCallback->mCmdBuffer);
	 if (typeid(*locCmdBlock) == typeid(p2p::PingBlock))
	 {
		 handlePing(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::ConnectBlock))
	 {
		 handleConnect(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::JoinBlock))
	 {
		 handleJoin(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::NotifyBlock))
	 {
		 handleNotify(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::StabilizeBlock))
	 {
		 handleStabilize(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::StoreBlock))
	 {
		 handleStore(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::FetchBlock)){
		 handleFetch(inCallback);
	 }
	 else if (typeid(*locCmdBlock) == typeid(p2p::RetrievedBlock))
	 {
		 handleRetrieved(inCallback);
	 }
	 delete locCmdBlock;
	return 0;
}

int ChordDhtStage::store(Locus inLocus, 
						p2p::DataType inDataType, 
						Time inTime, 
						void * inBuffer, 
						unsigned int inLength)
{
	if (mState != JOINED){
		cout << "DHT: have not joined, cannot store" << endl;
		return 0;
	}
	
	StoreBlock *cmdBlock = new StoreBlock();
	cmdBlock->mTransactionId = generateTransactionId();
	cmdBlock->mLocus = inLocus;	
	cmdBlock->mTime = inTime;
	cmdBlock->mDataType = inDataType;
	cmdBlock->mDataLength = inLength;
	cmdBlock->mData = inBuffer;		
	cmdBlock->mSubType = STORE_ORIGINAL_TYPE;
	
	if (!mRoutingStage.emptyPredecessors()){
		if (WrapBetween(mRoutingStage.frontPredecessor()->mLocus, mPeerId, inLocus)){
			processStore(cmdBlock);
			cout << "DHT: this node is responsible for store " << inLocus.toString() << endl;
			return 0;
		}
	}
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = mRoutingStage.frontSuccessor()->mFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	pushLocus(request->mDestLabelStack, mRoutingStage.frontSuccessor()->mLocus);
	pushLocus(request->mSrcLabelStack, mPeerId);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);

	delete cmdBlock;
	return 0;
}

int ChordDhtStage::fetch(Locus inLocus, DataType inDataType){
	if (mState != JOINED){
		cout << "DHT: have not joined, cannot fetch" << endl;
		return 0;
	}
	
	FetchBlock *cmdBlock = new FetchBlock();
	cmdBlock->mTransactionId = generateTransactionId();
	cmdBlock->mDataType = inDataType;
	cmdBlock->mLocus = inLocus;	
	
	if (!mRoutingStage.emptyPredecessors()){
		if (WrapBetween(mRoutingStage.frontPredecessor()->mLocus, mPeerId, inLocus)){
			cout << "DHT: this node is responsible for fetch, here is the fetched data:" << endl;
			std::vector<p2p::DataObject *> matchingData = mStorageStage.lookupByLocus(cmdBlock->mLocus);
			for (unsigned int i = 0; i < matchingData.size(); i++)
			{
				p2p::DataObject *data = matchingData[i];
				mStorageStage.printDataObject(data);
			}
			delete cmdBlock;
			return 0;
		}
	}	
	cout << "DHT: sending fetch out to " << mRoutingStage.frontSuccessor()->mLocus.toString() << ":";
	cout << mRoutingStage.frontSuccessor()->mFlowId << endl;
	
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = mRoutingStage.frontSuccessor()->mFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	pushLocus(request->mDestLabelStack,inLocus);
	pushLocus(request->mSrcLabelStack, mPeerId);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	delete cmdBlock;
	
	return 0;
}

int ChordDhtStage::handleFetch(ForwardReadCallback *inCallback){
	FetchBlock * inCmdBlock = (FetchBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	std::vector<p2p::DataObject *> matchingData = mStorageStage.lookup(inCmdBlock->mLocus, inCmdBlock->mDataType);
	if (matchingData.empty())
	{
		cout << "STORAGE: no storage objects matching " << inCmdBlock->mLocus.toString() << endl;
		return 0;
	}
		
	cout << "DHT: responding to fetch for " << inCmdBlock->mLocus.toString() << endl;;

	//TODO: is it worth it to form connection if we don't have one back to source of fetch?
	//TODO: mutliplex by type as well
	for (unsigned int i = 0; i < matchingData.size(); i++){
		p2p::DataObject *data = matchingData[i];
		RetrievedBlock * cmdBlock = new RetrievedBlock();
		cmdBlock->mTransactionId = generateTransactionId();
		cmdBlock->mLocus = data->mLocus;	
		cmdBlock->mDataType = data->mDataType;
		cmdBlock->mDataLength = data->mDataLength;
		cmdBlock->mData = data->mData;		

		//TODO:Adjust the time for this object
		cmdBlock->mTime = data->mExpirationTime;
	
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = inCallback->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		request->mDestLabelStack = inCallback->mSrcLabelStack;
		pushLocus(request->mSrcLabelStack,mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		delete cmdBlock;
	}
	delete inCallback;
	delete inCmdBlock;
	return 0;
}



int ChordDhtStage::handleRetrieved(ForwardReadCallback *inCallback)
{
	RetrievedBlock * inCmdBlock = (RetrievedBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	mStorageStage.handleRetrieved(inCmdBlock);
	delete inCallback;
	delete inCmdBlock;
}

int ChordDhtStage::handleStore(ForwardReadCallback * inCallback)
{
	StoreBlock * inCmdBlock = (StoreBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	processStore(inCmdBlock);
	delete inCmdBlock;
	delete inCallback;
}

int ChordDhtStage::processStore(StoreBlock *inCmdBlock){
	p2p::DataObject * dataObject = mStorageStage.handleStore(inCmdBlock);
	if (inCmdBlock->mSubType == STORE_ORIGINAL_TYPE){
		//We are responsible for replicating this data block;
		std::vector<PeerInfo *> successorsList = mRoutingStage.getAvailableSuccessors();
		for (unsigned int i = 0; i < successorsList.size() && i < NUM_REPLICAS; i++){
			if (successorsList[i]->mConnectionStatus == PeerInfo::CONNECTED)
			{
				StoreBlock *cmdBlock = new StoreBlock();
				cmdBlock->mTransactionId = generateTransactionId();
				cmdBlock->mLocus = dataObject->mLocus;	
				cmdBlock->mTime = dataObject->mExpirationTime;
				cmdBlock->mDataType = dataObject->mDataType;
				cmdBlock->mDataLength = dataObject->mDataLength;
				cmdBlock->mData = dataObject->mData;		
				cmdBlock->mSubType = STORE_REPLICA_TYPE;
				
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = successorsList[i]->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, successorsList[i]->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
			}
			else
			{
				successorsList[i]->mPendingStores.push_back(dataObject);
			}
		}
	}
}

NotifyBlock * ChordDhtStage::generateNotify(){
	//Set up NotifyBlock
	NotifyBlock * cmdBlock = new NotifyBlock();
	cmdBlock->mLocus = mPeerId;
	cmdBlock->mListeningAddress = mSelfListeningAddress;	
	cmdBlock->mTransactionId = generateTransactionId();
	std::vector<PeerInfo * > locPredecessors = mRoutingStage.getConnectedPredecessors();
	std::vector<PeerInfo * > locSuccessors = mRoutingStage.getConnectedSuccessors();
	
	//Set up dummy predecessor
	if (locPredecessors.empty()){
		cmdBlock->mPredecessorNode.mLocus = DUMMY_LOCUS;
		memset(&cmdBlock->mPredecessorNode.mAddress, 0, sizeof(CandidateAddress));
	}
	else{
		cmdBlock->mPredecessorNode.mLocus = locPredecessors.front()->mLocus;
		cmdBlock->mPredecessorNode.mAddress = locPredecessors.front()->mListeningAddress;
	}		
	for (unsigned int i = 0; i < locSuccessors.size(); i++){
		PeerParameter locSuccessor;
		locSuccessor.mLocus = locSuccessors[i]->mLocus;
		locSuccessor.mAddress = locSuccessors[i]->mListeningAddress;
		cmdBlock->mSuccessorNodes.push_back(locSuccessor);
	}
	return cmdBlock;
}

int ChordDhtStage::handleSuccessors(NotifyBlock * inCmdBlock, ForwardReadCallback *inCallback){
	PeerInfo * successorNode;
	for (unsigned int i = 0; i < inCmdBlock->mSuccessorNodes.size(); i++){
		//Break if there is a loop
		if (inCmdBlock->mSuccessorNodes[i].mLocus == mPeerId)
		{
			break;
		}
		successorNode = mRoutingStage.lookupByLocus(inCmdBlock->mSuccessorNodes[i].mLocus);
		//We have never deal with this backup successor before, issue a connect to this node
		if (successorNode == NULL) 
		{	
			//Create new successor node and send out connect block. Keep track of this node as a pending peer
			successorNode = new PeerInfo();
			successorNode->setRole(PEER_ROLE_SUCCESSOR_BACKUP);
			successorNode->mConnectionStatus = PeerInfo::ISSUED_CONNECT;
			successorNode->mListeningAddress = inCmdBlock->mSuccessorNodes[i].mAddress;
			successorNode->mLocus = inCmdBlock->mSuccessorNodes[i].mLocus;
			
			//Insert the peer into successors list, and the list of all pending peers
			mRoutingStage.setSuccessor(i+1, successorNode);
			mRoutingStage.insertByLocus(successorNode->mLocus, successorNode);
			
			ConnectBlock * cmdBlock = generateConnectBlock();
			ForwardSendRequest * request = new ForwardSendRequest();
			request->mTtl = DEFAULT_TTL;
			request->mFlowId = inCallback->mFlowId;
			request->mBuffer = cmdBlock->toBuffer(request->mLength);
			pushLocus(request->mDestLabelStack, inCmdBlock->mSuccessorNodes[i].mLocus);
			pushLocus(request->mDestLabelStack, inCmdBlock->mLocus);
			pushLocus(request->mSrcLabelStack, mPeerId);
			request->mEventId = EventInfo::generateEventId();
			auto_ptr<EventInfo> requestPtr(request);
			mForwardingStage->handleEvent(requestPtr);
			
			cout << "DHT: " << successorNode->mLocus.toString() << ":" << ntohs(successorNode->mListeningAddress.sin_port);
			cout << " is a back up successor but has not sent me a connect, so we are connecting to him" << endl;
		}
		//We already have some dealings with this peer
		else{
			switch(successorNode->mConnectionStatus)
			{
			case PeerInfo::DISCONNECTED:{
				successorNode->mIndex = i+1;
				successorNode->clearRole(PEER_ROLE_SUCCESSOR);
				successorNode->addRole(PEER_ROLE_SUCCESSOR_BACKUP);
				mRoutingStage.setSuccessor(i+1, successorNode);
				
				//Send out a connect block for this guy
				successorNode->mConnectionStatus = PeerInfo::ISSUED_CONNECT;
				ConnectBlock * cmdBlock = generateConnectBlock();
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = inCallback->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, inCmdBlock->mSuccessorNodes[i].mLocus);
				pushLocus(request->mDestLabelStack, inCmdBlock->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				break;
			}
			case PeerInfo::CONNECTED:
			case PeerInfo::ISSUED_CONNECT:
			case PeerInfo::WAITING_ACCEPT:
			{
				successorNode->clearRole(PEER_ROLE_SUCCESSOR);
				successorNode->addRole(PEER_ROLE_SUCCESSOR_BACKUP);
				mRoutingStage.setSuccessor(i+1, successorNode);
				break;
			}
			default:
				break;
			}
		}
	}
	
	return 0;
}


int ChordDhtStage::handleNotify(ForwardReadCallback *inCallback){
	NotifyBlock * inCmdBlock = (NotifyBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	if (mState == JOINING){
		mState = JOINED;		
		//Set up successor, who we should have a "direct" connection to
		PeerInfo * successorNode = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
		assert(mRoutingStage.emptySuccessors());
		assert(successorNode->mLocus == inCmdBlock->mLocus);
		mRoutingStage.setSuccessor(0, successorNode);
		
		cout << "DHT: updating state to joined, we have connection to successor, so ";
		cout << inCmdBlock->mLocus.toString() << ":" << ntohs(successorNode->mListeningAddress.sin_port) << " is now a successor for ";
		cout << mPeerId.toString() << endl;

		//TODO: fix this nastiness. Check the predecessor field for a dummy node. This indicates
		//that there are only two nodes in the system.
		if (inCmdBlock->mPredecessorNode.mLocus == DUMMY_LOCUS){
			successorNode->addRole(PEER_ROLE_PREDECESSOR);
			mRoutingStage.setPredecessor(0, successorNode);
			cout << "DHT: " << inCmdBlock->mLocus.toString() << " is also a predecessor " << endl;;
		}
		handleSuccessors(inCmdBlock, inCallback);
		printState();
		
		//Start listening on parameters established by configure
		ForwardListenRequest * listener = new ForwardListenRequest();
		listener->mEventId = EventInfo::generateEventId();
		listener->mAddress = mSelfListeningAddress;
		if (USE_TCP)
		{
			listener->mType = ForwardListenRequest::TCP;
		}
		else if (USE_UDP)
		{
			listener->mType = ForwardListenRequest::UDP;
		}
		else
		{
			
		}
		auto_ptr<EventInfo> listenerPtr(listener);
		mForwardingStage->handleEvent(listenerPtr);
		
		//Set up send Request
		StabilizeBlock * cmdBlock = generateStabilize();
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = inCallback->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		pushLocus(request->mDestLabelStack, inCmdBlock->mLocus);
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		
		//Lastly, begin timer to send stabilize
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STABILIZE_PERIOD;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STABILIZE;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
		delete cmdBlock;
		delete inCallback;
		delete inCmdBlock;
	}
	else if (mState == JOINED){
		//Our successor has changed!
		if (!(inCmdBlock->mPredecessorNode.mLocus == mPeerId)){
			cout << "DHT: new successor is " << inCmdBlock->mPredecessorNode.mLocus.toString() << endl;
			PeerInfo * peer = mRoutingStage.lookupByLocus(inCmdBlock->mPredecessorNode.mLocus);
			mRoutingStage.clearSuccessors();
			
			//We've seen this peer before, simply update the successors
			if (peer != NULL){
				switch(peer->mConnectionStatus)
				{
				case PeerInfo::CONNECTED:
				{
					//We already have a connection to our new successor, can simply move him to the successors table
					PeerInfo * successorNode = peer;
					successorNode->addRole(PEER_ROLE_SUCCESSOR);
					successorNode->mListeningAddress = inCmdBlock->mPredecessorNode.mAddress;
					mRoutingStage.setSuccessor(0, successorNode);
					handleSuccessors(inCmdBlock, inCallback);
					printState();
				
					//Send stabilize to new successor
					cout << "DHT: stabilizing with already connected successor " << inCmdBlock->mPredecessorNode.mLocus.toString() << endl;
					StabilizeBlock * cmdBlock = generateStabilize();
					ForwardSendRequest * request = new ForwardSendRequest();
					request->mFlowId = successorNode->mFlowId;
					request->mBuffer = cmdBlock->toBuffer(request->mLength);
					pushLocus(request->mDestLabelStack, inCmdBlock->mPredecessorNode.mLocus);
					pushLocus(request->mSrcLabelStack, mPeerId);
					request->mEventId = EventInfo::generateEventId();
					auto_ptr<EventInfo> requestPtr(request);
					mForwardingStage->handleEvent(requestPtr);
					break;
				}
				case PeerInfo::WAITING_ACCEPT:
				case PeerInfo::ISSUED_CONNECT:
					cout << "DHT: already waiting for successor to contact or return my connect " << inCmdBlock->mPredecessorNode.mLocus.toString() << endl;
					PeerInfo * successorNode = peer;
					successorNode->addRole(PEER_ROLE_SUCCESSOR);
					mRoutingStage.setSuccessor(0, successorNode);
					handleSuccessors(inCmdBlock, inCallback);
					printState();					
					break;
				default:
					cout << "DHT: successor is in some strange state" << endl;
				}
			}
			//We lack a connection to our new successor, so we must send out a connect block
			else{
				if (DHT_DEBUG)
					cout << "DHT: no connection for new successor, sending connect block via old successor towards new successor " << endl;
				
				PeerInfo * successorNode = new PeerInfo();
				successorNode->setRole(PEER_ROLE_SUCCESSOR);
				successorNode->mConnectionStatus = PeerInfo::ISSUED_CONNECT;
				successorNode->mListeningAddress = inCmdBlock->mPredecessorNode.mAddress;
				successorNode->mLocus = inCmdBlock->mPredecessorNode.mLocus;
				mRoutingStage.insertByLocus(successorNode->mLocus, successorNode);
				mRoutingStage.setSuccessor(0, successorNode);
				
				//Need to send connect block to the new successor via the old successor
				ConnectBlock * cmdBlock = generateConnectBlock();
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = inCallback->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, inCmdBlock->mPredecessorNode.mLocus);
				pushLocus(request->mDestLabelStack, inCmdBlock->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
			}
			
		}
		//Our immediate successor has not changed!
		else{
			//Set up successor, who we should have a "direct" connection to
			PeerInfo * successorNode = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
			successorNode->mIndex = 0;
			mRoutingStage.clearSuccessors();
			mRoutingStage.setSuccessor(0, successorNode);
			handleSuccessors(inCmdBlock, inCallback);
		}
	}
}

int ChordDhtStage::join(PeerInfo * peer){
	JoinBlock * cmdBlock = new JoinBlock();
	cmdBlock->mExtBits = 0;
	cmdBlock->mPeerId = mPeerId;
	cmdBlock->mUserId = (char *) mUserId.c_str();
	cmdBlock->mTransactionId = generateTransactionId();

	//Put this node's peer id on the destination label stack, since we are joining
	//and we want the node currently responsible for this node's locus to respond to us
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = peer->mFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	pushLocus(request->mDestLabelStack, peer->mLocus);
	pushLocus(request->mSrcLabelStack, mPeerId);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	delete cmdBlock;
	
	return 0;
}

int ChordDhtStage::handleJoin(ForwardReadCallback *inCallback){
	JoinBlock * inCmdBlock = (JoinBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	NotifyBlock * cmdBlock = generateNotify();	
	PeerInfo * peer = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
	if (peer == NULL)
	{
		cout << "DHT: null peer sent join" << endl;
		delete inCmdBlock;
		delete cmdBlock;	
		return 0;
	}
	else{
		//We need to remember that this peer is specifically joining. A joining peer needs
		//to receive part of the data that we were previously responsible for. A peer that
		//is updating it's successor to a newly joined node should NOT receive any data
		peer->addRole(PEER_ROLE_JOINING);
	}
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = inCallback->mFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	request->mDestLabelStack = inCallback->mSrcLabelStack;
	pushLocus(request->mSrcLabelStack, mPeerId);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	if (DHT_DEBUG)
		cout << "DHT:  sending notify from " << mPeerId.toString() << " to " << inCmdBlock->mPeerId.toString() << endl;
	
	delete inCmdBlock;
	delete cmdBlock;	
	return 0;
}

ConnectBlock * ChordDhtStage::generateConnectBlock(){
	return generateConnectBlock(generateTransactionId());
}

ConnectBlock * ChordDhtStage::generateConnectBlock(TransactionId trid){
	//Issue connect
	ConnectBlock * cmdBlock = new ConnectBlock();
	cmdBlock->mTransactionId = trid;
	cmdBlock->mLocus = mPeerId;
	cmdBlock->mResponse = 0;
	
	//TODO: set up proper attributes for connect block
	string locUserName = mPeerId.toString();
	cmdBlock->mUserName = (char *) locUserName.c_str();
	cmdBlock->mPassword = "Guest";
	cmdBlock->mRole = "active";
	cmdBlock->mFingerPrint = 696969;
	cmdBlock->mCandidates.push_back(mSelfListeningAddress);
	
	return cmdBlock; 
}

int ChordDhtStage::handleConnect(ForwardReadCallback * inCallback){
	ConnectBlock * inCmdBlock = (ConnectBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	switch (mState)
	{
	case ROOT:
	{
		//If this node is a root node, then this should be a direct connection, 
		//or the direct connection that represents this peer
		assert(inCmdBlock->mResponse == 0);
		PeerInfo * peer = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
		if (peer == NULL){
			cout << "DHT: in handle connect, we could not find a peer representing this flow id" << endl;
			delete inCmdBlock;
			delete inCallback;
			return 0;
		}
		peer->mListeningAddress = inCmdBlock->mCandidates.front();
		ConnectBlock * cmdBlock = generateConnectBlock(inCmdBlock->mTransactionId);
		cmdBlock->mResponse = 1;
		
		//Set up send request
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		request->mFlowId = inCallback->mFlowId;
		request->mDestLabelStack = inCallback->mSrcLabelStack;
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		delete cmdBlock;
		break;
	}
	case JOINED:
	case JOINING:{
		if (inCmdBlock->mResponse == 1){
			//TODO: this should be all ICE related, but for now, just connect to the
			//first candidate address specificed
			CandidateAddress locAddress = inCmdBlock->mCandidates.front();
			PeerInfo * peer = mRoutingStage.lookupByLocus(inCmdBlock->mLocus);
			
			//We should now connect to this peer regardless, since we have no past information on this peer
			if (peer == NULL){
				//Send off a request to connect to peer
				ForwardConnectRequest * request =  new ForwardConnectRequest();
				request->mEventId = EventInfo::generateEventId();
				request->mAddress = locAddress;
				if (USE_TCP)
				{
					request->mType = ForwardConnectRequest::TCP;
				}
				else if (USE_UDP)
				{
					request->mType = ForwardConnectRequest::UDP;
				}
				else
				{
					
				}
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);	
								
				delete inCmdBlock;
				delete inCallback;
			}
			//We have dealt with this peer in some way
			else{			
				bool performConnect = true;
				switch(peer->mConnectionStatus)
				{
					case PeerInfo::CONNECTED:{
						//Should actually be able to eliminate the following case
						if (mState == JOINING){
							//We have already connected to this node, which is our successor
							peer->mListeningAddress = locAddress;
							cout << "DHT: already have connection to future successor " << peer->mLocus.toString() << endl;
							join(peer);
						}
						performConnect = false;
						break;
					}
					//We issued a connect, but we later found out that the same peer had issued a connect to us with
					//a lower peer id
					case PeerInfo::WAITING_ACCEPT:
					{
						peer->mListeningAddress = locAddress;
						cout << "DHT: waiting for " << inCmdBlock->mLocus.toString() << " to connect to us" << endl;
						performConnect = false;
						break;
					}
					case PeerInfo::DISCONNECTED:
					case PeerInfo::ISSUED_CONNECT:				
					default:
						break;
				}
				if (performConnect){
					//Send off a request to connect to peer
					peer->mListeningAddress = locAddress;
					ForwardConnectRequest * request =  new ForwardConnectRequest();
					request->mEventId = EventInfo::generateEventId();
					request->mAddress = locAddress;
					if (USE_TCP)
					{
						request->mType = ForwardConnectRequest::TCP;
					}
					else if (USE_UDP)
					{
						request->mType = ForwardConnectRequest::UDP;
					}
					else
					{
						
					}
					auto_ptr<EventInfo> requestPtr(request);
					mForwardingStage->handleEvent(requestPtr);	
				}						
				delete inCmdBlock;
				delete inCallback;
			}
		}
		//We are recieving a CONNECT block for the first time
		else{
			PeerInfo * peer = mRoutingStage.lookupByLocus(inCmdBlock->mLocus);
			//We've never dealt with this peer before, but track the candidate addresses for the future
			if (peer == NULL){
				peer = new PeerInfo();
				peer->setRole(PEER_ROLE_UNKNOWN);
				peer->mConnectionStatus = PeerInfo::WAITING_ACCEPT;
				peer->mListeningAddress = inCmdBlock->mCandidates.front();
				peer->mLocus = inCmdBlock->mLocus;
				mRoutingStage.insertByLocus(peer->mLocus, peer);
				cout << "DHT: received connect for first time for " << peer->mLocus.toString() << endl;

			}
			//We've seen this peer before in some way
			else{
				switch(peer->mConnectionStatus)
				{
					//We've issued a connect, prevent forming more than one connection
					case PeerInfo::ISSUED_CONNECT:
					{
						//We have a larger peer id, so downgrade our connection status to waiting accept
						if (inCmdBlock->mLocus < mPeerId){
							peer->mConnectionStatus = PeerInfo::WAITING_ACCEPT;
						}
						break;
					}
					default:
					{
						break;
					}
				}
				//Update
				peer->mListeningAddress = inCmdBlock->mCandidates.front();
				cout << "DHT: connect received, updating peer " << peer->mLocus.toString() << ":";
				cout << ntohs(peer->mListeningAddress.sin_port) << endl;
			}
			
			//Respond to the connect request
			ConnectBlock * cmdBlock = generateConnectBlock(inCmdBlock->mTransactionId);
			cmdBlock->mResponse = 1;				
			ForwardSendRequest * request = new ForwardSendRequest();
			request->mBuffer = cmdBlock->toBuffer(request->mLength);
			request->mFlowId = inCallback->mFlowId;
			request->mDestLabelStack = inCallback->mSrcLabelStack;
			pushLocus(request->mSrcLabelStack, mPeerId);
			request->mEventId = EventInfo::generateEventId();
			auto_ptr<EventInfo> requestPtr(request);
			mForwardingStage->handleEvent(requestPtr);
			delete cmdBlock;
			break;
		}
		break;
	}
	default:
		break;
	}	
	
	return 0;
}


StabilizeBlock * ChordDhtStage::generateStabilize(){
	StabilizeBlock * cmdBlock = new StabilizeBlock();
	cmdBlock->mListeningAddress = mSelfListeningAddress;	
	cmdBlock->mPeerId = mPeerId;
	return cmdBlock;
}


int ChordDhtStage::handleJoinedPeer(PeerInfo * peer)
{
	cout << "DHT: newly joined block is receiving the following data:" << endl;
	std::vector<DataObject *> dataObjects;
	if (mRoutingStage.emptyPredecessors())
	{
		dataObjects = mStorageStage.lookupByRange(peer->mLocus, mPeerId);
	}
	else
	{
		//Only want to send back data that this new peer is responsible for, namely, all the data we have
		//between our old predecessor and the new one
		dataObjects = mStorageStage.lookupByRange(mRoutingStage.frontPredecessor()->mLocus, peer->mLocus);
	}
	for (unsigned int i = 0; i < dataObjects.size(); i++)
	{
		DataObject * dataObject = dataObjects[i];
		mStorageStage.printDataObject(dataObject);
		StoreBlock *cmdBlock = new StoreBlock();
		cmdBlock->mTransactionId = generateTransactionId();
		cmdBlock->mLocus = dataObject->mLocus;
		
		//TODO:Adjust the time for this object
		cmdBlock->mTime = dataObject->mExpirationTime;
		cmdBlock->mDataType = dataObject->mDataType;
		cmdBlock->mDataLength = dataObject->mDataLength;
		cmdBlock->mData = dataObject->mData;		
		cmdBlock->mSubType = STORE_REPLICA_TYPE;
		
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = peer->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		pushLocus(request->mDestLabelStack, peer->mLocus);
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		delete cmdBlock;
	}
	return 0;
}

int ChordDhtStage::handleStabilize(ForwardReadCallback *inCallback){
	StabilizeBlock * inCmdBlock = (StabilizeBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	
	switch(mState){
	case ROOT:
	{
		mState = JOINED;
		PeerInfo * peer = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
		if (peer == NULL){
			cout << "DHT: in handle stabilized, a NULL peer sent a stabilized to us from " << inCallback->mFlowId << endl;
			delete inCmdBlock;
			delete inCallback;
			return 0;
		}
		//Need to send back all the data blocks
		if (peer->testRole(PEER_ROLE_JOINING))
		{
			handleJoinedPeer(peer);
			peer->clearRole(PEER_ROLE_JOINING);
		}

		peer->setRole(PEER_ROLE_SUCCESSOR);
		peer->addRole(PEER_ROLE_PREDECESSOR);
		mRoutingStage.setSuccessor(0, peer);
		mRoutingStage.setPredecessor(0, peer);
		cout << "DHT: root transitioning to joined, already have connection to ";
		cout << inCmdBlock->mPeerId.toString() << ", now a successor and predecessor for ";
		cout << mPeerId.toString() << endl;
				
		//Lastly, begin timer to send stabilize
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STABILIZE_PERIOD;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STABILIZE;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
		delete inCallback;
		delete inCmdBlock;
		break;
	}
	case JOINING:
	{
		cout << "DHT: received stabilize from " << inCallback->mFlowId << " even though this node is joining" << endl;
		delete inCallback;
		delete inCmdBlock;
		break;
	}
	case JOINED:
	{
		//We've never seen a predecessor before, this occurs when we first join the ring and have yet to see a stabilize
		if (mRoutingStage.emptyPredecessors())
		{
			PeerInfo * peer = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
			if (peer == NULL){
				cout << "DHT: null peer sent a stabilize" << endl;
				delete inCallback;
				delete inCmdBlock;
				return 0;
			}
			//Need to send back all the data blocks
			if (peer->testRole(PEER_ROLE_JOINING)){
				handleJoinedPeer(peer);
				peer->clearRole(PEER_ROLE_JOINING);
			}
			peer->addRole(PEER_ROLE_PREDECESSOR);
			assert(peer->mLocus == inCmdBlock->mPeerId);
			peer->mListeningAddress = inCmdBlock->mListeningAddress;
			mRoutingStage.setPredecessor(0, peer);
			cout << "DHT: new predecessor set to " << inCmdBlock->mPeerId.toString() << endl;
		}
		else
		{
			PeerInfo * predecessorNode = mRoutingStage.frontPredecessor();
			if (predecessorNode == NULL){
				cout << "DHT: received stabilize with non empty predecessor but we have a null predecessor" << endl;
				delete inCallback;
				delete inCmdBlock;
				return 0;
			}
			if (WrapBetween(predecessorNode->mLocus, mPeerId, inCmdBlock->mPeerId))
			{
				//We were "stabilized" from a new predecessor, we must have a connection to our new predecessor
				cout << "DHT: previous predecessor was " << predecessorNode->mLocus.toString() << endl;
				PeerInfo * peer = mRoutingStage.lookupByFlowId(inCallback->mFlowId);
				if (peer == NULL){
					cout << "DHT: null peer sent a stabilize, this is not possible" << endl;
					delete inCallback;
					delete inCmdBlock;
					return 0;
				}
				//Need to send back all the data blocks
				if (peer->testRole(PEER_ROLE_JOINING)){
					handleJoinedPeer(peer);
					peer->setRole(PEER_ROLE_PREDECESSOR);
				}
				assert(peer->mLocus == inCmdBlock->mPeerId);
				mRoutingStage.clearPredecessors();
				mRoutingStage.setPredecessor(0, peer);
				cout << "DHT: new predecessor is " <<  peer->mLocus.toString() << endl;
				
				if (DHT_DEBUG){
					cout << "DHT: informing old predecessor " << predecessorNode->mLocus.toString();
					cout << " about new successor " << peer->mLocus.toString() << endl;
				}
				
				//Downgrade role and inform old predecessor about it's new successor
				predecessorNode->clearRole(PEER_ROLE_PREDECESSOR);
				NotifyBlock * cmdBlock = generateNotify();	
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = predecessorNode->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, predecessorNode->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
			}
			else{
			}
		}
		
		//Needs to be sent back to predecessor regardless, whether new or old
		NotifyBlock * cmdBlock = generateNotify();
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = inCallback->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		request->mDestLabelStack = inCallback->mSrcLabelStack;
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		
		delete inCmdBlock;
		delete inCallback;
		break;
	}
	default:
		cout << "DHT: received stabilize in some unknown state " << endl;
		delete inCmdBlock;
		delete inCallback;
	}
	return 0;
}

int ChordDhtStage::handleDiscovery(PingBlock * inCmdBlock, ForwardReadCallback * inCallback)
{
	PeerInfo * peer = NULL;
	Locus inLocus = inCmdBlock->mPeerId;
	unsigned int inFlowId = inCallback->mFlowId;
	if ((peer = mRoutingStage.lookupByLocus(inLocus)) == NULL)
	{
		//This peer is entirely new, we have no prior information about this node
		peer = new PeerInfo();
		peer->mLocus = inLocus;
		peer->mFlowId = inFlowId;
		peer->setRole(PEER_ROLE_UNKNOWN);
		peer->mConnectionStatus = PeerInfo::CONNECTED;
		mRoutingStage.insertByLocus(inLocus, peer);
	}
	//We know something about this peer
	else
	{
		peer->mLocus = inLocus;
		peer->mFlowId = inFlowId;
		peer->mConnectionStatus = PeerInfo::CONNECTED;
		
		if (peer->testRole(PEER_ROLE_SUCCESSOR))
		{
			cout << "DHT: now have connection with successor " << peer->mLocus.toString() <<  endl;
			std::vector<p2p::DataObject * >::iterator iter;
			for (iter = peer->mPendingStores.begin(); iter != peer->mPendingStores.end();){
				p2p::DataObject * dataObject = (*iter);
				StoreBlock *cmdBlock = new StoreBlock();
				cmdBlock->mTransactionId = generateTransactionId();
				cmdBlock->mLocus = dataObject->mLocus;	
				cmdBlock->mTime = dataObject->mExpirationTime;
				cmdBlock->mDataType = dataObject->mDataType;
				cmdBlock->mDataLength = dataObject->mDataLength;
				cmdBlock->mData = dataObject->mData;		
				cmdBlock->mSubType = STORE_REPLICA_TYPE;
				
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = peer->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, peer->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
				
				iter = peer->mPendingStores.erase(iter);
				delete [] dataObject->mData;
				delete dataObject;
			}
		}
		if (peer->testRole(PEER_ROLE_SUCCESSOR_BACKUP))
		{
			cout << "DHT: now have connection with backup successor " << peer->mLocus.toString() <<  endl;
			std::vector<p2p::DataObject * >::iterator iter;
			for (iter = peer->mPendingStores.begin(); iter != peer->mPendingStores.end();)
			{
				p2p::DataObject * dataObject = (*iter);
				StoreBlock *cmdBlock = new StoreBlock();
				cmdBlock->mTransactionId = generateTransactionId();
				cmdBlock->mLocus = dataObject->mLocus;	
				cmdBlock->mTime = dataObject->mExpirationTime;
				cmdBlock->mDataType = dataObject->mDataType;
				cmdBlock->mDataLength = dataObject->mDataLength;
				cmdBlock->mData = dataObject->mData;		
				cmdBlock->mSubType = STORE_REPLICA_TYPE;
				
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = peer->mFlowId;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, peer->mLocus);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
				
				iter = peer->mPendingStores.erase(iter);
				delete [] dataObject->mData;
				delete dataObject;
			}
		}

		if (peer->testRole(PEER_ROLE_PREDECESSOR))
		{
			cout << "DHT: now have connection with predecessor " << peer->mLocus.toString() << endl;
		}

		if (peer->testRole(PEER_ROLE_FINGER)){
			
		}

		if (peer->testRole(PEER_ROLE_BOOTSTRAP)){
			
		}

	}
	
	//We've either created a new peer or reacted to a peer that was already in our system
	switch(inCmdBlock->mSubType)
	{
	case PING_RESPONSE_DISCOVER:
	{
		switch (mState)
		{
			case BOOTSTRAPPING:
			{
				mState = JOINING;
				peer->setRole(PEER_ROLE_BOOTSTRAP);
				PingBlock *cmdBlock = new PingBlock();
				cmdBlock->mTransactionId = generateTransactionId();
				cmdBlock->mPeerId = mPeerId;
				cmdBlock->mSubType = PING_REQUEST;

				//Set up a send Request
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mFlowId = inCallback->mFlowId;
				if (DHT_DEBUG)
					cout << "DHT: sending out ping on flow "<< request->mFlowId << endl;
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				pushLocus(request->mDestLabelStack, mPeerId);
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
				break;
			}
			case ROOT:
			case JOINED:
				peer = mRoutingStage.lookupByLocus(inLocus);
				peer->mFlowId = inFlowId;
				break;
			case JOINING:
				peer = mRoutingStage.lookupByLocus(inLocus);
				peer->mFlowId = inFlowId;
				join(peer);
				break;
			default:
				cout << "ERROR" << endl;
				break;
		}
		break;
	}
	case PING_REQUEST_DISCOVER:
	{
		PingBlock * cmdBlock = new PingBlock();
		cmdBlock->mTransactionId = inCmdBlock->mTransactionId;
		cmdBlock->mPeerId = mPeerId;
		cmdBlock->mSubType = PING_RESPONSE_DISCOVER;
		
		if (DHT_DEBUG){
			cout << "DHT: sending ping discovery response from " << mPeerId.toString();
			cout << " to " << inCmdBlock->mPeerId.toString() << endl;
		}
		//Set up Send Request
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = inCallback->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		request->mDestLabelStack = inCallback->mSrcLabelStack;
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		delete cmdBlock;
		break;
	}
	default:
		cout << "ERROR!" << endl;
	}
	delete inCmdBlock;
	delete inCallback;
	return 0;
}

int ChordDhtStage::handlePing(ForwardReadCallback * inCallback){
	PingBlock * inCmdBlock = (PingBlock *) p2p::parseCommandBlock(inCallback->mCmdBuffer);
	switch(inCmdBlock->mSubType)
	{
		//These are special empty pings that let us know who we are directly connected to
		case PING_RESPONSE_DISCOVER:
		case PING_REQUEST_DISCOVER:
			handleDiscovery(inCmdBlock, inCallback);
			break;
		//Otherwise, these ping types are as expected
		case PING_RESPONSE:
			switch (mState)
			{
			case JOINING:{
				//TODO: want to ping inCmdBlock->mPeerId+1 and so forth to obtain neighborhood set
				//For now, just issue connect
				PeerInfo * peer = mRoutingStage.lookupByLocus(inCmdBlock->mPeerId);
				if (peer != NULL)
				{
					//Should be able to optimize here, since we know we are connected to this peer
					//... for the time being, generate a connect, since we want the listening address
					peer->setRole(PEER_ROLE_SUCCESSOR);
					assert(peer->mConnectionStatus == PeerInfo::CONNECTED);
				}
				else
				{
					peer = new PeerInfo();
					peer->setRole(PEER_ROLE_SUCCESSOR);
					peer->mConnectionStatus = PeerInfo::ISSUED_CONNECT;
					peer->mLocus = inCmdBlock->mPeerId;
					mRoutingStage.insertByLocus(inCmdBlock->mPeerId, peer);
				}
				ConnectBlock * cmdBlock = generateConnectBlock(inCmdBlock->mTransactionId);
				if (DHT_DEBUG) {
					cout << "DHT: sending connect from "<< mPeerId.toString();
					cout << " to "<< inCmdBlock->mPeerId.toString() << endl;
				}
				
				//Set up send request
				ForwardSendRequest * request = new ForwardSendRequest();
				request->mBuffer = cmdBlock->toBuffer(request->mLength);
				request->mFlowId = inCallback->mFlowId;
				request->mDestLabelStack = inCallback->mSrcLabelStack;
				pushLocus(request->mSrcLabelStack, mPeerId);
				request->mEventId = EventInfo::generateEventId();
				auto_ptr<EventInfo> requestPtr(request);
				mForwardingStage->handleEvent(requestPtr);
				delete cmdBlock;
				delete inCmdBlock;
				break;
			}
			case JOINED:
				cout << "DHT: ping response received from " << inCmdBlock->mPeerId.toString() << endl;
				break;
			case ROOT:
			case BOOTSTRAPPING:
			default:
				cout << "DHT: error! received ping response from " << inCmdBlock->mPeerId.toString();
				cout << " while in a state not expecting ping response" << endl;
			}
			break;
		case PING_REQUEST:
		{
			PingBlock * cmdBlock = new PingBlock();
			cmdBlock->mTransactionId = inCmdBlock->mTransactionId;
			cmdBlock->mPeerId = mPeerId;
			cmdBlock->mSubType = PING_RESPONSE;
			
			if (DHT_DEBUG){
				cout << "DHT: sending ping response from " << mPeerId.toString();
				cout << " to " << inCmdBlock->mPeerId.toString();
				cout << " for " << topLocus(inCallback->mDestLabelStack).toString() << endl;
			}
			//Set up Send Request
			ForwardSendRequest * request = new ForwardSendRequest();
			request->mFlowId = inCallback->mFlowId;
			request->mBuffer = cmdBlock->toBuffer(request->mLength);
			request->mDestLabelStack = inCallback->mSrcLabelStack;
			pushLocus(request->mSrcLabelStack, mPeerId);
			request->mEventId = EventInfo::generateEventId();
			auto_ptr<EventInfo> requestPtr(request);
			mForwardingStage->handleEvent(requestPtr);
			delete cmdBlock;
			delete inCmdBlock;
			break;
		}
	}
	
	return 0;
}

int ChordDhtStage::ping(unsigned int inFlowId){
	cout << "DHT: sending empty ping out on " << inFlowId << endl;
	//Remember the transaction id for this discovery request
	PingBlock *cmdBlock = new PingBlock();
	cmdBlock->mTransactionId = generateTransactionId();
	cmdBlock->mPeerId = mPeerId;	
	cmdBlock->mSubType = PING_REQUEST_DISCOVER;
	
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = inFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	pushLocus(request->mSrcLabelStack, mPeerId);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	delete cmdBlock;
	return 0;
}

int ChordDhtStage::ping(Locus inLocus){
	unsigned int locFlowId;
	PingBlock *cmdBlock = new PingBlock();
	cmdBlock->mTransactionId = generateTransactionId();
	cmdBlock->mPeerId = mPeerId;	
	cmdBlock->mSubType = PING_REQUEST;
	
	if (!mRoutingStage.emptyPredecessors()){
		if (WrapBetween(mRoutingStage.frontPredecessor()->mLocus, mPeerId, inLocus)){
			cout << "DHT: this node is responsible for ping " << inLocus.toString() << endl;
			return 0;
		}
	}
	
	if (mRoutingStage.emptySuccessors()){
		cout << "DHT: have no node to send ping for " << inLocus.toString() << endl;
		return 0;
	}
	locFlowId = mRoutingStage.frontSuccessor()->mFlowId;
	
	if (DHT_DEBUG){
		cout << "DHT: sending ping from " << mPeerId.toString();
		cout << " to " << inLocus.toString()  << " using flow id " << locFlowId << endl;
	}
	
	//Set up Send Request
	ForwardSendRequest * request = new ForwardSendRequest();
	request->mFlowId = locFlowId;
	request->mBuffer = cmdBlock->toBuffer(request->mLength);
	pushLocus(request->mDestLabelStack, inLocus);
	request->mEventId = EventInfo::generateEventId();
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	
	delete cmdBlock;
	return 0;
}

int ChordDhtStage::handleFailure(PeerInfo * peer){
	if (peer->testRole(PEER_ROLE_SUCCESSOR)){
		cout << "DHT: successor " << peer->mLocus.toString() << " failed" <<  endl;
		mRoutingStage.removeSuccessor(peer);
		PeerInfo * successorNode = mRoutingStage.frontSuccessor();
		if (successorNode == NULL){
			cout << "DHT: no more successors... reverting to root" << endl;
			mState = ROOT;
			return 0;
		}
		successorNode->addRole(PEER_ROLE_SUCCESSOR);
		//If we already have connection, then send stabilize
		if (successorNode->mConnectionStatus == PeerInfo::CONNECTED){
			cout << "DHT: stabilizing with backup successor " << successorNode->mLocus.toString() << endl;
			StabilizeBlock * cmdBlock = generateStabilize();
			ForwardSendRequest * request = new ForwardSendRequest();
			request->mFlowId = successorNode->mFlowId;
			request->mBuffer = cmdBlock->toBuffer(request->mLength);
			pushLocus(request->mDestLabelStack, successorNode->mLocus);
			pushLocus(request->mSrcLabelStack, mPeerId);
			request->mEventId = EventInfo::generateEventId();
			auto_ptr<EventInfo> requestPtr(request);
			mForwardingStage->handleEvent(requestPtr);
			delete cmdBlock;
		}
		else
		{
			//TODO: timeout on the connect? try sending to next successor that is connected?
		}
	}
	if (peer->testRole(PEER_ROLE_SUCCESSOR_BACKUP))
	{
		cout << "DHT: backup successor failed " << peer->mLocus.toString() << endl;
		mRoutingStage.removeSuccessor(peer);
	}
	
	if (peer->testRole(PEER_ROLE_PREDECESSOR))
	{
		cout << "DHT: predecessor failed" << endl;
		mRoutingStage.removePredecessor(peer);	
	}

	if (peer->testRole(PEER_ROLE_BOOTSTRAP)){
			
	}

	if (peer->testRole(PEER_ROLE_FINGER)){
			
	}
	
	//Keep the peer around for a while
	peer->mConnectionStatus = PeerInfo::FAILED;
	peer->setRole(PEER_ROLE_UNKNOWN);
	
	return 0;
}
int ChordDhtStage::handleNotification(ForwardNotification * inEvent){
	switch(inEvent->mEventType){
	case ForwardNotification::FAILURE:{
		PeerInfo * peer = mRoutingStage.lookupByFlowId(inEvent->mFlowId);
		cout << "DHT: flow " << inEvent->mFlowId << " failed" << endl;
		if (peer == NULL){
			cout << "DHT: flow " << inEvent->mFlowId << " failed, but no peer associated with it " << endl;
			delete inEvent;
		}
		else{
			handleFailure(peer);
			delete inEvent;
		}
		break;
	}
	case ForwardNotification::CONNECT_COMPLETE:{
		//Perform empty ping on newly connected node
		ping(inEvent->mFlowId);
		break;
	}
	case ForwardNotification::WRITE_COMPLETE:
		break;
	}
	
	return 0;
}

int ChordDhtStage::handleTimer(TimerCallbackEvent * inCallback){
	if (mState == ROOT){
		delete inCallback;
		return 0;
	}
	
	if (inCallback->mTimerType == TimerRequestEvent::STABILIZE){
		StabilizeBlock * cmdBlock = generateStabilize();
		if (mRoutingStage.emptySuccessors()){
			delete cmdBlock;
			delete inCallback;
			cout << "DHT: stabilizing, but no successors" << endl;
			return 0;
		}
		PeerInfo * successor = mRoutingStage.frontSuccessor();
		if (successor->mConnectionStatus != PeerInfo::CONNECTED)
		{
			delete cmdBlock;
			delete inCallback;
			cout << "DHT: stabilizing, but no successors" << endl;
			return 0;
		}
		
		//Set up Send Request
		ForwardSendRequest * request = new ForwardSendRequest();
		request->mFlowId = successor->mFlowId;
		request->mBuffer = cmdBlock->toBuffer(request->mLength);
		pushLocus(request->mDestLabelStack, successor->mLocus);
		pushLocus(request->mSrcLabelStack, mPeerId);
		request->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(request);
		mForwardingStage->handleEvent(requestPtr);
		/*
		if (DHT_DEBUG){
			cout << "TIMER: sending stabilize from " << mPeerId.toString();
			cout << " to " << mRoutingStage.mSuccessorsTable.front()->mLocus.toString()  << endl;
		}
		*/
		
		//Lastly, renew timer to send stabilize
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STABILIZE_PERIOD;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STABILIZE;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
		
		delete cmdBlock;
		delete inCallback;
	}
	
	return 0;
}

void ChordDhtStage::printState(){
	cout << "DHT: state for peer " << mPeerId.toString() << endl;
	std::vector<PeerInfo * > successors = mRoutingStage.getConnectedSuccessors();
	std::vector<PeerInfo * > predecessors = mRoutingStage.getConnectedPredecessors();
	
	for (unsigned int i = 0; i < successors.size(); i++){
		cout << " 	DHT: successor[" << i << "] = " << successors[i]->mLocus.toString() << ":"
			<< ntohs(successors[i]->mListeningAddress.sin_port) << endl;
		
	}
	for (unsigned int i = 0; i <predecessors.size(); i++){
		cout << " 	DHT: predecessor[" << i << "] = " << predecessors[i]->mLocus.toString() << ":"
		<< ntohs(predecessors[i]->mListeningAddress.sin_port) << endl;
	}	
	
	
}
int ChordDhtStage::handleAcceptance(ForwardListenCallback * inEvent){	
	if (DHT_DEBUG){
		cout << "Accepted a connection with flow id " << inEvent->mFlowId << endl;
	}
	delete inEvent;
	
	return 0;
}

int ChordDhtStage::listen(){
	//Start listening on parameters established by configure
	ForwardListenRequest * request = new ForwardListenRequest();
	request->mEventId = EventInfo::generateEventId();
	request->mAddress = mSelfListeningAddress;
	if (USE_TCP)
	{
		request->mType = ForwardListenRequest::TCP;
	}
	else if (USE_UDP)
	{
		request->mType = ForwardListenRequest::UDP;
	}
	else
	{
		
	}
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	mState = ROOT;
	return 0;
}


void ChordDhtStage::bootstrap(Address inAddress){
	EventId locEventId = EventInfo::generateEventId();	
	ForwardConnectRequest * request =  new ForwardConnectRequest();
	request->mEventId = locEventId;
	request->mAddress = inAddress;
	if (USE_TCP)
	{
		request->mType = ForwardConnectRequest::TCP;
	}
	else if (USE_UDP)
	{
		request->mType = ForwardConnectRequest::UDP;
	}
	else
	{
		
	}
	auto_ptr<EventInfo> requestPtr(request);
	mForwardingStage->handleEvent(requestPtr);
	mState = BOOTSTRAPPING;
}

}
