#ifndef NETWORKTRANSPORTSTAGE_H_
#define NETWORKTRANSPORTSTAGE_H_
#include "GenericStage.h"
#include "AsyncStage.h"
#include "ForwardingStage.h"

namespace p2p
{

class AsyncStage;
class ForwardingStage;
class EventInfo;

const int TYPE_OFFSET = 0;
const int TYPE_SIZE = 1;
const int LENGTH_OFFSET = TYPE_SIZE + TYPE_OFFSET;
const int LENGTH_SIZE = 4;
const int TCP_HEADER_SIZE= TYPE_SIZE + LENGTH_SIZE;

const unsigned char TCP_TYPE = 0;
const unsigned char UDP_MESSAGE_LENGTH_TYPE = 1;
const unsigned char UDP_DATA_TYPE = 2;
const unsigned char UDP_ACK_TYPE = 3;

//UDP messages start right after the type, 
const int UDP_MESSAGE_ID_OFFSET = TYPE_SIZE + TYPE_OFFSET;
const int UDP_MESSAGE_ID_SIZE = 4;
const int UDP_SEQUENCE_NUMBER_OFFSET = UDP_MESSAGE_ID_OFFSET + UDP_MESSAGE_ID_SIZE;
const int UDP_SEQUENCE_NUMBER_SIZE = 4;
const int UDP_HEADER_SIZE = UDP_SEQUENCE_NUMBER_OFFSET + UDP_SEQUENCE_NUMBER_SIZE;
const int UDP_ACK_SIZE = UDP_SEQUENCE_NUMBER_OFFSET + UDP_SEQUENCE_NUMBER_SIZE;

const int UDP_MAX_PKT_SIZE = 1024;
const unsigned int UDP_DEFAULT_TIMEOUT_SECONDS = 5;
const unsigned int UDP_DEFAULT_TIMEOUT_MICROSECONDS = UDP_DEFAULT_TIMEOUT_SECONDS * 1000000;
const unsigned int UDP_DEFAULT_MAX_SEND_TIMES = 5;

//This is necessary to know about how large the entire message is supposed to be
const int UDP_MESSAGE_LENGTH_OFFSET = UDP_SEQUENCE_NUMBER_OFFSET + UDP_SEQUENCE_NUMBER_SIZE;
const int UDP_MESSAGE_LENGTH_SIZE = 4;
const int UDP_MESSAGE_LENGTH_HEADER_SIZE = UDP_MESSAGE_LENGTH_OFFSET + UDP_MESSAGE_LENGTH_SIZE;
	
//These are the traditional parameters
const int UDP_DATA_LENGTH_OFFSET = UDP_SEQUENCE_NUMBER_OFFSET + UDP_SEQUENCE_NUMBER_SIZE;
const int UDP_DATA_LENGTH_SIZE = 4;
const int UDP_DATA_HEADER_SIZE = UDP_DATA_LENGTH_OFFSET + UDP_DATA_LENGTH_SIZE;
const int UDP_MAX_DATA_SIZE = UDP_MAX_PKT_SIZE-UDP_DATA_HEADER_SIZE;



typedef unsigned int FlowId;

class NetworkTransportStage : public p2p::GenericStage
{

public:
	NetworkTransportStage(){};
	NetworkTransportStage( p2p::AsyncStage *, p2p::ForwardingStage *);
	virtual ~NetworkTransportStage();
    Socket mSocket;
    p2p::ForwardingStage *mForwardingStage;
    p2p::AsyncStage *mAsyncStage;
    virtual int enqueueEvent(std::auto_ptr<p2p::EventInfo> )=0;
    virtual int handleEvent(std::auto_ptr<p2p::EventInfo> )=0;
    FlowId mFlowId;
    
    static std::set<FlowId> mFlowIdSet;
    
    static FlowId generateFlowId(){
    	FlowId locRand = rand();
    	while(p2p::NetworkTransportStage::mFlowIdSet.find(locRand) != 
    		p2p::NetworkTransportStage::mFlowIdSet.end())
    	{
    		locRand = rand();
    	}
    	return locRand;		
    }	
    
    static void releaseFlowId(FlowId inFlowId){
    	mFlowIdSet.erase(inFlowId);
    }
};

}

#endif /*NETWORKTRANSPORTSTAGE_H_*/
