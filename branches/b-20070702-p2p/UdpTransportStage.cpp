#include "UdpTransportStage.h"

namespace p2p
{
using namespace std;
UdpTransportStage::UdpTransportStage()
{
	mPeerSeq = 0;
	mSelfSeq = 0;
	mState = UdpTransportStage::DISCONNECTED;
	mFlowId = p2p::NetworkTransportStage::generateFlowId();
}

UdpTransportStage::UdpTransportStage(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mPeerSeq = 0;
	mSelfSeq = 0;
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
	mState = UdpTransportStage::DISCONNECTED;
	mFlowId = p2p::NetworkTransportStage::generateFlowId();
}


UdpTransportStage::~UdpTransportStage()
{
	std::vector<TransportBufferInfo * >::iterator iter;
	TransportBufferInfo * buffer;
	for (iter = mOutgoingBuffers.begin();
		iter != mOutgoingBuffers.end();){
		buffer = *iter;
		iter = mOutgoingBuffers.erase(iter);
		delete (buffer);
	}
	p2p::NetworkTransportStage::releaseFlowId(mFlowId);
}

UdpTransportStage::TransportMessageInfo::~TransportMessageInfo()
{
	std::vector<TransportBufferInfo * >::iterator iter;
	TransportBufferInfo * buffer;
	for (iter = mIncomingBuffers.begin(); iter != mIncomingBuffers.end();){
		buffer = *iter;
		iter = mIncomingBuffers.erase(iter);
		delete (buffer);
	}
}

UdpTransportStage::TransportBufferInfo::TransportBufferInfo()
{
	mBuffer = NULL;
	mSeqNum = 0;
	mLength = -1;
	mEventId = -1;
	mSource = NULL;
}

UdpTransportStage::TransportBufferInfo::~TransportBufferInfo()
{
	if (mBuffer)
		delete [] mBuffer;
}

int UdpTransportStage::enqueueEvent(std::auto_ptr<p2p::EventInfo> inEventInfo)
{

	return 0;
}

int UdpTransportStage::handleEvent(std::auto_ptr<p2p::EventInfo> inEventInfo)
{
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
	else if (typeid(*inEventInfo) == typeid(p2p::TimerCallbackEvent)){
		EventInfo * locEventInfo = inEventInfo.release();
		TimerCallbackEvent * timerCallback = (p2p::TimerCallbackEvent *) (locEventInfo);
		return handleTimer(timerCallback);
	}
	return 0;
}


int UdpTransportStage::handleNetworkCallback(NetworkCallbackEvent * inCallback){
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


int UdpTransportStage::handleTimer(TimerCallbackEvent * inCallback)
{
	timeval currentTime;
	std::vector<TransportBufferInfo *>::iterator iter;
	TransportBufferInfo * buffer;
	for (iter = mRetransmitBuffers.begin(); iter != mRetransmitBuffers.end(); )
	{
		buffer = (*iter);
		gettimeofday(&currentTime, NULL);
		//We have yet to receive an ack for this buffer
		if (computeTimeElapsed(currentTime, buffer->mTime) >= UDP_DEFAULT_TIMEOUT_MICROSECONDS)
		{
			//This buffer has used up its nine lives, assume the "connection" has failed
			if (buffer->mSentTimes >= UDP_DEFAULT_MAX_SEND_TIMES)
			{
				UdpPeerInfo * peer = lookupByFlowId(buffer->mFlowId);
				if (peer != NULL)
				{
					//This peer has NOT already been noted as having failed, notify upper layers
					//We don't want to notify them multiple times for the same failed node
					//TODO: need to clean up failed udp peers
					if (peer->mConnectionStatus != UdpPeerInfo::FAILED)
					{
						failUdpPeer(peer);
						TransportNotification * locNotification = new TransportNotification(this);
						locNotification->mEventType = TransportNotification::FAILURE;
						locNotification->mEventId = inCallback->mEventId;
						locNotification->mAddress = buffer->mAddress;
						locNotification->mFlowId = buffer->mFlowId;
						auto_ptr<EventInfo> notificationPtr(locNotification);
						mForwardingStage->handleEvent(notificationPtr);
					}
				}
				iter = mRetransmitBuffers.erase(iter);
				delete buffer;		
			}
			else
			{	
				cout << "UDP: retransmitting buffer after " << computeTimeElapsed(currentTime, buffer->mTime);
				cout << buffer->mFlowId << ":" << buffer->mMsgId << ":" << buffer->mSeqNum << ":" << buffer->mLength << endl;
				
				if (mOutgoingBuffers.empty())
				{
					NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
					locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
					locNetworkRequestEvent->mSocket = mSocket;
					auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
					mAsyncStage->handleEvent(requestPtr);
				}
				mOutgoingBuffers.push_back(buffer);	
				iter++;
			}
		}
		else
		{
			iter++;
		}
	}
	delete inCallback;	

	//Lastly, renew timer
	if (!mRetransmitBuffers.empty())
	{
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = UDP_DEFAULT_TIMEOUT_SECONDS;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::UDP_TIMEOUT;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
	}
}

int UdpTransportStage::failUdpPeer(UdpPeerInfo * peer)
{
	//Go through outgoing buffers and delete any buffers matching the peer
	std::vector<TransportBufferInfo *>::iterator bufIter;
	TransportBufferInfo * buffer;
	for (bufIter = mOutgoingBuffers.begin(); bufIter != mOutgoingBuffers.end();) 
	{
		buffer = (*bufIter);
		if (buffer->mFlowId == peer->mFlowId)
		{
			bufIter = mOutgoingBuffers.erase(bufIter);
			delete buffer;
		}
		else
		{
			bufIter++;
		}
	}	
	peer->clearIncomingMessages();
	peer->mConnectionStatus = UdpPeerInfo::FAILED;
	
	//Right now, this is literally just a dummy map to keep track of acked peers
	peer->mAckedBuffers.clear();
	return 0;
}

void UdpTransportStage::removeRetransmitBuffer(Address inAddress, unsigned int messageId, unsigned int seqNumber)
{
	std::vector<TransportBufferInfo *>::iterator iter;
	TransportBufferInfo * buffer = NULL;
	for (iter = mOutgoingBuffers.begin(); iter != mOutgoingBuffers.end();) 
	{
		if(((*iter)->mAddress.sin_port == inAddress.sin_port &&
			(*iter)->mAddress.sin_addr.s_addr == inAddress.sin_addr.s_addr) &&
			(*iter)->mMsgId == messageId && 
			(*iter)->mSeqNum == seqNumber && 
			(*iter)->mRetransmit == true)
		{			
			if (buffer != NULL)
			{
				assert(*iter == buffer);
			}
			else
			{
				buffer = *iter;
			}
			iter = mOutgoingBuffers.erase(iter);
		}
		else
		{
			iter++;
		}
	}	
	
	for (iter = mRetransmitBuffers.begin(); iter != mRetransmitBuffers.end();)
	{
		if(((*iter)->mAddress.sin_port == inAddress.sin_port &&
			(*iter)->mAddress.sin_addr.s_addr == inAddress.sin_addr.s_addr) &&
			(*iter)->mMsgId == messageId && 
			(*iter)->mSeqNum == seqNumber &&
			(*iter)->mRetransmit == true)
		{
			if (buffer != NULL)
			{
				assert(*iter == buffer);
			}
			else
			{
				buffer = *iter;
			}
			iter = mRetransmitBuffers.erase(iter);
		}
		else
		{
			iter++;
		}
	}
	if (buffer == NULL)
	{
		/*
		cout << ">>>>>>>>>>> Buffer not found";
		cout << messageId << ":" << seqNumber << endl;
		for (unsigned int i = 0; i < mRetransmitBuffers.size(); i++)
		{
			buffer = mRetransmitBuffers[i];
			cout << "	Remaining retransmit buffer ";
			cout << buffer->mFlowId << ":" << buffer->mMsgId << ":" << buffer->mSeqNum << ":" << buffer->mLength << endl;
		}
		*/
	}
	else	
	{
		/*
		cout << "Removed buffer ";
		cout << buffer->mFlowId << ":" << buffer->mMsgId << ":" << buffer->mSeqNum << ":" << buffer->mLength << endl;
		*/
		delete buffer;
	}
}

void UdpTransportStage::insertRetransmitBuffer(TransportBufferInfo * buffer)
{
	mRetransmitBuffers.push_back(buffer);
}


UdpTransportStage::UdpPeerInfo * UdpTransportStage::lookupByFlowId(unsigned int inFlowId)
{
	HashMap<Address, struct UdpPeerInfo *, p2p::AddressHash,  p2p::AddressCmp>::iterator iter;
	for (iter = mUdpPeerMap.begin(); iter != mUdpPeerMap.end(); iter++)
	{
		if ((iter->second)->mFlowId == inFlowId)
			return iter->second;
	}	
	return NULL;
}

UdpTransportStage::UdpPeerInfo * UdpTransportStage::lookupByAddress(Address inAddress)
{
	return mUdpPeerMap[inAddress];
}

void UdpTransportStage::insertUdpPeer(UdpPeerInfo * inPeer)
{
	assert(inPeer != NULL);
	mUdpPeerMap[inPeer->mAddress] = inPeer;
}


int UdpTransportStage::handleConnectCallback(NetworkCallbackEvent * inCallback)
{
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

UdpTransportStage::TransportBufferInfo * UdpTransportStage::generateAck(Address inAddress, unsigned int messageId, unsigned int seqNumber)
{
	unsigned int locId = htonl(messageId);
	unsigned int locSeq = htonl(seqNumber);
	char * buffer = new char[UDP_ACK_SIZE];
	memcpy(buffer + TYPE_OFFSET, &UDP_ACK_TYPE, TYPE_SIZE);
	memcpy(buffer + UDP_MESSAGE_ID_OFFSET, &locId, UDP_MESSAGE_ID_SIZE );
	memcpy(buffer + UDP_SEQUENCE_NUMBER_OFFSET, &locSeq, UDP_SEQUENCE_NUMBER_SIZE);
	TransportBufferInfo * bufferInfo = new TransportBufferInfo();
	bufferInfo->mBuffer = buffer;
	bufferInfo->mLength = UDP_ACK_SIZE;
	bufferInfo->mMsgId = messageId;
	bufferInfo->mSeqNum = seqNumber;
	bufferInfo->mRetransmit = false;
	bufferInfo->mAddress = inAddress;
	return bufferInfo;
}

int UdpTransportStage::handleReadCallback(NetworkCallbackEvent * inCallback)
{
	struct sockaddr_in peerAddress; // connector's address information
	socklen_t addressLength;
	int numBytes;
	char buf[UDP_MAX_PKT_SIZE];
	memset (buf, '\0', UDP_MAX_PKT_SIZE);

	//Receive bytes from peer
	addressLength = sizeof(struct sockaddr);
	if ((numBytes=recvfrom(mSocket, buf, UDP_MAX_PKT_SIZE, 0,
			(struct sockaddr *)&peerAddress, &addressLength)) <= 0) {
		perror("recvfrom");
		TransportNotification * locNotification = new TransportNotification(this);
		locNotification->mEventType = TransportNotification::FAILURE;
		locNotification->mEventId = inCallback->mEventId;
		locNotification->mAddress = peerAddress;
		locNotification->mFlowId = mFlowId;
		auto_ptr<EventInfo> notificationPtr(locNotification);
		mForwardingStage->handleEvent(notificationPtr);
		delete inCallback;
		return NetworkRequestEvent::INCOMPLETE;
	}
	assert(numBytes >= UDP_HEADER_SIZE);
	
	//Read the UDP header:
	unsigned char messageType;
	unsigned int messageId;
	unsigned int seqNumber;
	memcpy(&messageType, buf + TYPE_OFFSET, TYPE_SIZE);
	memcpy(&messageId, buf + UDP_MESSAGE_ID_OFFSET, UDP_MESSAGE_ID_SIZE);
	memcpy(&seqNumber, buf + UDP_SEQUENCE_NUMBER_OFFSET, UDP_SEQUENCE_NUMBER_SIZE);
	messageId = ntohl(messageId);
	seqNumber = ntohl(seqNumber);
	
	//Look up this particular udp peer. If the peer is not found, then generate a 
	//listen callback event, since this is a new peer we are hearing from.
	//Also, generate a new flow id.
	UdpPeerInfo * udpPeer; 	    
	if ((udpPeer = lookupByAddress(peerAddress)) == NULL) {
		udpPeer = new UdpPeerInfo();
		udpPeer->mAddress = peerAddress;
		udpPeer->mFlowId = p2p::NetworkTransportStage::generateFlowId();
		insertUdpPeer(udpPeer);
		
		//Inform higher layers of new connection	
		TransportListenCallback * listenCallback = new TransportListenCallback(this);
		listenCallback->mEventId = inCallback->mEventId;
		listenCallback->mTransportStage = this;
		listenCallback->mAddress = peerAddress;
		listenCallback->mFlowId = udpPeer->mFlowId;
		auto_ptr<EventInfo> notificationPtr(listenCallback);
		mForwardingStage->handleEvent(notificationPtr);
	}
	
	//This is an ack
	if (messageType == UDP_ACK_TYPE)
	{	
		//cout << "<------- Packet" << messageId << ":" << seqNumber << " is MY OWN ack " << endl;
		removeRetransmitBuffer(udpPeer->mAddress, messageId, seqNumber);
		delete inCallback;
		return NetworkRequestEvent::INCOMPLETE;
	}
	
	//Check whether this is an already acked buffer
	if (udpPeer->mAckedBuffers.find(seqNumber) != udpPeer->mAckedBuffers.end())
	{
		if (mOutgoingBuffers.empty())
		{
			//Create request
			NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
			locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
			locNetworkRequestEvent->mSocket = mSocket;
			locNetworkRequestEvent->mEventId = EventInfo::generateEventId();
			auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
			mAsyncStage->handleEvent(requestPtr);
		}
		
		//Send back ack
		//cout << "----------> We have already acked " << messageId << ":" << seqNumber << endl;

		TransportBufferInfo * buffer = generateAck(peerAddress, messageId, seqNumber);
		buffer->mFlowId = udpPeer->mFlowId;
		mOutgoingBuffers.push_back(buffer);

		delete inCallback;
		return NetworkRequestEvent::INCOMPLETE;
	}
	
	//At this point, the message is either a header, or a data packet
	if (messageType == UDP_MESSAGE_LENGTH_TYPE)
	{
		//cout << "----------> Is a message length type " << messageId << ":" << seqNumber << endl;

		assert(numBytes == UDP_MESSAGE_LENGTH_HEADER_SIZE);
		unsigned int messageSize;	
		memcpy(&messageSize, buf + UDP_MESSAGE_LENGTH_OFFSET, UDP_MESSAGE_LENGTH_SIZE);
		messageSize = ntohl(messageSize);
		TransportMessageInfo * messageInfo;
		if (udpPeer->mIncomingMessages.find(messageId) == udpPeer->mIncomingMessages.end())
		{
			//First time we encountered this message
			messageInfo = new TransportMessageInfo();
			messageInfo->mMessageId = messageId;
			messageInfo->mMessageSize = messageSize;
			udpPeer->mIncomingMessages[messageId] = messageInfo;
		}
		else
		{
			//A data packet arrived before the header arrived, just set up the information
			messageInfo = udpPeer->mIncomingMessages[messageId];
			messageInfo->mMessageId = messageId;
			messageInfo->mMessageSize = messageSize;
		}
		unsigned int dataSum = 0;
		for (unsigned int i = 0; i < messageInfo->mIncomingBuffers.size(); i++)
		{
			dataSum += messageInfo->mIncomingBuffers[i]->mLength;
		}
		
		if (dataSum == messageInfo->mMessageSize)
		{
			//Finish copying in the bytes, issue callback, get rid of header bytes
			TransportReadCallback * readCallback = new TransportReadCallback(this);
			unsigned int bufSize = static_cast<unsigned int>(messageInfo->mMessageSize);
			char * callbackBuffer = new char[bufSize];
			memset(callbackBuffer, 0, bufSize);
				
			//Sort the list and copy in the data
			sort(messageInfo->mIncomingBuffers.begin(), messageInfo->mIncomingBuffers.end(), p2p::UdpTransportStage::SequenceCompare);
			unsigned int offset = 0;
			for (unsigned int i = 0; i < messageInfo->mIncomingBuffers.size(); i++)
			{
				memcpy(callbackBuffer + offset, messageInfo->mIncomingBuffers[i]->mBuffer, messageInfo->mIncomingBuffers[i]->mLength);
				offset += messageInfo->mIncomingBuffers[i]->mLength;
			}
			readCallback->mFlowId = udpPeer->mFlowId;
			readCallback->mLength = bufSize;
			readCallback->mBuffer = callbackBuffer;
			readCallback->mAddress = udpPeer->mAddress;
			readCallback->mEventId = EventInfo::generateEventId();	
			auto_ptr<EventInfo> notificationPtr(readCallback);
			mForwardingStage->handleEvent(notificationPtr);
			udpPeer->clearIncomingMessage(messageId);
		}
	}
	else
	{
//		cout << "---------->Is a data type " << messageId << ":" << seqNumber << endl;

		unsigned int dataSize;	
		memcpy(&dataSize, buf + UDP_DATA_LENGTH_OFFSET, UDP_DATA_LENGTH_SIZE);
		dataSize = ntohl(dataSize);
		assert(static_cast<unsigned int>(numBytes) == UDP_DATA_HEADER_SIZE + dataSize);

		TransportMessageInfo * messageInfo;
		TransportBufferInfo * transportBuffer = new TransportBufferInfo();
		transportBuffer->mBuffer = new char [dataSize];
		memcpy(transportBuffer->mBuffer, buf + UDP_DATA_HEADER_SIZE, dataSize);
		transportBuffer->mLength = dataSize;		
		transportBuffer->mSeqNum = seqNumber;
		transportBuffer->mMsgId = messageId;
		transportBuffer->mAddress = udpPeer->mAddress;
			
		if (udpPeer->mIncomingMessages.find(messageId) == udpPeer->mIncomingMessages.end())
		{
			//This is the first data packet to arrive for this peer, no header is present
			messageInfo = new TransportMessageInfo();
			messageInfo->mMessageId = messageId;
			udpPeer->mIncomingMessages[messageId] = messageInfo;
			messageInfo->mIncomingBuffers.push_back(transportBuffer);
		}
		else
		{
			messageInfo = udpPeer->mIncomingMessages[messageId];
			messageInfo->mIncomingBuffers.push_back(transportBuffer);
			
			unsigned int dataSum = 0;
			for (unsigned int i = 0; i < messageInfo->mIncomingBuffers.size(); i++)
			{
				dataSum += messageInfo->mIncomingBuffers[i]->mLength;
			}
			if (dataSum == messageInfo->mMessageSize)
			{
				//Finish copying in the bytes, issue callback, get rid of header bytes
				TransportReadCallback * readCallback = new TransportReadCallback(this);
				unsigned int bufSize = static_cast<unsigned int>(messageInfo->mMessageSize);
				char * callbackBuffer = new char[bufSize];
				unsigned int offset = 0;
				sort(messageInfo->mIncomingBuffers.begin(), messageInfo->mIncomingBuffers.end(), p2p::UdpTransportStage::SequenceCompare);
				for (unsigned int i = 0; i < messageInfo->mIncomingBuffers.size(); i++)
				{
					memcpy(callbackBuffer + offset, messageInfo->mIncomingBuffers[i]->mBuffer, messageInfo->mIncomingBuffers[i]->mLength);
					offset += messageInfo->mIncomingBuffers[i]->mLength;
				}
				readCallback->mFlowId = udpPeer->mFlowId;
				readCallback->mLength = bufSize;
				readCallback->mBuffer = callbackBuffer;
				readCallback->mAddress = udpPeer->mAddress;
				readCallback->mEventId = EventInfo::generateEventId();	
				auto_ptr<EventInfo> notificationPtr(readCallback);
				mForwardingStage->handleEvent(notificationPtr);
				udpPeer->clearIncomingMessage(messageId);
			}
		}
	}
	 
	//Lastly, send back an ack for this packet
	udpPeer->mAckedBuffers[seqNumber] = NULL;
	TransportBufferInfo * buffer = generateAck(peerAddress, messageId, seqNumber);
	buffer->mFlowId = udpPeer->mFlowId;
	if (mOutgoingBuffers.empty())
	{
		//Create request
		NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
		locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
		locNetworkRequestEvent->mSocket = mSocket;
		locNetworkRequestEvent->mEventId = EventInfo::generateEventId();
		auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
		mAsyncStage->handleEvent(requestPtr);
	}
	mOutgoingBuffers.push_back(buffer);

	return NetworkRequestEvent::INCOMPLETE;
}

int UdpTransportStage::handleWriteCallback(NetworkCallbackEvent * inCallback)
{
	if (mOutgoingBuffers.empty())
	{
		return NetworkRequestEvent::COMPLETE;
	}
	
	TransportBufferInfo * bufferInfo = mOutgoingBuffers.front();
	ssize_t n = sendto(mSocket,  bufferInfo->mBuffer, bufferInfo->mLength, 0,
			(struct sockaddr *)& bufferInfo->mAddress, sizeof(struct sockaddr));
	
	if (n <0) 
	{
		cout << "UDP: failed to send to " << ntohs(bufferInfo->mAddress.sin_port) << ":" << bufferInfo->mFlowId << endl;
		perror("sendto");
		exit(1);
	}
	else
	{/*
		if (bufferInfo->mRetransmit)
			cout << "---------> sent out  " << bufferInfo->mMsgId << ":" << bufferInfo->mSeqNum << ":" << bufferInfo->mLength << endl;
		else
			cout << "=========> acked  " << bufferInfo->mMsgId << ":" << bufferInfo->mSeqNum << ":" << bufferInfo->mLength << endl;
	*/
	}
	assert(n == bufferInfo->mLength);
	mOutgoingBuffers.erase(mOutgoingBuffers.begin());
	
	if (bufferInfo->mRetransmit == true)
	{
		//If we've never retransmitted this before, insert into retransmit queue
		if (bufferInfo->mSentTimes == 0){
			insertRetransmitBuffer(bufferInfo);			
		}
		//Update number of times sent, and last sent time
		bufferInfo->mSentTimes++;
		gettimeofday(&bufferInfo->mTime, NULL);
		
		if (mRetransmitBuffers.size() == 1)
		{	
			TimerRequestEvent * timer = new TimerRequestEvent(this);
			timer->mExpireTime.tv_sec = UDP_DEFAULT_TIMEOUT_SECONDS;
			timer->mExpireTime.tv_usec = 0;
			timer->mTimerType = TimerRequestEvent::UDP_TIMEOUT;
			auto_ptr<EventInfo> timerPtr(timer);
			mAsyncStage->handleEvent(timerPtr);
		}
	}
	else
	{
		delete bufferInfo;
	}
	delete inCallback;
	
	if (mOutgoingBuffers.empty())
	{
		return NetworkRequestEvent::COMPLETE;
	}
	else
	{
		return NetworkRequestEvent::INCOMPLETE;
	}

}

int UdpTransportStage::handleListenCallback(NetworkCallbackEvent * inCallback)
{
	return NetworkRequestEvent::COMPLETE;
}	

int UdpTransportStage::handleWriteRequest(TransportWriteRequest * inTransportRequestEvent)
{
	if (mState == UdpTransportStage::CONNECTED){
		//AsyncStage not aware of this write socket
		if (mOutgoingBuffers.empty())
		{
			//Create request
			NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
			locNetworkRequestEvent->mEventType = NetworkRequestEvent::WRITE;
			locNetworkRequestEvent->mSocket = mSocket;
			locNetworkRequestEvent->mEventId = inTransportRequestEvent->mEventId;
			auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
			mAsyncStage->handleEvent(requestPtr);
		}
	}
	
	UdpPeerInfo * peerInfo = lookupByFlowId(inTransportRequestEvent->mFlowId);
	if (peerInfo == NULL)
	{
		cout << "UDP: cannot write out buffer, no flow id for " << inTransportRequestEvent->mFlowId << endl;
		delete inTransportRequestEvent;
		return 0;
	}
		
	unsigned int messageId = peerInfo->mMsgId++;
	unsigned int seqNumber = peerInfo->mSeqNum++;
	unsigned int messageLength = static_cast<unsigned int>(inTransportRequestEvent->mLength);
	int numBuffers = ceil(static_cast<float>(messageLength)/static_cast<float>(UDP_MAX_DATA_SIZE));
	
	//Create the header packet
	TransportBufferInfo * bufferInfo = new TransportBufferInfo();
	unsigned int locId = htonl(messageId);
	unsigned int locSeq = htonl(seqNumber);
	unsigned int locSize = htonl(messageLength);
	char * locBufferPtr = new char [UDP_MESSAGE_LENGTH_HEADER_SIZE];
	memcpy(locBufferPtr + TYPE_OFFSET, &UDP_MESSAGE_LENGTH_TYPE, TYPE_SIZE);
	memcpy(locBufferPtr + UDP_MESSAGE_ID_OFFSET, &locId, UDP_MESSAGE_ID_SIZE);
	memcpy(locBufferPtr + UDP_SEQUENCE_NUMBER_OFFSET, &locSeq, UDP_SEQUENCE_NUMBER_SIZE);
	memcpy(locBufferPtr + UDP_MESSAGE_LENGTH_OFFSET, &locSize, UDP_MESSAGE_LENGTH_SIZE);
	bufferInfo->mBuffer = locBufferPtr;
	bufferInfo->mEventId = inTransportRequestEvent->mEventId;
	bufferInfo->mLength = UDP_MESSAGE_LENGTH_HEADER_SIZE;
	bufferInfo->mAddress = peerInfo->mAddress;
	bufferInfo->mSeqNum = seqNumber;
	bufferInfo->mMsgId = messageId;
	bufferInfo->mRetransmit = true;
	bufferInfo->mSentTimes = 0;
	bufferInfo->mFlowId = peerInfo->mFlowId;
	mOutgoingBuffers.push_back(bufferInfo);
	
	//Go through and create data packets
	unsigned int offset = 0;
	unsigned int remainingLength = messageLength;
	unsigned int bufferLength = UDP_MAX_DATA_SIZE;
	for (int i = 0; i < numBuffers; i++)
	{
		assert(remainingLength > 0);
		if (remainingLength < static_cast<unsigned int>(UDP_MAX_DATA_SIZE))
		{
			bufferLength = remainingLength;
			remainingLength = 0;
		}
		else
		{
			bufferLength = UDP_MAX_DATA_SIZE;
			remainingLength -= bufferLength;
		}
		
		//Generate a sequence number
		seqNumber = peerInfo->mSeqNum++;
		bufferInfo = new TransportBufferInfo();
		locId = htonl(messageId);
		locSeq = htonl(seqNumber);
		locSize = htonl(bufferLength);
		char * locBufferPtr = new char [UDP_DATA_HEADER_SIZE + bufferLength];
		memcpy(locBufferPtr + TYPE_OFFSET, &UDP_DATA_TYPE, TYPE_SIZE);
		memcpy(locBufferPtr + UDP_MESSAGE_ID_OFFSET, &locId, UDP_MESSAGE_ID_SIZE);
		memcpy(locBufferPtr + UDP_SEQUENCE_NUMBER_OFFSET, &locSeq, UDP_SEQUENCE_NUMBER_SIZE);
		memcpy(locBufferPtr + UDP_DATA_LENGTH_OFFSET, &locSize, UDP_DATA_LENGTH_SIZE);
		memcpy(locBufferPtr + UDP_DATA_HEADER_SIZE, inTransportRequestEvent->mBuffer + offset, bufferLength);
		bufferInfo->mBuffer = locBufferPtr;
		bufferInfo->mEventId = inTransportRequestEvent->mEventId;
		bufferInfo->mLength = UDP_DATA_HEADER_SIZE + bufferLength;
		bufferInfo->mAddress = peerInfo->mAddress;
		bufferInfo->mSeqNum = seqNumber;
		bufferInfo->mMsgId = messageId;
		bufferInfo->mRetransmit = true;
		bufferInfo->mSentTimes = 0;
		bufferInfo->mFlowId = peerInfo->mFlowId;
		offset += bufferLength;
		mOutgoingBuffers.push_back(bufferInfo);
	}
	delete inTransportRequestEvent;
	
	return 0;
}

int UdpTransportStage::handleConnectRequest(TransportConnectRequest * inTransportRequestEvent)
{
	if (mState == UdpTransportStage::DISCONNECTED) 
	{
		if ((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("socket");
			exit(1);
		}
		p2p::makeSocketNonBlocking(mSocket);

		//Create request
		mPeerAddress = inTransportRequestEvent->mAddress;
		NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
		locNetworkRequestEvent->mEventId = inTransportRequestEvent->mEventId;
		locNetworkRequestEvent->mEventType = NetworkRequestEvent::CONNECT;
		locNetworkRequestEvent->mSocket = mSocket;
		auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
		mAsyncStage->handleEvent(requestPtr);

		//Insert "connected" peer in my UDP peer info list
		UdpPeerInfo * udpPeer = new UdpPeerInfo();
		udpPeer->mAddress = mPeerAddress;
		udpPeer->mFlowId = mFlowId;
		insertUdpPeer(udpPeer);			
		delete inTransportRequestEvent;
		return 0;
	}
	return 0;
}

int UdpTransportStage::handleDisconnectRequest(TransportDisconnectRequest * inTransportRequestEvent)
{
}

int UdpTransportStage::handleListenRequest(TransportListenRequest * inTransportRequestEvent)
{
	mState = CONNECTED;
	if((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) 
	{
				perror("socket");
			    exit(1);
	}
		
	p2p::makeSocketNonBlocking(mSocket);		
	int flag = 1;
	if(setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	
	if (bind(mSocket, (struct sockaddr *)&inTransportRequestEvent->mAddress, sizeof(struct sockaddr)) < 0)
	{
		cout << "UDP: bind error" <<endl;
	}
	
	//Create read request
	NetworkRequestEvent * locNetworkRequestEvent = new NetworkRequestEvent(this);
	locNetworkRequestEvent->mEventType = NetworkRequestEvent::READ;
	locNetworkRequestEvent->mSocket = mSocket;
	auto_ptr<EventInfo> requestPtr(locNetworkRequestEvent);
	mAsyncStage->handleEvent(requestPtr);
	return 0;
}

}
