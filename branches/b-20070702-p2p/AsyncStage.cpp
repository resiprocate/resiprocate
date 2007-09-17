#include "AsyncStage.h"
#include "TcpTransportStage.h"

using namespace p2p;
using namespace std;

AsyncStage::AsyncStage()
{
	mFdSet = new FdSet();
}

AsyncStage::~AsyncStage()
{
}

AsyncStage::NetworkHandlerInfo::NetworkHandlerInfo()
{
}

AsyncStage::NetworkHandlerInfo::~NetworkHandlerInfo()
{
}

AsyncStage::TimerHandlerInfo::TimerHandlerInfo(timeval inTime, TimerRequestEvent * inRequest)
{
	mRequestedTime = inTime;
	mEventInfo = inRequest;
}

AsyncStage::TimerHandlerInfo::TimerHandlerInfo()
{
}

AsyncStage::TimerHandlerInfo::~TimerHandlerInfo()
{
}

AsyncStage::FdHandlerInfo::FdHandlerInfo()
{
}

AsyncStage::FdHandlerInfo::~FdHandlerInfo()
{
}
unsigned int computeTimeElapsed(timeval t1, timeval t2){
  unsigned int dt1, dt2;
  dt1 = t1.tv_sec * 1000000 + t1.tv_usec;
  dt2 = t2.tv_sec * 1000000 + t2.tv_usec;
  return dt1 - dt2;
}   

unsigned int computeAbsoluteTime(timeval t1){
	 unsigned int dt1;
	 dt1 = t1.tv_sec * 1000000 + t1.tv_usec;
	 return dt1;
}

AsyncStage::TimerHandlerInfo& AsyncStage::TimerHandlerInfo::operator= (const TimerHandlerInfo &inRhs){
	this->mEventInfo = inRhs.mEventInfo;
	this->mRequestedTime = inRhs.mRequestedTime;
	return *this;
}

bool AsyncStage::TimerHandlerInfo::operator== (const TimerHandlerInfo &inRhs) const {
	unsigned long long dt1, dt2;
	dt1 = computeAbsoluteTime(this->mRequestedTime);
	dt2 = computeAbsoluteTime(inRhs.mRequestedTime);
	dt1 += computeAbsoluteTime(this->mEventInfo->mExpireTime);
	dt2 += computeAbsoluteTime(inRhs.mEventInfo->mExpireTime);
	
	if (dt1 == dt2)
		return true;
	else
		return false;
}

bool AsyncStage::TimerHandlerInfo::operator< (const TimerHandlerInfo &inRhs) const {
	unsigned long long dt1, dt2;
	dt1 = computeAbsoluteTime(this->mRequestedTime);
	dt2 = computeAbsoluteTime(inRhs.mRequestedTime);
	dt1 += computeAbsoluteTime(this->mEventInfo->mExpireTime);
	dt2 += computeAbsoluteTime(inRhs.mEventInfo->mExpireTime);
	
	if (dt1 > dt2){
		return true;
	}
	else
		return false;
}

int AsyncStage::enqueueEvent(auto_ptr<EventInfo> inEventInfo){
	this->handleEvent(inEventInfo);
}

int AsyncStage::handleEvent(auto_ptr<EventInfo> inEventInfo){
	if (typeid(*inEventInfo) == typeid(p2p::NetworkRequestEvent)){
		EventInfo * locEventInfo = inEventInfo.release();
		NetworkRequestEvent * networkRequestEvent = (p2p::NetworkRequestEvent *) locEventInfo;
		handleNetworkRequest(networkRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::TimerRequestEvent)){	
		EventInfo * locEventInfo = inEventInfo.release();
		TimerRequestEvent * timerRequestEvent = (p2p::TimerRequestEvent *) locEventInfo;
		handleTimerRequest(timerRequestEvent);
	}
	else if (typeid(*inEventInfo) == typeid(p2p::FdRequestEvent)){
		EventInfo * locEventInfo = inEventInfo.release();
		FdRequestEvent * fdRequestEvent = (p2p::FdRequestEvent *) locEventInfo;
		handleFdRequest(fdRequestEvent);
	}
}

void 
AsyncStage::handleFdRequest(FdRequestEvent * inRequestEvent)
{
	AsyncStage::FdHandlerInfo *handlerInfo = new AsyncStage::FdHandlerInfo();;
	handlerInfo->mEventInfo = inRequestEvent;
	switch(inRequestEvent->mEventType)
	{
	case FdRequestEvent::READ:
		mFdSet->setRead(inRequestEvent->mFd);	
		fdHandlerInfoMap[inRequestEvent->mFd] = handlerInfo;
		break;
	case FdRequestEvent::WRITE:
		mFdSet->setWrite(inRequestEvent->mFd);
		fdHandlerInfoMap[inRequestEvent->mFd] = handlerInfo;
		break;
	default:
		break;
	}
}


void 
AsyncStage::handleTimerRequest(TimerRequestEvent * inRequestEvent)
{
	AsyncStage::TimerHandlerInfo handlerInfo;
	handlerInfo.mEventInfo = inRequestEvent;	
	gettimeofday(&handlerInfo.mRequestedTime, NULL);
	timerHandlers.push(handlerInfo);
}

void 
AsyncStage::handleNetworkRequest(NetworkRequestEvent * inNetworkRequestEvent)
{
	AsyncStage::NetworkHandlerInfo *networkHandlerInfo = new AsyncStage::NetworkHandlerInfo();
	networkHandlerInfo->mEventInfo = inNetworkRequestEvent;
	switch(inNetworkRequestEvent->mEventType)
    {
    case NetworkRequestEvent::READ:
    case NetworkRequestEvent::LISTEN:
    	mFdSet->setRead(inNetworkRequestEvent->mSocket);	
    	networkHandlerInfoMap[inNetworkRequestEvent->mSocket].readHandlers.push_back(networkHandlerInfo);
      break;
    case NetworkRequestEvent::WRITE:
    case NetworkRequestEvent::CONNECT:
    	mFdSet->setWrite(inNetworkRequestEvent->mSocket);
    	networkHandlerInfoMap[inNetworkRequestEvent->mSocket].writeHandlers.push_back(networkHandlerInfo);
    	break;
    default:
      break;
  }
}

int
AsyncStage::processTimerHandlers()
{
	timeval beforeTime;
	timeval afterTime;
	timeval currentTime;
	int readyCount; 
	
	if (timerHandlers.empty())
	{
		return mFdSet->select();
	}
	
	//1. Compute the amount of time that has elapsed between when timer was requested and currentime
	//2. mExpireTime is indicative of the amount of time that should elapsed between requestedtime 
	//   and when callback actually occurs
	gettimeofday(&currentTime, NULL);
	unsigned long elapsedTime = computeTimeElapsed(currentTime, timerHandlers.top().mRequestedTime);
	unsigned long expirationTime = computeAbsoluteTime(timerHandlers.top().mEventInfo->mExpireTime);
	unsigned long remainingTime = 0;
	
	//Sufficient time has passed without selecting, call nulltime
	if (elapsedTime >= expirationTime) 
	{
		gettimeofday(&beforeTime, NULL);
		readyCount = mFdSet->select();
		gettimeofday(&afterTime, NULL);
	}
	//Insufficient time has elapsed
	else 
	{
		remainingTime = expirationTime - elapsedTime;
		gettimeofday(&beforeTime, NULL);
		readyCount = mFdSet->selectMicroSeconds(remainingTime);
		gettimeofday(&afterTime, NULL);
	}
	//Check to make sure enough time has elapsed before bothering with timers
	if (computeTimeElapsed(afterTime, beforeTime) >= remainingTime) 
	{
		while (true) 
		{
			if (timerHandlers.empty()) 
			{
				break;
			}
			TimerHandlerInfo locTimerHandler = timerHandlers.top();
			expirationTime = computeAbsoluteTime(timerHandlers.top().mEventInfo->mExpireTime);
			elapsedTime = computeTimeElapsed(afterTime, locTimerHandler.mRequestedTime);
			
			//Timer is eligible to be fired, go ahead and do so
			//Pop the timer from the priority queue first
			if (elapsedTime >= expirationTime) {
				timerHandlers.pop();
				TimerCallbackEvent * callbackEvent = new TimerCallbackEvent(this);
				callbackEvent->mEventId = locTimerHandler.mEventInfo->mEventId;
				callbackEvent->mTimerType = locTimerHandler.mEventInfo->mTimerType;
				auto_ptr<EventInfo> callbackPtr(callbackEvent);
				locTimerHandler.mEventInfo->mEventSource->handleEvent(callbackPtr);
				//Delete old event info and update time to account for processing time
				delete locTimerHandler.mEventInfo;
				gettimeofday(&afterTime, NULL);
			} 
			else 
			{
				break;
			}
		}
	}
	return readyCount;
}

int
AsyncStage::processFdHandlers(int inFd)
{
	bool locReadReady = false;
	bool locWriteReady = false;
	if (!(fdHandlerInfoMap.find(inFd) == fdHandlerInfoMap.end())) {
		int locRequestType = fdHandlerInfoMap[inFd]->mEventInfo->mEventType;
		if (mFdSet->readyToRead(inFd)) {
			locReadReady = true;
		}
		if (mFdSet->readyToWrite(inFd)) {
			locWriteReady = true;
		}
		if (!locReadReady && !locWriteReady ||
			(locReadReady && !(locRequestType == FdRequestEvent::READ))||
			(locWriteReady && !(locRequestType == FdRequestEvent::WRITE))) {
			return 0;
		}
		p2p::GenericStage * locRequestSource = fdHandlerInfoMap[inFd]->mEventInfo->mEventSource;
		FdCallbackEvent * callbackEvent = new FdCallbackEvent((GenericStage *) this);
		callbackEvent->mEventId = fdHandlerInfoMap[inFd]->mEventInfo->mEventId;
		callbackEvent->mFd = inFd;
		auto_ptr<EventInfo> callbackPtr(callbackEvent);

		switch (locRequestType) {
		case FdRequestEvent::WRITE:
			callbackEvent->mEventType = FdCallbackEvent::WRITE;
			break;
		case FdRequestEvent::READ:
			callbackEvent->mEventType = FdCallbackEvent::READ;
			break;
		default:
			break;
		}
		int locRequestResult = locRequestSource->handleEvent(callbackPtr);
		if (locRequestResult != FdRequestEvent::INCOMPLETE) {
			delete(fdHandlerInfoMap[inFd]->mEventInfo);
			delete(fdHandlerInfoMap[inFd]);
		}
		return 0;
	}
	return 0;
}

void 
AsyncStage::clearNetworkHandlers(std::vector<GenericStage *> & failedStages,
								std::vector<NetworkHandlerInfo *> &inNetworkHandlers)
{
	std::vector<NetworkHandlerInfo *>::iterator locNetHandlerIter;
	std::vector<GenericStage *>::iterator failedIter;
	NetworkHandlerInfo * locNetHandlerInfo;
	bool stagePresent;
	for (locNetHandlerIter = inNetworkHandlers.begin();
		locNetHandlerIter != inNetworkHandlers.end();)
	{
		locNetHandlerInfo = *locNetHandlerIter;
		stagePresent = false;
		
		//TODO: This is hacky and stupid, but we need to make sure we don't delete stage twice
		for (unsigned int i = 0; i < failedStages.size(); i++){
			if (failedStages[i] == locNetHandlerInfo->mEventInfo->mEventSource){
				stagePresent = true;
				break;
			}
		}
		if (!stagePresent)
			failedStages.push_back(locNetHandlerInfo->mEventInfo->mEventSource);
		locNetHandlerIter = inNetworkHandlers.erase(locNetHandlerIter);
		delete(locNetHandlerInfo->mEventInfo);
		delete(locNetHandlerInfo);
	}	
}


int AsyncStage::fireNetworkHandlers(std::vector<NetworkHandlerInfo *> &inNetworkHandlers){
	NetworkHandlerInfo * locNetHandlerInfo;
	int retNetworkRequestResult = NetworkRequestEvent::COMPLETE;
	int locNetworkRequestResult;
	int locNetworkRequestType;
	GenericStage * locRequestSource;
	vector<NetworkHandlerInfo *>::iterator locNetHandlerIter;
	for (locNetHandlerIter = inNetworkHandlers.begin(); 
		 locNetHandlerIter != inNetworkHandlers.end();)
	{
		locNetHandlerInfo = *locNetHandlerIter;
		locNetworkRequestType = locNetHandlerInfo->mEventInfo->mEventType;		
		locRequestSource = locNetHandlerInfo->mEventInfo->mEventSource;

		NetworkCallbackEvent * callbackEvent = new NetworkCallbackEvent((GenericStage *) this);
		callbackEvent->mEventId = locNetHandlerInfo->mEventInfo->mEventId;
		auto_ptr<EventInfo> callbackPtr(callbackEvent);
			
		switch (locNetworkRequestType) {
		case NetworkRequestEvent::WRITE:
			callbackEvent->mEventType = NetworkCallbackEvent::WRITE;
			break;
		case NetworkRequestEvent::CONNECT:
			callbackEvent->mEventType = NetworkCallbackEvent::CONNECT;
			break;
		case NetworkRequestEvent::LISTEN:
			callbackEvent->mEventType = NetworkCallbackEvent::LISTEN;
			break;
		case NetworkRequestEvent::READ:
			callbackEvent->mEventType = NetworkCallbackEvent::READ;
			break;
		default:
			break;
		}
		locNetworkRequestResult = locRequestSource->handleEvent(callbackPtr);
		if (locNetworkRequestResult == NetworkRequestEvent::INCOMPLETE) {
			retNetworkRequestResult = NetworkRequestEvent::INCOMPLETE;
			locNetHandlerIter++;
		}
		else if (locNetworkRequestResult == NetworkRequestEvent::FAIL){
			retNetworkRequestResult = NetworkRequestEvent::FAIL;
			break;
		}
		else{
			locNetHandlerIter = inNetworkHandlers.erase(locNetHandlerIter);
			delete(locNetHandlerInfo->mEventInfo);
			delete(locNetHandlerInfo);
		}
	}
	return retNetworkRequestResult;
}

int
AsyncStage::processNetworkHandlers(Socket inSocket){
	bool locReadReady = false;
	bool locWriteReady = false;
	
	if (!(networkHandlerInfoMap.find(inSocket) == networkHandlerInfoMap.end())) {
		int locResult = NetworkRequestEvent::INCOMPLETE;
		if (mFdSet->readyToRead(inSocket)) {
			locReadReady = true;
		}
		if (mFdSet->readyToWrite(inSocket)) {
			locWriteReady = true;
		}		
		if (!locReadReady && !locWriteReady)
			return 0;

		std::vector<NetworkHandlerInfo *> cpyReadHandlers;
		std::vector<NetworkHandlerInfo *> cpyWriteHandlers;
		
		if (locReadReady && locResult != NetworkRequestEvent::FAIL) {
			cpyReadHandlers = networkHandlerInfoMap[inSocket].readHandlers;
			networkHandlerInfoMap[inSocket].readHandlers.clear();
			locResult = fireNetworkHandlers(cpyReadHandlers);
			if (locResult == NetworkRequestEvent::INCOMPLETE){
				for (unsigned int i = 0; i < cpyReadHandlers.size(); i++){
					networkHandlerInfoMap[inSocket].readHandlers.push_back(cpyReadHandlers[i]);
				}
			}
			else if (locResult == NetworkRequestEvent::COMPLETE){
				if (networkHandlerInfoMap[inSocket].readHandlers.empty())
					mFdSet->clearRead(inSocket);
			}
		}
		
		if (locWriteReady && locResult != NetworkRequestEvent::FAIL) {
			cpyWriteHandlers = networkHandlerInfoMap[inSocket].writeHandlers;
			networkHandlerInfoMap[inSocket].writeHandlers.clear();
			locResult = fireNetworkHandlers(cpyWriteHandlers);
			if (locResult == NetworkRequestEvent::INCOMPLETE){
				for (unsigned int i = 0; i < cpyWriteHandlers.size(); i++){
					networkHandlerInfoMap[inSocket].writeHandlers.push_back(cpyWriteHandlers[i]);
				}
			}
			else if (locResult == NetworkRequestEvent::COMPLETE){
				if (networkHandlerInfoMap[inSocket].writeHandlers.empty())
					mFdSet->clearWrite(inSocket);
			}
		}
		
		if (locResult == NetworkRequestEvent::FAIL){
			mFdSet->clearWrite(inSocket);
			mFdSet->clearRead(inSocket);
			std::vector<GenericStage *> failedStages;
			clearNetworkHandlers(failedStages, cpyReadHandlers);
			clearNetworkHandlers(failedStages, cpyWriteHandlers);
			clearNetworkHandlers(failedStages, networkHandlerInfoMap[inSocket].readHandlers);
			clearNetworkHandlers(failedStages, networkHandlerInfoMap[inSocket].writeHandlers);

			//We need to delete the transport for this failed connection
			for (unsigned int k  = 0; k < failedStages.size(); k++){
				GenericStage * stage = failedStages[k];
				if (typeid(*stage) == typeid(p2p::TcpTransportStage)){
					delete (p2p::TcpTransportStage *) stage;
				}
			}		
		}
	}
	return 0;
}

void AsyncStage::run() 
{
	int locReadyCount = 0;
	while (true) {
		if (!timerHandlers.empty()){
			locReadyCount = processTimerHandlers();
		}
		else{
			locReadyCount = mFdSet->select();
		}
		int locSize = mFdSet->size;
		for (int i = 0; i < locSize; i++) 
		{
			if (!(fdHandlerInfoMap.find(i) == fdHandlerInfoMap.end()))
			{
				processFdHandlers(i);
			}
			
			else if (!(networkHandlerInfoMap.find(i) == networkHandlerInfoMap.end()))
			{
				processNetworkHandlers(i);
			}
		}
	}
}

