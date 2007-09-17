#ifndef STORAGESTAGE_H_
#define STORAGESTAGE_H_
#include "GenericStage.h"
#include "RoutingStage.h"
#include "CommandBlock.h"
#include "EventInfo.h"
#include "AsyncStage.h"
namespace p2p
{

class ChordDhtStage;
class AsyncStage;

struct DataObject{
	Locus mLocus;
	timeval mRequestedTime;
	Time mExpirationTime;
	char *mData;
	DataType mDataType;
	unsigned int mDataLength;
};

const unsigned int NUM_REPLICAS = 3;
class StorageStage : public p2p::GenericStage
{

public:
	StorageStage();
	virtual ~StorageStage();
	
	void initialize(AsyncStage * inAsyncStage, ChordDhtStage * inDhtStage)
	{
		mAsyncStage = inAsyncStage;
		mDhtStage = inDhtStage;
	}
	
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> inEvent){
		return 0;
	}
	int handleEvent(std::auto_ptr<p2p::EventInfo> inEvent){
		TimerCallbackEvent * event = (TimerCallbackEvent *) inEvent.release();
		handleTimer(event);
		return 0;
	}
	
	DataObject * handleStore(StoreBlock *);
	int handleRetrieved(RetrievedBlock *);
	int handleTimer(TimerCallbackEvent *);
	void insertData(DataObject *);
	void printStorage();
	void printDataObject(DataObject *);
	std::vector<DataObject *> lookupByRange(Locus, Locus);
	std::vector<DataObject *> lookupByLocus(Locus);
	std::vector<DataObject *> lookup(Locus inLocus, DataType inType);
	bool wrapBetween (Locus l, Locus h, Locus x){
		if (l < h) {
			return ((x > l) && (x < h));
		 }
		else {
			return ((x > l) || (x < h));
		 }
	}
	unsigned int computeTimeElapsed(timeval t1, timeval t2){
	  unsigned int dt1, dt2;
	  dt1 = t1.tv_sec * 1000000 + t1.tv_usec;
	  dt2 = t2.tv_sec * 1000000 + t2.tv_usec;
	  return dt1 - dt2;
	}   

	AsyncStage * mAsyncStage;
	ChordDhtStage * mDhtStage; 
	std::vector<DataObject *> mDataList;
	
};

}

#endif /*STORAGESTAGE_H_*/
