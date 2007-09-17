#include "ForwardBlock.h"
#include "NetworkTransportStage.h"

using namespace p2p;
using namespace std;

//Offsets necessary for parsing
const int VER_RESV_OFFSET = 0;
const int VER_RESV_SIZE = 1;
const int SRC_COUNT_OFFSET = VER_RESV_OFFSET + VER_RESV_SIZE;
const int SRC_COUNT_SIZE = 1;
const int DST_COUNT_OFFSET = SRC_COUNT_OFFSET + SRC_COUNT_SIZE;
const int DST_COUNT_SIZE = 1;
const int TTL_OFFSET = DST_COUNT_OFFSET + DST_COUNT_SIZE;
const int TTL_SIZE = 1;
const int NETWORK_ID_OFFSET = TTL_OFFSET + TTL_SIZE;
const int NETWORK_ID_SIZE = 3;
const int NETWORK_VER_OFFSET = NETWORK_ID_OFFSET + NETWORK_ID_SIZE;
const int NETWORK_VER_SIZE = 1;
const int FWD_HEADER_SIZE = NETWORK_VER_OFFSET + NETWORK_VER_SIZE;

ForwardBlock::ForwardBlock(){
	mCmdBuffer = NULL;
	mCmdLength = 0;
}

ForwardBlock::~ForwardBlock(){
	if (mCmdBuffer)
		delete [] mCmdBuffer;
}

ForwardBlock::ForwardBlock(unsigned int inLength, char * inBuffer){
	mLength = inLength;
	parseBuffer(inBuffer);
}

char * ForwardBlock::toBuffer(int & inLength){
	char * buffer;
	unsigned char locVerRsv;
	unsigned char locDestCount = mDestLabelStack.size();
	unsigned char locSrcCount = mSrcLabelStack.size();
	unsigned int labelOffset = FWD_HEADER_SIZE;
	unsigned int totalLength = (locDestCount + locSrcCount) * LABEL_SIZE;
	totalLength += FWD_HEADER_SIZE;
	totalLength += mCmdLength;
	buffer = new char[totalLength];
	inLength = static_cast<int>(totalLength);
	locVerRsv = mVer << 6 | mResv;
	
	memcpy(buffer + VER_RESV_OFFSET, &locVerRsv, sizeof(locVerRsv));
	memcpy(buffer + SRC_COUNT_OFFSET, &locSrcCount, SRC_COUNT_SIZE);
	memcpy(buffer + DST_COUNT_OFFSET, &locDestCount, DST_COUNT_SIZE);
	memcpy(buffer + TTL_OFFSET, &mTtl, TTL_SIZE);
	memcpy(buffer + NETWORK_ID_OFFSET, &mNetworkId, NETWORK_ID_SIZE);
	memcpy(buffer + NETWORK_VER_OFFSET, &mNetworkVer, NETWORK_VER_SIZE);
	
	unsigned int locLabel;
	for (unsigned int i = 0; i < mSrcLabelStack.size(); i++){
		locLabel = htonl(mSrcLabelStack[i]);
		memcpy(buffer + labelOffset, &locLabel, LABEL_SIZE);
		labelOffset += LABEL_SIZE;
	}
	
	for (unsigned int i = 0; i < mDestLabelStack.size(); i++){
		locLabel = htonl(mDestLabelStack[i]);
		memcpy(buffer + labelOffset, &locLabel, LABEL_SIZE);
		labelOffset += LABEL_SIZE;
	}
	memcpy(buffer + labelOffset, mCmdBuffer, mCmdLength);
	return buffer;
}

int ForwardBlock::parseBuffer(char * inBuffer)
{
	unsigned char locVerRsv;
	unsigned char locDestCount;
	unsigned char locSrcCount;
	unsigned int destSize;
	unsigned int srcSize;
	unsigned int labelOffset = FWD_HEADER_SIZE;
	memcpy(&locVerRsv, inBuffer + VER_RESV_OFFSET, sizeof(locVerRsv));
    memcpy(&locSrcCount, inBuffer + SRC_COUNT_OFFSET, sizeof(locSrcCount));
    memcpy(&locDestCount, inBuffer + DST_COUNT_OFFSET, sizeof(locDestCount));
	memcpy(&mTtl, inBuffer + TTL_OFFSET, TTL_SIZE);
	memcpy( &mNetworkId, inBuffer + NETWORK_ID_OFFSET,NETWORK_ID_SIZE);
	memcpy( &mNetworkVer, inBuffer + NETWORK_VER_OFFSET, NETWORK_VER_SIZE);
	srcSize = locSrcCount;
	destSize = locDestCount;
	mVer = locVerRsv >> 6;
	mResv = 0x3F && locVerRsv;
	unsigned int locLabel;
	unsigned short locLabelSize = static_cast<unsigned short>(LABEL_SIZE);
	
	for (unsigned int i = 0; i < srcSize; i++){
		memcpy(&locLabel, inBuffer +  labelOffset, LABEL_SIZE);
		locLabel = ntohl(locLabel);
		mSrcLabelStack.push_back(locLabel);
		labelOffset += LABEL_SIZE;
	}
	for (unsigned int i = 0; i < destSize; i++){
		memcpy(&locLabel, inBuffer +  labelOffset, LABEL_SIZE);
		locLabel = ntohl(locLabel);
		mDestLabelStack.push_back(locLabel);
		labelOffset += LABEL_SIZE;
	}
	mCmdLength = mLength - labelOffset;
	char * cmdBuffer = new char[mCmdLength];
	memcpy(cmdBuffer, inBuffer + labelOffset, mCmdLength);
	mCmdBuffer = cmdBuffer;
	return 0;
}