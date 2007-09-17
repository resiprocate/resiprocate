#include <iostream>
#include "CommandBlock.h"
#include "ForwardBlock.h"
#include "AsyncStage.h"
#include "ChordDhtStage.h"
#include "ForwardingStage.h"
#include "InputStage.h"
#include "Locus.h"
#include "Socket.h"
#include "EventInfo.h"
#include "Util.h"
#include <stack>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cmath>;

using namespace p2p;
using namespace std;

int joinTest(){
	JoinBlock joinBlock;
	char * userId = "helloworld";
	joinBlock.mPeerId = 1;
	joinBlock.mTransactionId = 2;
	joinBlock.mUserId = userId;
	cout << joinBlock.mPeerId.toString() << endl;
	
	int length;
	char * block = joinBlock.toBuffer(length);
	JoinBlock blockAgain(block);
	string test(blockAgain.mUserId);
	cout << length << endl;
	cout << test << endl;
	cout << blockAgain.mPeerId.toString() << endl;
	
	return 0;
	
}

int pingTest(){
	PingBlock cmdBlock;
	cmdBlock.mPeerId = 1;
	cmdBlock.mTransactionId = 2;
	cmdBlock.mPeerId = 69;
	
	int length;
	char * block = cmdBlock.toBuffer(length);
	PingBlock blockAgain(block);
	cout << length << endl;
	cout << blockAgain.mPeerId.toString() << endl;
	cout << blockAgain.mTransactionId << endl;
	
	return 0;
	
}

int fetchTest(){
	FetchBlock cmdBlock;
	cmdBlock.mTransactionId = 2;
	cmdBlock.mLocus = 69;
	cmdBlock.mType = 99;
	
	int length;
	char * block = cmdBlock.toBuffer(length);
	FetchBlock blockAgain(block);
	cout << length << endl;
	cout << blockAgain.mLocus.toString() << endl;
	cout << blockAgain.mType << endl;

}

int storeTest(){
	StoreBlock cmdBlock;
	cmdBlock.mTransactionId = 2;
	cmdBlock.mLocus = 69;
	cmdBlock.mType = 99;
	cmdBlock.mTime = 66;
	string str = "hello world";
	cmdBlock.mDataLength = str.size();
	cmdBlock.mData = (void *) str.c_str();
	
	int length;
	char * block = cmdBlock.toBuffer(length);
	StoreBlock blockAgain(block);
	cout << length << endl;
	cout << blockAgain.mLocus.toString() << endl;
	cout << blockAgain.mType << endl;
	cout << blockAgain.mTime << endl;
	cout << blockAgain.mDataLength << endl;
	char buf[12];
	memset(buf, 0, 12);
	memcpy(buf, blockAgain.mData, blockAgain.mDataLength);
	string strAgain(buf);
	cout << strAgain << endl;
	
}


int connectTest(){
	ConnectBlock cmdBlock;
	cmdBlock.mTransactionId = 2;
	cmdBlock.mLocus = 69;
	char * username = "hello";
	char * password = "world";
	char * role = "active";
	cmdBlock.mResponse = 1;
	cmdBlock.mRole = role;
	cmdBlock.mUserName = username;
	cmdBlock.mPassword = password;
	cmdBlock.mFingerPrint = 696969;
	cout << cmdBlock.mLocus.toString() << endl;
	
	CandidateAddress addr;
	struct sockaddr_in dest_addr; 
    dest_addr.sin_family = AF_INET;          // host byte order
    dest_addr.sin_port = htons(1000);   // short, network byte order
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    memset(dest_addr.sin_zero, '\0', sizeof dest_addr.sin_zero);
	memcpy(&addr, &dest_addr, sizeof(struct sockaddr_in));
	cmdBlock.mCandidates.push_back(addr);
	
	int length;
	char * block = cmdBlock.toBuffer(length);
	ConnectBlock blockAgain(block);
	string strRole(blockAgain.mRole);
	string strUser(blockAgain.mUserName);
	string strPass(blockAgain.mPassword);
	cout << strRole << ":" << strUser << ":" << strPass << endl;
	cout << blockAgain.mFingerPrint << endl;
	memcpy(&dest_addr, &cmdBlock.mCandidates.front(), sizeof(struct sockaddr_in));
	cout << ntohs(dest_addr.sin_port) << endl;
	cout << blockAgain.mLocus.toString() << endl;
	if (blockAgain.mResponse == 1){
		cout << "SET" << endl;
	}
	else{
		cout << "UNSET" << endl;
	}
}

int notifyTest(){
	CandidateAddress addr;
	struct sockaddr_in dest_addr; 
    dest_addr.sin_family = AF_INET;          // host byte order
    dest_addr.sin_port = htons(1000);   // short, network byte order
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    memset(dest_addr.sin_zero, '\0', sizeof dest_addr.sin_zero);
    memcpy(&addr, &dest_addr, sizeof(struct sockaddr_in));
    
	NotifyBlock cmdBlock;
	cmdBlock.mTransactionId = 2;
	cmdBlock.mLocus = 69;
	cout << "MY LOCUS " << cmdBlock.mLocus.toString() << endl;
	cmdBlock.mPredecessorNode.mLocus = 10;
	cout << "Pred LOCUS " << cmdBlock.mPredecessorNode.mLocus.toString() << endl;
	memcpy(&cmdBlock.mPredecessorNode.mAddress, &dest_addr, sizeof(struct sockaddr_in));

    dest_addr.sin_port = htons(2000);   // short, network byte order
	PeerParameter peer;
	peer.mLocus = 11;
	memcpy(&peer.mAddress, &dest_addr, sizeof(struct sockaddr_in));
	cmdBlock.mSuccessorNodes.push_back(peer);
	cout << "SUCC LOCUS " << cmdBlock.mSuccessorNodes[0].mLocus.toString() << endl;
	
	int length;
	char * block = cmdBlock.toBuffer(length);
	NotifyBlock blockAgain(block);
	cout << "MY LOCUS AGAIN " << blockAgain.mLocus.toString() << endl;

	memcpy(&dest_addr, &blockAgain.mPredecessorNode.mAddress, sizeof(struct sockaddr_in));
	cout << ntohs(dest_addr.sin_port) << endl;
	cout << "Pred LOCUS AGAIN " << blockAgain.mPredecessorNode.mLocus.toString() << endl;
	memcpy(&dest_addr, &blockAgain.mSuccessorNodes[0].mAddress, sizeof(struct sockaddr_in));
	cout << ntohs(dest_addr.sin_port) << endl;
	cout << "SUCC LOCUS " << blockAgain.mSuccessorNodes[0].mLocus.toString() << endl;
}


int fwdTest(){
	JoinBlock joinBlock;
	char * userId = "helloworld";
	joinBlock.mPeerId = 69;
	cout << joinBlock.mPeerId.toString() << endl;
	
	joinBlock.mTransactionId = 99;
	joinBlock.mUserId = userId;	
	int length;
	char * cmdBuffer = joinBlock.toBuffer(length);
	ForwardBlock fwdBlock;
	fwdBlock.mCmdBuffer = cmdBuffer;
	fwdBlock.mCmdLength = static_cast<unsigned int> (length);
	fwdBlock.mVer = 1;
	fwdBlock.mResv = 2;
	fwdBlock.mTtl = 3;
    fwdBlock.mNetworkVer = 4;
    fwdBlock.mNetworkId = 5;
    Locus destLocus(6);
    Locus srcLocus(7);
    cout << destLocus.toString() << endl;
    cout << srcLocus.toString() << endl;
    
    pushLocus(fwdBlock.mDestLabelStack, destLocus);
    pushLabel(fwdBlock.mDestLabelStack, 999999);
    pushLocus(fwdBlock.mSrcLabelStack, srcLocus);
    pushLabel(fwdBlock.mSrcLabelStack, 666666);

    
    int size;
    char * fwdBuffer = fwdBlock.toBuffer(size);
    ForwardBlock fwdBlockAgain(static_cast<unsigned int>(size), fwdBuffer);  
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    cout << topLabel(fwdBlockAgain.mDestLabelStack) << endl;
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    cout << popLabel(fwdBlockAgain.mDestLabelStack) << endl;
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    cout << topLocus(fwdBlockAgain.mDestLabelStack).toString()<< endl;
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    cout << popLocus(fwdBlockAgain.mDestLabelStack).toString()<< endl;
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    
    cout << popLabel(fwdBlockAgain.mSrcLabelStack) << endl;
    cout << popLocus(fwdBlockAgain.mSrcLabelStack).toString() << endl;
    cout << fwdBlockAgain.mDestLabelStack.size() << endl;
    cout << fwdBlockAgain.mSrcLabelStack.size() << endl;
    
    cout << fwdBlockAgain.mCmdLength << endl;
    
    JoinBlock joinBlockAgain(fwdBlockAgain.mCmdBuffer);
    string userIdTest(joinBlockAgain.mUserId);
    cout << userIdTest << endl;
    cout << joinBlockAgain.mPeerId.toString() << endl;
}

class TimeoutTest:public p2p::GenericStage{
public:
	TimeoutTest(){};
	virtual ~TimeoutTest(){};
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> ){return 0;};
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
	timeval lastTimeCalled;
	bool called;
};

int TimeoutTest::handleEvent(std::auto_ptr<p2p::EventInfo> inPtr){
	if (typeid(*inPtr) == typeid(p2p::TimerCallbackEvent)){
		timeval t1 = lastTimeCalled;
		timeval t2;
		gettimeofday(&t2,NULL);
		unsigned int dt1, dt2;
		dt1 = t1.tv_sec * 1000000 + t1.tv_usec;
		dt2 = t2.tv_sec * 1000000 + t2.tv_usec;
		cout << "CALLED AFTER " << (dt2 - dt1) << endl;
		
		if (!called){
			cout << "HERE!" << endl;
			TimerRequestEvent * event2 = new TimerRequestEvent(this);
			event2->mExpireTime.tv_sec = 3;
			event2->mExpireTime.tv_usec = 0;
			auto_ptr<EventInfo> ptr2(event2);
			inPtr->mEventSource->handleEvent(ptr2);
			called = true;
		}
	}
}


int timeTest(){
	p2p::AsyncStage asyncStage;
	TimeoutTest timeoutStage;
	
	TimerRequestEvent * event1 = new TimerRequestEvent(&timeoutStage);
	event1->mExpireTime.tv_sec = 1;
	event1->mExpireTime.tv_usec = 0;
	auto_ptr<EventInfo> ptr1(event1);
	asyncStage.handleEvent(ptr1);
	
	TimerRequestEvent * event2 = new TimerRequestEvent(&timeoutStage);
	event2->mExpireTime.tv_sec = 10;
	event2->mExpireTime.tv_usec = 0;
	auto_ptr<EventInfo> ptr2(event2);
	asyncStage.handleEvent(ptr2);
	
	gettimeofday(&timeoutStage.lastTimeCalled,NULL);
	asyncStage.run();
	return 0;	
}

int inputTest(){
	p2p::AsyncStage asyncStage;
	p2p::InputStage inputStage;
	FdRequestEvent * request = new FdRequestEvent(&inputStage);
	request->mEventType = FdRequestEvent::READ;
	request->mFd = STDIN_FILENO;
	auto_ptr<EventInfo> requestPtr(request);
	asyncStage.handleEvent(requestPtr);
	asyncStage.run();
	
}
int listDirectoryTest(){
	DirectoryBlock blk;
	blk.mLocus = 1;
	cout << blk.mLocus.toString() << endl;
	
	ListResult list;
	list.mLocus = 2;
	list.mDataType = 3;
	cout << list.mLocus.toString() << " " << list.mDataType << endl;	
	
	ListResult list1;
	list1.mLocus = 4;
	list1.mDataType = 5;
	cout << list1.mLocus.toString() << " " << list1.mDataType << endl;	

	blk.mListResults.push_back(list);
	blk.mListResults.push_back(list1);
	int size;
	char * buffer = blk.toBuffer(size);
	DirectoryBlock blockAgain(buffer);
	cout << "AGAIN!!!!" << endl;
	cout << blockAgain.mLocus.toString() << endl;
	
	for (unsigned int i = 0; i < blockAgain.mListResults.size() ; i++){
		cout << blockAgain.mListResults[i].mLocus.toString() << " " << blockAgain.mListResults[i].mDataType << endl;	
	}
	return 1;
}


int main(int argc, char ** argv)
{	
	srand ( time(NULL) );
	bool LISTENING = true;
	int PEER_PORT = 17000;
	std::string PEER_ADDRESS = "localhost";
	int SELF_PORT = 16000;
	string SELF_ADDRESS = "localhost";
	Locus PEER_LOCUS(PEER_ADDRESS, PEER_PORT);
	char * LOCUS = NULL;
	for(int i=1; i< argc;) 
	{
		if (!strcmp(argv[i], "-b")){
			LISTENING = false;
			i++;
		}
		else if (!strcmp(argv[i], "-pa")){
			PEER_ADDRESS = argv[i+1];
			i+=2;
		}
		else if (!strcmp(argv[i], "-pp")){
			PEER_PORT = atoi(argv[i+1]);
			i+=2;
		}
		else if (!strcmp(argv[i], "-sa")){
			SELF_ADDRESS = argv[i+1];
			i+=2;
		}
		else if (!strcmp(argv[i], "-sp")){
			SELF_PORT = atoi(argv[i+1]);
			i+=2;
		}
		else if (!strcmp(argv[i], "-l")){
			LOCUS = argv[i+1];
			i+=2;
		}
		else
			i++;
	} 
	Locus locLocus(PEER_ADDRESS, PEER_PORT);
	PEER_LOCUS = locLocus;
	if (LISTENING)
		cout << "LISTENING on " << SELF_ADDRESS << ":" << SELF_PORT << endl;// << " with Locus " << PEER_LOCUS.toString() << endl;
	else{
		cout << "LISTENING on " << SELF_ADDRESS << ":" << SELF_PORT << endl;;//<< " with Locus " << PEER_LOCUS.toString() << endl;
		cout << "BOOSTRAPPING to " << PEER_ADDRESS << ":" << PEER_PORT << endl;//<< " with Locus " << PEER_LOCUS.toString() << endl;
	}
	//Construct address
	struct hostent *h;
	if ((h=gethostbyname(PEER_ADDRESS.c_str())) == NULL) {
		perror("Could not get ip of peer");
	    return -1;
	 }
	
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ((struct in_addr *)h->h_addr)->s_addr;
	address.sin_port = htons(PEER_PORT);
	memset(&(address.sin_zero), 0, 8);

	p2p::AsyncStage asyncStage;
	p2p::ChordDhtStage dhtStage;
	p2p::ForwardingStage fwdStage;
	dhtStage.initialize(&asyncStage, &fwdStage);
	if (LOCUS == NULL)
		dhtStage.configure(SELF_ADDRESS, SELF_PORT);
	else
		dhtStage.configure(LOCUS, SELF_ADDRESS, SELF_PORT);
	fwdStage.initialize(&asyncStage, &dhtStage);
	p2p::InputStage inputStage(&dhtStage);
	
	if (LISTENING)
		dhtStage.listen();
	else
		dhtStage.bootstrap(address);
	
	FdRequestEvent * request = new FdRequestEvent(&inputStage);
	request->mEventType = FdRequestEvent::READ;
	request->mFd = STDIN_FILENO;
	auto_ptr<EventInfo> requestPtr(request);
	asyncStage.handleEvent(requestPtr);
	asyncStage.run();
	
	return 0;
}