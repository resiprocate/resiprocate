#include "TcpTransportStage.h"

#define TRANSPORT_DEBUG 0

namespace p2p
{
using namespace std;

TcpTransportStage::TcpTransportStage()
{
	mState = TcpTransportStage::DISCONNECTED;
	mFlowId = p2p::NetworkTransportStage::generateFlowId();

}

TcpTransportStage::TcpTransportStage(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
	mState = TcpTransportStage::DISCONNECTED;
	mFlowId = p2p::NetworkTransportStage::generateFlowId();
}


TcpTransportStage::~TcpTransportStage()
{
	std::vector<TransportBufferInfo * >::iterator iter;
	TransportBufferInfo * buffer;
	for (iter = mOutgoingBuffers.begin();
		iter != mOutgoingBuffers.end();){
		buffer = *iter;
		iter = mOutgoingBuffers.erase(iter);
		delete (buffer);
	}
	for (iter = mIncomingBuffers.begin();
		iter != mIncomingBuffers.end();){
		buffer = *iter;
		iter = mIncomingBuffers.erase(iter);
		delete (buffer);
	}
	p2p::NetworkTransportStage::releaseFlowId(mFlowId);
}

TcpTransportStage::TransportBufferInfo::TransportBufferInfo()
{
	mBuffer = NULL;
	mIndex = 0;
	mLength = -1;
	mEventId = -1;
	mSource = NULL;
}

TcpTransportStage::TransportBufferInfo::~TransportBufferInfo()
{
	if (mBuffer)
		delete [] mBuffer;
}

int TcpTransportStage::enqueueEvent(std::auto_ptr<p2p::EventInfo> inEventInfo){

	return 0;
}


int TcpTransportStage::handleEvent(std::auto_ptr<p2p::EventInfo> inEventInfo){
	if (typeid(*inEventInfo) == typeid(p2p::TransportWriteRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportWriteRequest * transportRequestEvent = (p2p::TransportWriteRequest *) locEventInfo;
		handleWriteRequest(transportRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportConnectRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportConnectRequest * transportRequestEvent = (p2p::TransportConnectRequest *) locEventInfo;
		handleConnectRequest(transportRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportDisconnectRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportDisconnectRequest * transportRequestEvent = (p2p::TransportDisconnectRequest *) locEventInfo;
		handleDisconnectRequest(transportRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TransportListenRequest)){
		EventInfo * locEventInfo = inEventInfo.release();
		TransportListenRequest * transportRequestEvent = (p2p::TransportListenRequest *) locEventInfo;
		handleListenRequest(transportRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::NetworkCallbackEvent)){
		EventInfo * locEventInfo = inEventInfo.release();
		NetworkCallbackEvent * networkCallback = (p2p::NetworkCallbackEvent *) (locEventInfo);
		return handleNetworkCallback(networkCallback);
	}
	return 0;
}

int  TcpTransportStage::handleNetworkCallback(NetworkCallbackEvent * inCallback){
	switch(inCallback->mEventType){
	case(NetworkCallbackEvent::CONNECT):
		handleConnectCallback(inCallback);
		break;
	case(NetworkCallbackEvent::WRITE):
		handleWriteCallback(inCallback);
		break;
	case(NetworkCallbackEvent::READ):
		handleReadCallback(inCallback);
		break;
	case(NetworkCallbackEvent::LISTEN):
		handleListenCallback(inCallback);
		break;
	}
}

int TcpTransportStage::handleConnectCallback(NetworkCallbackEvent * inCallback){
	socklen_t len = sizeof(int);
	int val = -1;
	int stat = getsockopt (mSocket, SOL_SOCKET, SO_ERROR, &val, &len);
	bool error = false;
	if (stat < 0){
		error = true;
	}
	else{
		switch(val){
		case ECONNREFUSED:
			error = true;
			break;	
		default:
			break;
		}
	}
	if (error){
		TransportNotification * locNotification = new TransportNotification(this);
		locNotification->mEventType = TransportNotification::FAILURE;
		locNotification->mEventId = inCallback->mEventId;
		locNotification->mAddress = mPeerAddress;
		locNotification->mFlowId = mFlowId;
		auto_ptr<EventInfo> notificationPtr(locNotification);
		mForwardingStage->handleEvent(notificationPtr);

		delete inCallback;
		return NetworkRequestEvent::FAIL;
	}
	
	//Update the stage
	if (mState == DISCONNECTED)
		mState = CONNECTED;
	
	//If the outgoing buffers queue contains packets, issue write request
	if (!mOutgoingBuffers.empty()){
		NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
		locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
		locNetworkRequestEvent->mSocket = mSocket;
		auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
		mAsyncStage->handleEvent(requestPtr);
	}
	//Inform lower layer of desire to read
	//TODO: HERE??!
	NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
	locNetworkRequestEvent->mEventType = NetworkRequestEvent::READ;
	locNetworkRequestEvent->mSocket = mSocket;
	auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
	mAsyncStage->handleEvent(requestPtr);
	
	//Inform the higher layers of the completed connection
	TransportNotification * locNotification = new TransportNotification(this);
	locNotification->mEventType = TransportNotification::CONNECT_COMPLETE;
	locNotification->mEventId = inCallback->mEventId;
	locNotification->mAddress = mPeerAddress;
	locNotification->mFlowId = mFlowId;
	auto_ptr<EventInfo> notificationPtr(locNotification);
	mForwardingStage->handleEvent(notificationPtr);
	
	delete inCallback;
	return NetworkRequestEvent::COMPLETE;
}

int TcpTransportStage::handleRead(char * locReadBuffer, int locBytes){
	TransportBufferInfo * readBuffer;
	if (mIncomingBuffers.empty()){
		readBuffer = new TransportBufferInfo();
		readBuffer->mEventId = EventInfo::generateEventId();
		mIncomingBuffers.push_back(readBuffer);
	}
	else{
		readBuffer = mIncomingBuffers.back();
	}
	
	//New packet, we know nothing about length
	if (readBuffer->mLength == -1) {
		if (readBuffer->mIndex > LENGTH_OFFSET + LENGTH_SIZE) {
			cout << "THIS SHOULD NEVER HAPPEN HOW DID INDEX GET BIGGER"<< endl;
		}
		//We can extract length 
		if (readBuffer->mIndex + locBytes > LENGTH_OFFSET + LENGTH_SIZE) {
			int locLength;
			//No prior bytes were read in, just convert what we read in
			if (readBuffer->mBuffer == NULL) {
				memcpy(&locLength, locReadBuffer + LENGTH_OFFSET, LENGTH_SIZE);
				locLength = ntohl(locLength);
				//cout << "TCP: enough bytes, length is " << locLength << endl;
			}
			//Prior bytes were read in, need to append old buffer with new info
			else {
				char locCopyBuffer[LENGTH_OFFSET + LENGTH_SIZE];
				int locRemainder = LENGTH_OFFSET + LENGTH_SIZE - readBuffer->mIndex;
				
				//Copy prior bytes, then copy new bytes
				memcpy(locCopyBuffer, readBuffer->mBuffer, readBuffer->mIndex);
				memcpy(locCopyBuffer + readBuffer->mIndex, locReadBuffer, static_cast<unsigned int>(locRemainder));
				memcpy(&locLength, locCopyBuffer + LENGTH_OFFSET, LENGTH_SIZE);
				locLength = ntohl(locLength);
				//cout << "TCP: enough bytes including buffer, length is " << locLength << endl;

			}
			if (locLength <= 0){
				cout << "TCP: 0 length read"<< endl;
			}
			//Create ample buffer space, then delete old buffer
			readBuffer->mLength = locLength;
			char * locPtr = new char[readBuffer->mLength + TCP_HEADER_SIZE];
			//Copy over the old buffer
			if (readBuffer->mBuffer != NULL){
				memcpy(locPtr, readBuffer->mBuffer, readBuffer->mIndex);
				delete[]readBuffer->mBuffer;
			}
			readBuffer->mBuffer = locPtr;
		}
		//Else we still know nothing about length, all we can do is buffer new bytes
		else {
			if (readBuffer->mBuffer == NULL) {
				readBuffer->mBuffer = new char[locBytes];
				memcpy(readBuffer->mBuffer, locReadBuffer, locBytes);
				readBuffer->mIndex += locBytes;
			} else {
				//Create new buffer space for incoming bytes
				char * locPtr = new char[readBuffer->mIndex + locBytes];
				//Copy the old bytes into the new read buffer
				memcpy(locPtr, readBuffer->mBuffer, readBuffer->mIndex);
				memcpy(locPtr + readBuffer->mIndex, locReadBuffer, locBytes);
				delete[] readBuffer->mBuffer;
				readBuffer->mBuffer = locPtr;
				readBuffer->mIndex += locBytes;
			}
			//cout << "TCP: not enough bytes, buffering " << locBytes << endl;
			return locBytes;
		}
	}
	//Check to see if we have complete header and body, if not, copy into buffer and return
	if (locBytes + readBuffer->mIndex < TCP_HEADER_SIZE + readBuffer->mLength){
		memcpy(readBuffer->mBuffer + readBuffer->mIndex, locReadBuffer, static_cast<unsigned int>(locBytes));
		readBuffer->mIndex += locBytes;
		if (readBuffer != mIncomingBuffers.back()) {
			mIncomingBuffers.push_back(readBuffer);
		}
		return locBytes;
	}
	//We have either the exact number of bytes we want, or we have more
	else
	{	
		//We already allocated all the buffer space we need, so compute difference and copy in
		int locRemainder = TCP_HEADER_SIZE + readBuffer->mLength - readBuffer->mIndex;
		memcpy(readBuffer->mBuffer + readBuffer->mIndex, locReadBuffer, static_cast<unsigned int>(locRemainder));
		readBuffer->mIndex += locRemainder;
	
		//Finish copying in the bytes, issue callback, get rid of header bytes
		TransportReadCallback * readCallback = new TransportReadCallback(this);
		unsigned int bufSize = static_cast<unsigned int>(readBuffer->mLength);
		char * callbackBuffer = new char[bufSize];
		
		//Avoid copying in the header
		memcpy(callbackBuffer, readBuffer->mBuffer + TCP_HEADER_SIZE, readBuffer->mLength);
		readCallback->mLength = readBuffer->mLength;
		readCallback->mBuffer = callbackBuffer;
		readCallback->mAddress = mPeerAddress;
		readCallback->mFlowId = mFlowId;
		readCallback->mEventId = EventInfo::generateEventId();
	
		auto_ptr<EventInfo> notificationPtr(readCallback);
		mForwardingStage->handleEvent(notificationPtr);
		if (!mIncomingBuffers.empty()){
			if (readBuffer == mIncomingBuffers.back()) {
				std::vector<TransportBufferInfo *>::iterator iter;
				for (iter = mIncomingBuffers.begin(); iter != mIncomingBuffers.end(); iter++){
					if (*iter == readBuffer){
						mIncomingBuffers.erase(iter);
						break;
					}
				}
			}
		}
		delete readBuffer;
		return locRemainder;
	}
	return locBytes;
}

int TcpTransportStage::handleReadCallback(NetworkCallbackEvent * inCallback){
	if (mState != CONNECTED){
		cout << "TCP: reading, but we are not connected" << endl;
	}
	char * locReadBuffer = new char[10240];
	char * locReadPtr = locReadBuffer;
	memset(locReadBuffer, 0, 10240);
	int locBytes = recv(mSocket, locReadBuffer, 10240, 0);
	if (locBytes <= 0){
		perror("Read failed");
		TransportNotification * locNotification = new TransportNotification(this);
		locNotification->mEventType = TransportNotification::FAILURE;
		locNotification->mEventId = inCallback->mEventId;
		locNotification->mAddress = mPeerAddress;
		locNotification->mFlowId = mFlowId;
		auto_ptr<EventInfo> notificationPtr(locNotification);
		mForwardingStage->handleEvent(notificationPtr);

		delete [] locReadPtr;
		delete inCallback;
		return NetworkRequestEvent::FAIL;
	}
	else{
		while (true){
			//handleRead returns the number of bytes processed
			int locBytesProcessed = handleRead(locReadBuffer, locBytes);
			if (locBytesProcessed < locBytes){
				locReadBuffer = locReadBuffer + locBytesProcessed;
				locBytes -= locBytesProcessed;
			}
			else{
				break;
			}
		}
	}
	delete [] locReadPtr;
	delete inCallback;
	return NetworkRequestEvent::INCOMPLETE;
}

int TcpTransportStage::handleWriteCallback(NetworkCallbackEvent * inCallback){
	if (mState != CONNECTED){
		cout << "TCP: writing but not connected" << endl;
	}
	if (mOutgoingBuffers.empty()){
		cout << "TCP: writing but no outgoing buffers" << endl;
	}
	TransportBufferInfo * writeBuffer = mOutgoingBuffers.front();
	if (writeBuffer->mIndex == writeBuffer->mLength){
		cout << "TCP: found buffer that has finished writing out length but still in queue" << endl;
	}
	if (TRANSPORT_DEBUG)
		cout << "TRANSPORT: using socket [" << mSocket << "] to write" << endl;
	
	int locBytes = send(mSocket, 
				writeBuffer->mBuffer + writeBuffer->mIndex, 
				writeBuffer->mLength - writeBuffer->mIndex, 
				0);
	if (locBytes < 0){
		perror ("Transport write");
		TransportNotification * locNotification = new TransportNotification(this);
		locNotification->mEventType = TransportNotification::FAILURE;
		locNotification->mEventId = inCallback->mEventId;
		locNotification->mAddress = mPeerAddress;
		locNotification->mFlowId = mFlowId;
		auto_ptr<EventInfo> notificationPtr(locNotification);
		mForwardingStage->handleEvent(notificationPtr);

		delete inCallback;
		return NetworkRequestEvent::FAIL;	
	}
	else{
		if (locBytes + writeBuffer->mIndex == writeBuffer->mLength){
			mOutgoingBuffers.erase(mOutgoingBuffers.begin());			
			delete writeBuffer;
				
			TransportNotification * locNotification = new TransportNotification(this);
			locNotification->mEventType = TransportNotification::WRITE_COMPLETE;
			locNotification->mEventId = inCallback->mEventId;
			locNotification->mFlowId = mFlowId;
			auto_ptr<EventInfo> notificationPtr(locNotification);
			mForwardingStage->handleEvent(notificationPtr);
	
		}
		else{
			writeBuffer->mIndex += locBytes;
			cout << "TCP: did not finish writing, only wrote " << locBytes << " out of " << writeBuffer->mLength << endl;
		}
	}
	delete inCallback;
	if (mOutgoingBuffers.size() > 0)
		return NetworkRequestEvent::INCOMPLETE;
	else
		return NetworkRequestEvent::COMPLETE;
}

int TcpTransportStage::handleListenCallback(NetworkCallbackEvent * inCallback){
	Socket acceptedSocket;
	Address acceptedAddress;
	socklen_t acceptedLength = sizeof(acceptedAddress);
	acceptedSocket = accept(mSocket, (struct sockaddr *)&acceptedAddress, &acceptedLength);
	if (acceptedSocket <= 0){
		perror("Listen callback");
	}
	
	//Set up transport object
	TcpTransportStage * tcpTransportStage = new TcpTransportStage(mAsyncStage, mForwardingStage);
	tcpTransportStage->mSocket = acceptedSocket;
	memcpy(&tcpTransportStage->mSelfAddress, &mSelfAddress, sizeof(Address));
	memcpy(&tcpTransportStage->mPeerAddress, &acceptedAddress, sizeof(Address));
	tcpTransportStage->mState = CONNECTED;
	
	//Inform higher layers of new connection	
	TransportListenCallback * listenCallback = new TransportListenCallback(this);
	listenCallback->mFlowId = this->mFlowId;
	listenCallback->mEventId = inCallback->mEventId;
	listenCallback->mTransportStage = tcpTransportStage;
	listenCallback->mAddress = tcpTransportStage->mPeerAddress;
	auto_ptr<EventInfo> notificationPtr(listenCallback);
	mForwardingStage->handleEvent(notificationPtr);
	
	//TODO: HERE??! inform lower layer about new tcp stage reading
	NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(tcpTransportStage);
	locNetworkRequestEvent->mEventType = NetworkRequestEvent::READ;
	locNetworkRequestEvent->mSocket = acceptedSocket;
	auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
	mAsyncStage->handleEvent(requestPtr);
	
	//Want to continue listening for new connection requests
	delete inCallback;
	return NetworkRequestEvent::INCOMPLETE;
}

int TcpTransportStage::handleWriteRequest(TransportWriteRequest * inTransportRequestEvent){	
	//Create buffer info object
	TransportBufferInfo * bufferInfo = new TransportBufferInfo();
	bufferInfo->mIndex = 0;
	
	//Create a new buffer to additionally hold our header information
	char * locBufferPtr = new char [inTransportRequestEvent->mLength + TCP_HEADER_SIZE];
	int locSize = htonl(inTransportRequestEvent->mLength);
	memcpy(locBufferPtr+LENGTH_OFFSET, &locSize, LENGTH_SIZE);
	memcpy(locBufferPtr+TCP_HEADER_SIZE, inTransportRequestEvent->mBuffer, inTransportRequestEvent->mLength);
	bufferInfo->mBuffer = locBufferPtr;
	bufferInfo->mEventId = inTransportRequestEvent->mEventId;
	bufferInfo->mLength = inTransportRequestEvent->mLength + TCP_HEADER_SIZE;
		
	delete inTransportRequestEvent;
		
	if (mState == TcpTransportStage::CONNECTED){
		//AsyncStage not aware of this write socket
		if (mOutgoingBuffers.empty())
		{
			//Create request
			mOutgoingBuffers.push_back(bufferInfo);
			NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
			locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
			locNetworkRequestEvent->mSocket = mSocket;
			locNetworkRequestEvent->mEventId = inTransportRequestEvent->mEventId;
			auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
			mAsyncStage->handleEvent(requestPtr);
		}
		else {
			mOutgoingBuffers.push_back(bufferInfo);
		}
	}
	else{
		mOutgoingBuffers.push_back(bufferInfo);		
	}
	return 0;
}

int TcpTransportStage::handleConnectRequest(TransportConnectRequest * inTransportRequestEvent){
	if (mState == TcpTransportStage::DISCONNECTED) 
	{
		int flag = 1;
		mSocket = socket(PF_INET, SOCK_STREAM, 0); 
		p2p::makeSocketNonBlocking(mSocket);
		if(setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
			perror("setsockopt");
			exit(1);
		}

		//Issue the connect to the specified address
		int locErr = connect(mSocket,
							(struct sockaddr *) &inTransportRequestEvent->mAddress,
							sizeof(struct sockaddr));
		if (locErr == -1) 
		{
			switch (getErrno()) 
			{
			case EINPROGRESS:
			case EWOULDBLOCK:
				break;
			default:
				TransportNotification * locNotification = new TransportNotification(this);
				locNotification->mEventType = TransportNotification::FAILURE;
				locNotification->mEventId = inTransportRequestEvent->mEventId;
				locNotification->mAddress = mPeerAddress;
				locNotification->mFlowId = mFlowId;
				auto_ptr<EventInfo> notificationPtr(locNotification);
				mForwardingStage->handleEvent(notificationPtr);
				delete inTransportRequestEvent;				
				
				cout << "TCP: connect failed"<< endl;
				return 0;
			}
		}
		
		//Create request
		mPeerAddress = inTransportRequestEvent->mAddress;
		NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
		locNetworkRequestEvent->mEventId = inTransportRequestEvent->mEventId;
		locNetworkRequestEvent->mEventType = NetworkRequestEvent::CONNECT;
		locNetworkRequestEvent->mSocket = mSocket;
		auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
		mAsyncStage->handleEvent(requestPtr);
		
		delete inTransportRequestEvent;
		return 0;
	} 
	else {
		cout << "TRANSPORT: already connected, so why are you asking us to connect again?"<< endl;
		delete inTransportRequestEvent;
		return 0;
	}
}

int TcpTransportStage::handleDisconnectRequest(TransportDisconnectRequest * inTransportRequestEvent){
	close(mSocket);
	mState = DISCONNECTED;
	delete inTransportRequestEvent;
	return 0;
}

int TcpTransportStage::handleListenRequest(TransportListenRequest * inTransportRequestEvent){
	//begin listening on socket
	if (mState == DISCONNECTED){
		mSocket = socket(PF_INET, SOCK_STREAM, 0); 
		p2p::makeSocketNonBlocking(mSocket);
		
		int flag = 1;
		if(setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
			perror("setsockopt");
			exit(1);
		}
		
	    if (bind(mSocket, (struct sockaddr *)&inTransportRequestEvent->mAddress, sizeof(struct sockaddr)) < 0){
	      cout << "TCP: bind error" <<endl;
	    }
	    
		if (listen(mSocket,128) == -1)
		{
			cout << "TCP: listen error" << endl;
		}
		mSelfAddress = inTransportRequestEvent->mAddress;
		
		//Create request
		NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
		locNetworkRequestEvent->mEventType = NetworkRequestEvent::LISTEN;
		locNetworkRequestEvent->mSocket = mSocket;
		auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
		mAsyncStage->handleEvent(requestPtr);
		
	}		
	delete inTransportRequestEvent;
	return 0;
}

}
