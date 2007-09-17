#ifndef GENERICSTAGE_H_
#define GENERICSTAGE_H_
#include <memory>
#include "EventInfo.h"

namespace p2p
{
class EventInfo;

typedef unsigned int TransactionId;

class GenericStage
{
public:
	
	GenericStage();
	virtual ~GenericStage();
	virtual int enqueueEvent(std::auto_ptr<p2p::EventInfo> )=0;
	virtual int handleEvent(std::auto_ptr<p2p::EventInfo> )=0;
	TransactionId generateTransactionId(){
		TransactionId locRand = rand();
		while(mTrIdSet.find(locRand) != 
			  mTrIdSet.end())
		{
			locRand = rand();
		}
		return locRand;		
	}
	void releaseTransactionId(TransactionId inTrId){
		mTrIdSet.erase(inTrId);
	}
private:
	std::set<TransactionId> mTrIdSet;
};
}
#endif /*GENERICSTAGE_H_*/
