#ifndef ROUTINGSTAGE_H_
#define ROUTINGSTAGE_H_
#include <iostream>
#include <vector>
#include <map>
#include "GenericStage.h"
#include "FdSet.h"
#include "Socket.h"
#define HASH_MAP_NAMESPACE __gnu_cxx
#define HashMap __gnu_cxx::hash_map
#define HashSet __gnu_cxx::hash_set

namespace p2p
{
//TODO: According to Chord paper, we should maintain 160 successors... this seems a tad excessive
const int NUM_SUCCESSORS = 32;
const int NUM_PREDECESSORS = 1;
const int NUM_FINGERS = 32;

const unsigned int PEER_ROLE_SUCCESSOR = 1;
const unsigned int PEER_ROLE_SUCCESSOR_BACKUP = 1 << 2;
const unsigned int PEER_ROLE_FINGER = 1 << 3;
const unsigned int PEER_ROLE_JOINING = 1 << 4;
const unsigned int PEER_ROLE_UNKNOWN = 1 << 5;
const unsigned int PEER_ROLE_BOOTSTRAP = 1 << 6;	
const unsigned int PEER_ROLE_PREDECESSOR = 1 << 7;	

class DataObject;
class PeerInfo
{
public:
	
	PeerInfo(){
		memset(mLocus.mBuffer,0,LOCUS_SIZE);
	};
	
	~PeerInfo(){};
	enum ConnectionStatus{CONNECTING, CONNECTED, DISCONNECTED, ISSUED_CONNECT, WAITING_ACCEPT, FAILED};
	enum PeerRole{JOINING, PREDECESSOR, FINGER, SUCCESSOR, SUCCESSOR_BACKUP, UNKNOWN, BOOTSTRAP};
	unsigned int mFlowId;
	Locus mLocus;
	Address mAddress;
	Address mListeningAddress;
	std::vector<CandidateAddress> mCandidates;
	int mIndex;
	PeerRole mRole;
	ConnectionStatus mConnectionStatus;
	std::vector<DataObject * > mPendingStores;
	
	//TODO: use these functions for setting role, a peer can be both a predecessor 
	//and successor/backup_successor, for example
	unsigned int mPeerRole;
	void setRole(unsigned int i)
	{
		mPeerRole = i;
	}
	
	void addRole(unsigned int i)
	{
		mPeerRole = mPeerRole | i;
	}
	
	unsigned int getRole(unsigned int i)
	{
		return mPeerRole;
	}
	
	bool testRole (unsigned int i)
	{
		if (mPeerRole & i)
			return true;
		return false;
	}
	
	void clearRole(unsigned int i)
	{
		mPeerRole = (mPeerRole & (~i));
	}
	
};

struct FingerEntry{
	enum FingerState {CONNECTING, CONNECTED, DISCONNECTED};
	PeerInfo * mPeer;
	Locus mFingerLocus;
	FingerState mState;
};

struct RingEntry{
	PeerInfo * mPeer;
	timeval mStabilizeTime;
};

class RoutingStage : public p2p::GenericStage
{
public:
	RoutingStage()
	{
		for (int i = 0; i < NUM_SUCCESSORS; i++){
			mSuccessorsList[i].mPeer = NULL;
		}
		for (int i = 0; i < NUM_PREDECESSORS; i++){
			mPredecessorsList[i].mPeer = NULL;
		}
	};
	
	~RoutingStage()
	{
		
	};
	
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> inEvent){
		return 0;
	}
	int handleEvent(std::auto_ptr<p2p::EventInfo> inEvent){
		return 0;
	}
	
	std::vector<PeerInfo * > mPeerList;
	Locus mSelfLocus;
	
	bool wrapBetween (Locus l, Locus h, Locus x){
	  if (l < h) {
		  return ((x > l) && (x < h));
	  }
	  else {
		  return ((x > l) || (x < h));
	  }
	}
	
	PeerInfo * lookupByLocus(Locus inLocus){
		for (unsigned int i = 0; i < mPeerList.size(); i++){
			if (mPeerList[i]->mLocus == inLocus)
				return mPeerList[i];
		}
		return NULL;
	}
	
	PeerInfo * lookupByFlowId(unsigned int inFlowId){
		for (unsigned int i = 0; i < mPeerList.size(); i++){
			if (mPeerList[i]->mFlowId == inFlowId)
				return mPeerList[i];
		}
		return NULL;
	}
	
	void insertByLocus(Locus inLocus, PeerInfo * peer){
		assert(lookupByLocus(inLocus) == NULL);
		mPeerList.push_back(peer);
	}
	
	void insertByFlowId(unsigned int inFlowId, PeerInfo * peer){
		assert(lookupByFlowId(inFlowId) == NULL);
		mPeerList.push_back(peer);
	}
	
	void removeByLocus(Locus inLocus){
		std::vector<PeerInfo *>::iterator iter;
		for (iter = mPeerList.begin(); iter != mPeerList.end(); iter++){
			if ((*iter)->mLocus == inLocus){
				mPeerList.erase(iter);
				break;
			}
		}
	}

	void removeByFlowId(unsigned int inFlowId){
		std::vector<PeerInfo *>::iterator iter;
		for (iter = mPeerList.begin(); iter != mPeerList.end(); iter++){
			if ((*iter)->mFlowId == inFlowId){
				mPeerList.erase(iter);
				break;
			}
		}
	}
	
	void updateByLocus(Locus inLocus, PeerInfo * currPeer){
		PeerInfo * prevPeer;
		for (unsigned int i = 0; i < mPeerList.size(); i++){
			if (mPeerList[i]->mLocus == inLocus)
			{
				//Check to see whether these are the same pointers
				if (currPeer != mPeerList[i]){
					prevPeer = mPeerList[i];
					mPeerList[i] = currPeer;
					delete prevPeer;
				}
				else
				{
					mPeerList[i] = currPeer;
				}
			}
				
		}
	}
	
	void updateByFlowId(unsigned int inFlowId, PeerInfo * currPeer){
		PeerInfo * prevPeer;
		for (unsigned int i = 0; i < mPeerList.size(); i++){
			if (mPeerList[i]->mFlowId == inFlowId)
			{
				//Check to see whether these are the same pointers
				if (currPeer != mPeerList[i]){
					prevPeer = mPeerList[i];
					mPeerList[i] = currPeer;
					delete prevPeer;
				}
				else
				{
					mPeerList[i] = currPeer;
				}
			}
				
		}
	}
	
	
	//These are separate
	int findValidFingerIndex(int beginIndex){
	  if (mFingerTable[beginIndex].mState == FingerEntry::CONNECTED) 
		  return beginIndex;
	  for (int i = (beginIndex + 1) % NUM_FINGERS; i != beginIndex;)
	  {
		  if(mFingerTable[i].mState == FingerEntry::CONNECTED) 
			  return i;
		  i = (i+1) % NUM_FINGERS;
	  }
	  return -1;
	}
	
	unsigned int route(std::vector<unsigned int> srcLabelStack, std::vector<unsigned int> destLabelStack){
		assert(destLabelStack.front() == 1);
		std::vector<PeerInfo *> mSuccessorsTable = getConnectedSuccessors();
		std::vector<PeerInfo *> mPredecessorsTable = getConnectedPredecessors();
		
		Locus destLocus = topLocus(destLabelStack);
		if (wrapBetween(mSelfLocus, mSuccessorsTable.front()->mLocus, destLocus)){
			return mSuccessorsTable.front()->mFlowId;
		}
		if (mSuccessorsTable.size() == 1){
			return mSuccessorsTable.front()->mFlowId;
		}
		
		return mSuccessorsTable.front()->mFlowId;
		
		for (unsigned int i = 0; i < mSuccessorsTable.size() - 1; i++){
			if(wrapBetween(mSuccessorsTable[i]->mLocus, mSuccessorsTable[i+1]->mLocus, destLocus)){
				return mSuccessorsTable[i+1]->mFlowId;
			}
		}
		
		//None of the successors worked...rely on finger table
		int currFingerIndex;
		int nextFingerIndex;
		
		//Special cases:
		//No valid finger exist
		if((currFingerIndex = findValidFingerIndex(0)) == -1) 
		{
			return mSuccessorsTable.front()->mFlowId;
		}
		//Only 1 valid index exists (ie: running find_validFingerEntry twice returns the same node)
		else if((nextFingerIndex = findValidFingerIndex((currFingerIndex+1) % NUM_FINGERS)) == currFingerIndex)
		{
		    //Determine whether it is beneficial to use solitary finger entry or to simply route packet to successor
		    if (wrapBetween(mFingerTable[currFingerIndex].mPeer->mLocus, mSelfLocus, destLocus)){
		    	return mFingerTable[currFingerIndex].mPeer->mFlowId;	
		    }
		    else{
		      return mSuccessorsTable.front()->mFlowId;
		    }
		 }
		
		//currFingerIndex is already set to the first valid index at this point in the code's execution
		//During each iteration, there are at least 2 valid entries. Don't use findValidFingerIndex
		//to move currFingerIndex, since we only want to go through list once.
		for (; currFingerIndex < NUM_FINGERS; currFingerIndex++)
		{
			//Current index we are testing is not valid, so continue iterating
		    if(!(mFingerTable[currFingerIndex].mState == FingerEntry::CONNECTED)) 
		    	continue;
		    
		    //Find the subsequent valid entry after this node.
		    //We know it will be different from the validIndex, since we know 2 valid nodes are in the system
		    nextFingerIndex = findValidFingerIndex(((currFingerIndex+1) % NUM_FINGERS));
		    
		    //Return the finger table entry if the destination id satisfies the wrap between conditions
		    if (wrapBetween(mFingerTable[currFingerIndex].mPeer->mLocus, 
		    				mFingerTable[nextFingerIndex].mPeer->mLocus,
		    				destLocus))
		    {
		    	return mFingerTable[currFingerIndex].mPeer->mFlowId;
		    }
		}
		return mSuccessorsTable.front()->mFlowId;
	}
	
	void initializeFingerTable(Locus inLocus){
		int shift = LOCUS_SIZE*8/NUM_FINGERS;
		BIGNUM * mod = BN_new();
		BIGNUM * sum = BN_new();
		BIGNUM * addr = BN_new();
		BIGNUM * result = BN_new();
		
		BN_CTX * ctx = BN_CTX_new();
		BIGNUM * locus = BN_bin2bn(inLocus.mBuffer, LOCUS_SIZE, NULL);
		BN_one(addr)
	;	BN_one(mod);
		BN_lshift(mod, mod, (LOCUS_SIZE)*8-1);
		BN_lshift1(mod, mod);

		for (int i = 0; i < NUM_FINGERS; i++){
			BN_lshift(addr, addr, i*shift);
			BN_add(sum, locus, addr);
			BN_one(addr);
			BN_mod(result, sum, mod, ctx);
			BN_zero(sum);
			unsigned int bytes = BN_num_bytes(result);
			unsigned int diff = LOCUS_SIZE - bytes;
			BN_bn2bin(result, mFingerTable[i].mFingerLocus.mBuffer+diff);
			mFingerTable[i].mState = FingerEntry::DISCONNECTED;
			mFingerTable[i].mPeer = NULL;
		}
		BN_free(mod);
		BN_free(sum);
		BN_free(addr);
		BN_free(result);
		BN_free(locus);
		BN_CTX_free(ctx);
	}
	
	void setSuccessor(int i, PeerInfo * peer)
	{
		if (i < 0 || i > NUM_SUCCESSORS)
			return;
		peer->mIndex = i;
		if (mSuccessorsList[i].mPeer == peer)
			return;
		else if (mSuccessorsList[i].mPeer == NULL){
			mSuccessorsList[i].mPeer = peer;
		}
		else{
			if (i == 0)
				peer->addRole(PEER_ROLE_SUCCESSOR);
			else
				peer->addRole(PEER_ROLE_SUCCESSOR_BACKUP);
			mSuccessorsList[i].mPeer->clearRole(PEER_ROLE_SUCCESSOR | PEER_ROLE_SUCCESSOR_BACKUP);
			mSuccessorsList[i].mPeer = peer;
		}
	}
	
	void setPredecessor(int i, PeerInfo * peer)
	{
		if (i < 0 || i > NUM_PREDECESSORS)
			return;
		peer->mIndex = i;
		if (mPredecessorsList[i].mPeer == peer)
			return;
		else if (mPredecessorsList[i].mPeer == NULL){
			mPredecessorsList[i].mPeer = peer;
		}
		else{
			if (i == 0)
				peer->addRole(PEER_ROLE_PREDECESSOR);
			mPredecessorsList[i].mPeer->clearRole(PEER_ROLE_PREDECESSOR);
			mPredecessorsList[i].mPeer = peer;
		}
	}
	
	PeerInfo * frontPredecessor(){
		return mPredecessorsList[0].mPeer;
	}
	
	PeerInfo * frontSuccessor(){
		return mSuccessorsList[0].mPeer;
	}
	
	bool emptySuccessors(){
		if (mSuccessorsList[0].mPeer == NULL){
			return true;
		}
		else if (mSuccessorsList[0].mPeer->mConnectionStatus != PeerInfo::CONNECTED)
		{
			return true;
		}
		return false;
	}
	
	bool emptyPredecessors(){
		if (mPredecessorsList[0].mPeer == NULL){
			return true;
		}
		else if (mPredecessorsList[0].mPeer->mConnectionStatus != PeerInfo::CONNECTED)
		{
			return true;
		}
		return false;
	}
	
	void clearSuccessors(){
		for ( int i = 0; i < NUM_SUCCESSORS; i++){
			mSuccessorsList[i].mPeer = NULL;
		}
	}
	
	void clearPredecessors(){
		for ( int i = 0; i < NUM_PREDECESSORS; i++){
			mPredecessorsList[i].mPeer = NULL;
		}
	}

	void removeSuccessor(PeerInfo * peer){
		int index;
		for (index = 0; index < NUM_SUCCESSORS;){
			if (mSuccessorsList[index].mPeer == peer){
				mSuccessorsList[index].mPeer = NULL;
				break;
			}
			else{
				index++;
			}
		}
		
		if (index == NUM_SUCCESSORS){
			std::cout << "ROUING: no peer in our list to remove that matches" << std::endl;
		}
		
		//Shift the remaining successors over by one
		for ( int i = index; i < NUM_SUCCESSORS; i++){
			if (!(i+1 < NUM_SUCCESSORS)){
				//Nothing left to shift
				mSuccessorsList[i].mPeer = NULL;
			}
			else{
				mSuccessorsList[i].mPeer = mSuccessorsList[i+1].mPeer;
			}
		}
	}
	
	
	void removePredecessor(PeerInfo * peer){
		int index;
		for (index = 0; index < NUM_PREDECESSORS;){
			if (mPredecessorsList[index].mPeer == peer){
				mPredecessorsList[index].mPeer = NULL;
				break;
			}
			else{
				index++;
			}
		}
		
		if (index == NUM_PREDECESSORS){
			std::cout << "ROUING: no peer in our list to remove that matches" << std::endl;
		}
		
		//Shift the remaining successors over by one
		for ( int i = index; i < NUM_PREDECESSORS; i++){
			if (!(i+1 < NUM_PREDECESSORS)){
				//Nothing left to shift
				mPredecessorsList[i].mPeer = NULL;
			}
			else{
				mPredecessorsList[i].mPeer = mPredecessorsList[i+1].mPeer;
			}
		}
	}
	
	std::vector<PeerInfo *> getAvailableSuccessors(){
		std::vector<PeerInfo *> peers;
		for ( int i = 0; i < NUM_SUCCESSORS; i++){
			if (mSuccessorsList[i].mPeer == NULL)
				continue;
			else{
				peers.push_back(mSuccessorsList[i].mPeer);
			}
		}
		return peers;
	}
	
	std::vector<PeerInfo *> getConnectedSuccessors(){
		std::vector<PeerInfo *> peers;
		for ( int i = 0; i < NUM_SUCCESSORS; i++){
			if (mSuccessorsList[i].mPeer == NULL)
				return peers;
			if (mSuccessorsList[i].mPeer->mConnectionStatus != PeerInfo::CONNECTED)
				return peers;
			else{
				peers.push_back(mSuccessorsList[i].mPeer);
			}
		}
		return peers;
	}
	
	std::vector<PeerInfo *> getConnectedPredecessors(){
		std::vector<PeerInfo *> peers;
		for ( int i = 0; i < NUM_PREDECESSORS; i++){
			if (mPredecessorsList[i].mPeer == NULL)
				return peers;
			if (mPredecessorsList[i].mPeer->mConnectionStatus != PeerInfo::CONNECTED)
				return peers;
			else{
				peers.push_back(mPredecessorsList[i].mPeer);
			}
		}
		return peers;
	}
	
	std::vector<PeerInfo * > mPendingSuccessors;
	std::vector<PeerInfo * > mPendingPredecessors;
	
	
	//Keep track of successors/precessor's flow ids
	RingEntry mSuccessorsList[NUM_SUCCESSORS];
	RingEntry mPredecessorsList[NUM_PREDECESSORS];
	FingerEntry mFingerTable[NUM_FINGERS];

};

}

#endif /*ROUTINGSTAGE_H_*/
