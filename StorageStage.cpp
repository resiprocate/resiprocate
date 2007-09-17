#include "StorageStage.h"
#include "ChordDhtStage.h"

namespace p2p
{
using namespace std;

const int STORE_EXPIRATION_TIME = 5;

StorageStage::StorageStage()
{
}

StorageStage::~StorageStage()
{
}

DataObject * StorageStage::handleStore(StoreBlock * inCmdBlock)
{
	if (mDataList.empty()){
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STORE_EXPIRATION_TIME;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STORE_EXPIRATION;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
	}
	
	DataObject * data = new DataObject();
	data->mData = new char[inCmdBlock->mDataLength];
	memcpy(data->mData, inCmdBlock->mData, inCmdBlock->mDataLength);
	data->mDataLength = inCmdBlock->mDataLength;
	data->mExpirationTime = inCmdBlock->mTime;
	data->mLocus = inCmdBlock->mLocus;
	data->mDataType = inCmdBlock->mDataType;
	gettimeofday(&data->mRequestedTime, NULL);
	insertData(data);
	
	cout << "STORAGE: stored object " << data->mLocus.toString() << endl;
	
	printDataObject(data);
	return data;
}


int StorageStage::handleRetrieved(RetrievedBlock * inCmdBlock)
{
	if (mDataList.empty()){
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STORE_EXPIRATION_TIME;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STORE_EXPIRATION;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
	}
	
	DataObject * data = new DataObject();
	data->mData = new char[inCmdBlock->mDataLength];
	memcpy(data->mData, inCmdBlock->mData, inCmdBlock->mDataLength);
	data->mDataLength = inCmdBlock->mDataLength;
	data->mExpirationTime = inCmdBlock->mTime;
	data->mLocus = inCmdBlock->mLocus;
	data->mDataType = inCmdBlock->mDataType;
	gettimeofday(&data->mRequestedTime, NULL);
	insertData(data);
	
	cout << "STORAGE: stored retreived object " << data->mLocus.toString() << endl;
	
	printDataObject(data);
	return 0;
}

int StorageStage::handleTimer(TimerCallbackEvent * inCallback)
{
	timeval currentTime;
	gettimeofday(&currentTime, NULL);
	unsigned long elapsedTime;
	unsigned int expirationTime;
	DataObject * data;
	std::vector<DataObject * >::iterator iter;
	for (iter = mDataList.begin(); iter != mDataList.end();){
		expirationTime = (*iter)->mExpirationTime * 1000000;
		elapsedTime = computeTimeElapsed(currentTime, (*iter)->mRequestedTime);
		if (elapsedTime >= expirationTime){
			data = (*iter);
			cout << "STORAGE: purging the following data" << endl;
			printDataObject(data);
			iter = mDataList.erase(iter);
			delete []data->mData;
			delete data;
		}
		else{
			iter++;
		}
	}
	if (!mDataList.empty()){
		//Lastly, renew timer to send expire
		TimerRequestEvent * timer = new TimerRequestEvent(this);
		timer->mExpireTime.tv_sec = STORE_EXPIRATION_TIME;
		timer->mExpireTime.tv_usec = 0;
		timer->mTimerType = TimerRequestEvent::STORE_EXPIRATION;
		auto_ptr<EventInfo> timerPtr(timer);
		mAsyncStage->handleEvent(timerPtr);
	}
	return 0;
}

void StorageStage::printDataObject(DataObject * data)
{
	cout << "STORAGE: locus=[" << data->mLocus.toString() << "],type=[" << data->mDataType;
	cout << "],length=[" << data->mDataLength << "],time=[" << data->mExpirationTime << "]";
	
	switch (data->mDataType){
	case (StoreBlock::STRING):{
		char buf[1024];
		memset(buf, 0, 1024);
		memcpy(buf, data->mData, data->mDataLength);
		string str(buf);
		cout << ",string=[" << str << "]" << endl;
	}
	default:
		cout << endl;
		break;
	}
}
                                                   
void StorageStage::printStorage()
{
	for (unsigned int i = 0; i < mDataList.size(); i++)
	{
		printDataObject(mDataList[i]);
	}
}

void StorageStage::insertData(DataObject * inData)
{
	mDataList.push_back(inData);
}

std::vector<DataObject *> StorageStage::lookupByRange(Locus inLowLocus, Locus inHighLocus)
{
	std::vector<DataObject *> dataObjects;
	for (unsigned int i = 0; i < mDataList.size(); i++)
	{
		if (wrapBetween(inLowLocus, inHighLocus, mDataList[i]->mLocus))
		{
			dataObjects.push_back(mDataList[i]);
		}
	}
	return dataObjects;
	
}

std::vector<DataObject *> StorageStage::lookupByLocus(Locus inLocus)
{
	std::vector<DataObject *> dataObjects;
	for (unsigned int i = 0; i < mDataList.size(); i++)
	{
		if (mDataList[i]->mLocus == inLocus)
		{
			dataObjects.push_back(mDataList[i]);
		}
	}
	return dataObjects;
}

std::vector<DataObject *> StorageStage::lookup(Locus inLocus, DataType inType)
{
	std::vector<DataObject *> dataObjects;
	for (unsigned int i = 0; i < mDataList.size(); i++)
	{
		if (mDataList[i]->mLocus == inLocus && mDataList[i]->mDataType == inType)
		{
			dataObjects.push_back(mDataList[i]);
		}
	}
	return dataObjects;
}


}
