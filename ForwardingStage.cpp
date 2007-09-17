#include "ForwardingStage.h"
#include "TcpTransportStage.h"
#include "UdpTransportStage.h"

using namespace p2p;
using namespace std;


ForwardingStage::ForwardingStage(AsyncStage * inAsyncStage, GenericDhtStage * inDhtStage)
{	
	mAsyncStage = inAsyncStage;
	mDhtStage = inDhtStage; 
}

ForwardingStage::~ForwardingStage()
{
}

void ForwardingStage::initialize(AsyncStage * inAsyncStage, GenericDhtStage * inDhtStage)
{
	mAsyncStage = inAsyncStage;
	mDhtStage = inDhtStage; 
}

int ForwardingStage::handleEvent(auto_ptr<EventInfo> inEventInfo){
	if (typeid(*inEventInfo) == typeid(p2p::ForwardSendRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardSendRequest * requestEvent = (p2p::ForwardSendRequest *) locEventInfo;
		handleWriteRequest(requestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::ForwardConnectRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardConnectRequest * requestEvent = (p2p::ForwardConnectRequest *) locEventInfo;
		return handleConnectRequest(requestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::ForwardDisconnectRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardDisconnectRequest * requestEvent = (p2p::ForwardDisconnectRequest *) locEventInfo;
		handleDisconnectRequest(requestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::ForwardListenRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		ForwardListenRequest * requestEvent = (p2p::ForwardListenRequest *) locEventInfo;
		handleListenRequest(requestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportNotification)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportNotification * notificationEvent = (p2p::TransportNotification *) locEventInfo;
		handleNotification(notificationEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportListenCallback)){	
		EventInfo * locEventInfo = inEventInfo.release();
		TransportListenCallback * notificationEvent = (p2p::TransportListenCallback *) locEventInfo;
		handleListenCallback(notificationEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportReadCallback)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportReadCallback * readEvent = (p2p::TransportReadCallback *) locEventInfo;
		handleReadCallback(readEvent);
	}
	
	return 0;
}


int ForwardingStage::enqueueEvent(auto_ptr<EventInfo> inEventInfo){
	return 0;
}

int ForwardingStage::handleWriteRequest(ForwardSendRequest * inRequest){
	GenericStage * locTransport = NULL;
	if (mFlowIdMap.find(inRequest->mFlowId) == mFlowIdMap.end())
	{
		cout << "FORWARDING: no transport for " << inRequest->mFlowId << endl;
		//Need to delete the request
		delete inRequest;
		return 0;
	}
	else{
		locTransport = mFlowIdMap[inRequest->mFlowId];
	}
	
	TransportWriteRequest * transportRequestEvent = new TransportWriteRequest();
	transportRequestEvent->mFlowId = inRequest->mFlowId;
	ForwardBlock * fwdBlock = new ForwardBlock();
	fwdBlock->mTtl = inRequest->mTtl;
	fwdBlock->mCmdBuffer = new char[inRequest->mLength];
	memcpy(fwdBlock->mCmdBuffer, inRequest->mBuffer,inRequest->mLength);
	fwdBlock->mCmdLength = inRequest->mLength;
	fwdBlock->mDestLabelStack = inRequest->mDestLabelStack;
	fwdBlock->mSrcLabelStack = inRequest->mSrcLabelStack;
	transportRequestEvent->mBuffer = fwdBlock->toBuffer(transportRequestEvent->mLength);
	transportRequestEvent->mEventId = inRequest->mEventId;
	auto_ptr<EventInfo> requestPtr(transportRequestEvent);
	locTransport->handleEvent(requestPtr);

	//Need to delete the request
	delete inRequest;
	delete fwdBlock;
	return 0;
}


//Done
int ForwardingStage::handleConnectRequest(ForwardConnectRequest * inRequest){
	switch(inRequest->mType){
	case ForwardConnectRequest::TCP:
	{
		TcpTransportStage * transportStage = new TcpTransportStage(mAsyncStage, this);
		TransportConnectRequest * connectRequest = new TransportConnectRequest();
		connectRequest->mEventId = inRequest->mEventId;
		connectRequest->mAddress = inRequest->mAddress;
		auto_ptr<EventInfo> requestPtr(connectRequest);
		transportStage->handleEvent(requestPtr);			
		mFlowIdMap[transportStage->mFlowId] = transportStage;
		//TODO: the one function that returns something
		return transportStage->mFlowId;
		break;
	}
	case ForwardConnectRequest::UDP:
	{
		UdpTransportStage * transportStage = new UdpTransportStage(mAsyncStage, this);
		TransportConnectRequest * connectRequest = new TransportConnectRequest();
		connectRequest->mEventId = inRequest->mEventId;
		connectRequest->mAddress = inRequest->mAddress;
		auto_ptr<EventInfo> requestPtr(connectRequest);
		transportStage->handleEvent(requestPtr);			
		//TODO: the one function that returns something
		mFlowIdMap[transportStage->mFlowId] = transportStage;
		
		return transportStage->mFlowId;
		break;
	}
	default:
	{
		cout << "FORWADING: no transport for connect specified type" <<endl;
	}
	}
	delete inRequest;
	return 0;
}
int ForwardingStage::handleDisconnectRequest(ForwardDisconnectRequest *inRequest){
	TransportDisconnectRequest * request = new TransportDisconnectRequest();
	if (mFlowIdMap.find(inRequest->mFlowId) == mFlowIdMap.end())
	{
		delete inRequest;
		cout << "FORWADING: no transport for " << inRequest->mFlowId << endl;
		return 0;
	}
	GenericStage * locTransport = mFlowIdMap[inRequest->mFlowId];	
	auto_ptr<EventInfo> requestPtr(request);
	locTransport->handleEvent(requestPtr);
	
	delete inRequest;
	return 0;
}

//Done
int ForwardingStage::handleListenRequest(ForwardListenRequest *inRequest){
	//Check to see whether we already have a transport using this address
	switch(inRequest->mType){
	case ForwardListenRequest::TCP:{
		TcpTransportStage * transportStage = new TcpTransportStage(mAsyncStage, this);
		TransportListenRequest * request = new TransportListenRequest();
		request->mEventId = inRequest->mEventId;
		request->mAddress = inRequest->mAddress;
		auto_ptr<EventInfo> requestPtr(request);
		transportStage->handleEvent(requestPtr);
		break;
	}
	case ForwardListenRequest::UDP:{
		UdpTransportStage * transportStage = new UdpTransportStage(mAsyncStage, this);
		TransportListenRequest * request = new TransportListenRequest();
		request->mEventId = inRequest->mEventId;
		request->mAddress = inRequest->mAddress;
		auto_ptr<EventInfo> requestPtr(request);
		transportStage->handleEvent(requestPtr);
		mFlowIdMap[transportStage->mFlowId] = transportStage;
		break;
	}
	default:
		cout << "FORWARDING: no transport for listen request type" << endl;
	}
	delete inRequest;
	return 0;
}

int ForwardingStage::handleNotification(TransportNotification * inEvent)
{
	switch(inEvent->mEventType){
	case TransportNotification::CONNECT_COMPLETE:{
		ForwardNotification * notification = new ForwardNotification(inEvent->mEventSource);
		notification->mEventId = inEvent->mEventId;
		notification->mEventType = ForwardNotification::CONNECT_COMPLETE;
		notification->mAddress = inEvent->mAddress;
		notification->mFlowId = inEvent->mFlowId;
		auto_ptr<EventInfo> notifyPtr(notification);
		mDhtStage->handleEvent(notifyPtr);
		break;
	}	
	case TransportNotification::WRITE_COMPLETE:{
		ForwardNotification * notification = new ForwardNotification(inEvent->mEventSource);
		notification->mEventId = inEvent->mEventId;
		notification->mEventType = ForwardNotification::WRITE_COMPLETE;
		notification->mAddress = inEvent->mAddress;
		notification->mFlowId = inEvent->mFlowId;
		auto_ptr<EventInfo> notifyPtr(notification);
		mDhtStage->handleEvent(notifyPtr);
		break;
	}
	case TransportNotification::FAILURE:{
		ForwardNotification * notification = new ForwardNotification(inEvent->mEventSource);
		notification->mEventId = inEvent->mEventId;
		notification->mEventType = ForwardNotification::FAILURE;
		notification->mAddress = inEvent->mAddress;
		notification->mFlowId = inEvent->mFlowId;
		auto_ptr<EventInfo> notifyPtr(notification);
		mDhtStage->handleEvent(notifyPtr);
		
		mFlowIdMap.erase(inEvent->mFlowId);
		break;
	}
	}
	delete inEvent;
	return 0;
	
}

int ForwardingStage::handleListenCallback(TransportListenCallback * inCallback){
	mFlowIdMap[inCallback->mFlowId] = inCallback->mTransportStage;
	ForwardListenCallback * callback = new ForwardListenCallback();
	callback->mEventId = inCallback->mEventId;
	callback->mAddress = inCallback->mAddress;
	callback->mFlowId = inCallback->mFlowId;
	auto_ptr<EventInfo> callbackPtr(callback);
	mDhtStage->handleEvent(callbackPtr);
	
	delete inCallback;
	return 0;
}

ForwardReadCallback * generateReadCallback(TransportReadCallback * inCallback){
	ForwardReadCallback * callback = new ForwardReadCallback(inCallback->mEventSource);
	ForwardBlock * locFwdBlock = new ForwardBlock(inCallback->mLength, inCallback->mBuffer);
	callback->mFlowId = inCallback->mFlowId;
	callback->mCmdLength = locFwdBlock->mCmdLength;
	callback->mCmdBuffer = new char[locFwdBlock->mCmdLength];
	memcpy(callback->mCmdBuffer, locFwdBlock->mCmdBuffer, locFwdBlock->mCmdLength);
	callback->mDestLabelStack = locFwdBlock->mDestLabelStack;
	callback->mSrcLabelStack = locFwdBlock->mSrcLabelStack;
	callback->mEventId = inCallback->mEventId;
	callback->mAddress = inCallback->mAddress;
	callback->mTtl = locFwdBlock->mTtl;
	
	delete locFwdBlock;
	return callback;
}

int ForwardingStage::handleReadCallback(TransportReadCallback * inCallback){
	ForwardBlock * locFwdBlock = new ForwardBlock(inCallback->mLength, inCallback->mBuffer);
	if (locFwdBlock->mTtl == 0)
	{
		delete locFwdBlock;
		return 0;
	}
	
	//Message was sent to this peer with no further destinations, this must be the "end of the line"
	if (locFwdBlock->mDestLabelStack.size() == 0){
		ForwardReadCallback * callback = generateReadCallback(inCallback);
		auto_ptr<EventInfo> callbackPtr(callback);
		mDhtStage->handleEvent(callbackPtr);
	}
	//Front of the stack is a locus, have DHT deal with message
	else if (locFwdBlock->mDestLabelStack.front() == 1){
		ForwardReadCallback * callback = generateReadCallback(inCallback);
		auto_ptr<EventInfo> callbackPtr(callback);
		mDhtStage->handleEvent(callbackPtr);
	}
	//Otherwise, we have seen this message before. We inserted a flow id from which
	//the message originated, so send it back out on that flow
	else if (mFlowIdMap.find(locFwdBlock->mDestLabelStack.front()) != mFlowIdMap.end()){
		//Send out on the flow specified on the top of the destination stack
		unsigned int locFlowId = popLabel(locFwdBlock->mDestLabelStack);
		pushLabel(locFwdBlock->mSrcLabelStack, inCallback->mFlowId);
		locFwdBlock->mTtl = locFwdBlock->mTtl - 1;
		TransportWriteRequest * transportRequestEvent = new TransportWriteRequest();
		transportRequestEvent->mBuffer = locFwdBlock->toBuffer(transportRequestEvent->mLength);
		transportRequestEvent->mEventId = inCallback->mEventId;
		transportRequestEvent->mFlowId = locFlowId;
		auto_ptr<EventInfo> requestPtr(transportRequestEvent);
		mFlowIdMap[locFlowId]->handleEvent(requestPtr);
	}
	delete inCallback;
	delete locFwdBlock;
	return 0;
}