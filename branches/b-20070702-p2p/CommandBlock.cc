#include "CommandBlock.h"

using namespace p2p;
using namespace std;

//CommandBlock Header
const int EXTENSION_OFFSET = 0;
const int EXTENSION_SIZE = 1;
const int COMMAND_OFFSET = EXTENSION_OFFSET + EXTENSION_SIZE;
const int COMMAND_SIZE = 1;
const int COMMAND_LENGTH_OFFSET = COMMAND_OFFSET + COMMAND_SIZE;
const int COMMAND_LENGTH_SIZE = 2;
const int TRID_OFFSET = COMMAND_LENGTH_OFFSET + COMMAND_LENGTH_SIZE;
const int TRID_SIZE = 4;
const int COMMAND_HEADER_SIZE = EXTENSION_SIZE 
						+ COMMAND_SIZE 
						+ COMMAND_LENGTH_SIZE 
						+ TRID_SIZE;


//ParameterBlock header
const int PARAMETER_OFFSET = EXTENSION_OFFSET + EXTENSION_SIZE;
const int PARAMETER_SIZE = 1;
const int PARAMETER_LENGTH_OFFSET = PARAMETER_OFFSET + PARAMETER_SIZE;
const int PARAMETER_LENGTH_SIZE = 2;
const int PARAMETER_HEADER_OFFSET = EXTENSION_SIZE 
								+ PARAMETER_SIZE
								+ PARAMETER_LENGTH_SIZE;
const int PARAMETER_HEADER_SIZE = PARAMETER_HEADER_OFFSET;



#define ntohll(x) (((long long)(ntohl((int)((x << 32) >> 32))) << 32) | \
					(unsigned int)ntohl(((int)(x >> 32)))) 
#define htonll(x) ntohll(x)
                     
CommandBlock::CommandBlock(){
}

CommandBlock::~CommandBlock(){
}

std::vector<CommandParameter> * CommandBlock::parse(char * inBuffer){
	parseHeader(inBuffer);
	return parseParameters(inBuffer);
}

int CommandBlock::parseHeader(char * inBuffer)
{
	unsigned char locExtension;
	unsigned char locCommandType;
	unsigned short locCommandSize;
	unsigned int locTransactionId;
	
	//Parse the Header
	memcpy(&locExtension, inBuffer + EXTENSION_OFFSET, EXTENSION_SIZE);
	memcpy(&locCommandType, inBuffer + COMMAND_OFFSET, COMMAND_SIZE);
	memcpy(&locCommandSize, inBuffer + COMMAND_LENGTH_OFFSET, COMMAND_LENGTH_SIZE);
	memcpy(&locTransactionId, inBuffer + TRID_OFFSET, TRID_SIZE);
	locCommandSize = ntohs(locCommandSize);
	locTransactionId = ntohl(locTransactionId);
	
	//Set the common command variables
    mExtBits = locExtension;
    mType = locCommandType;
    mLength = locCommandSize;
	mTransactionId = locTransactionId;
	return locCommandSize;
}

int CommandBlock::loadHeader(char * buffer){
	unsigned short locCommandLength = htons(mLength);
	unsigned int locTrId = htonl(mTransactionId);
	memcpy(buffer + EXTENSION_OFFSET, &mExtBits, EXTENSION_SIZE);
	memcpy(buffer + COMMAND_OFFSET, &mType, COMMAND_SIZE);
	memcpy(buffer + COMMAND_LENGTH_OFFSET, &locCommandLength, COMMAND_LENGTH_SIZE);
	memcpy(buffer + TRID_OFFSET, &locTrId, TRID_SIZE);
	return 0;
}

std::vector<CommandParameter> * CommandBlock::parseParameters(char * inBuffer)
{
	unsigned char locExtension;
	unsigned char locParameterType;
	unsigned short locParameterSize;
	unsigned int locParameterOffset = COMMAND_HEADER_SIZE;
	unsigned int locCommandSize = mLength;
	std::vector<CommandParameter> * parameterContainer = new std::vector<CommandParameter>();
	
	while(locCommandSize > 0){
		CommandParameter parameter;
		memcpy(&locExtension, inBuffer + EXTENSION_OFFSET, EXTENSION_SIZE);
		memcpy(&locParameterType, inBuffer + locParameterOffset + PARAMETER_OFFSET, PARAMETER_SIZE);
		memcpy(&locParameterSize, inBuffer + locParameterOffset + PARAMETER_LENGTH_OFFSET, PARAMETER_LENGTH_SIZE);
		locParameterSize = ntohs(locParameterSize);
		if (locParameterSize == 0){
			break;
		}
		
		//Create a basic parameter object
		parameter.mExtBits = locExtension;
		parameter.mType = locParameterType;
		parameter.mLength = locParameterSize;
		parameter.mData = (inBuffer + locParameterOffset + PARAMETER_HEADER_OFFSET);
		parameterContainer->push_back(parameter);
		
		//Update offset
		locParameterOffset += PARAMETER_HEADER_SIZE + locParameterSize;
		locCommandSize = locCommandSize - (locParameterSize + PARAMETER_HEADER_SIZE);
	}
	return parameterContainer;
}

Locus parseLocus(char * inBuffer, unsigned short int inSize){
	Locus locPeerId ((unsigned char *) inBuffer, inSize);
	return locPeerId;
}

char * parseString(char * inBuffer, unsigned short int inSize){
	char  *str = new char[inSize + 1];
	memset(str, 0, inSize + 1);
	memcpy(str,  inBuffer, inSize);
	return str;
}

DataType parseDataType(char * inBuffer, unsigned short int inSize){
	DataType locDataType;
	memcpy(&locDataType,  inBuffer, inSize);
	locDataType = ntohl(locDataType);
	return locDataType;
}

unsigned char * parseData(char * inBuffer, unsigned short int inSize){
	unsigned char * locData = new unsigned char[inSize];
	memcpy(locData,  inBuffer, inSize);
	return locData;
}

Time parseTime(char * inBuffer, unsigned short int inSize){
	Time locTime;
	memcpy(&locTime,  inBuffer, inSize);
	locTime = ntohl(locTime);
	return locTime;
}

FingerPrint parseFingerPrint(char * inBuffer, unsigned short int inSize){
	FingerPrint locFingerPrint;
	memcpy(&locFingerPrint,  inBuffer, inSize);			
	locFingerPrint = ntohll(locFingerPrint);
	return locFingerPrint;
}

unsigned short parseNextProtocol(char * inBuffer, unsigned short int inSize){
	unsigned short locNextProtocol;
	memcpy(&locNextProtocol,  inBuffer, inSize);			
	locNextProtocol = ntohs(locNextProtocol);
	return locNextProtocol;
}

char parseChar(char * inBuffer, unsigned short int inSize){
	char locChar;
	memcpy(&locChar, inBuffer, inSize);			
	return locChar;
}

CandidateAddress parseCandidateAddress(char * inBuffer, unsigned short int inSize){
	CandidateAddress candidateAddress;
	memcpy(&candidateAddress,  inBuffer, inSize);
	return candidateAddress;
}
const int PEER_NODE_ID_OFFSET = 0;
const int PEER_NODE_ID_SIZE = LOCUS_SIZE;
const int PEER_NODE_ADDR_OFFSET = PEER_NODE_ID_SIZE;
const int PEER_NODE_ADDR_SIZE = sizeof(CandidateAddress);

PeerParameter parsePeer(char * inBuffer, unsigned short int inSize){
	PeerParameter peer;
	Locus locLocus ((unsigned char *) inBuffer, LOCUS_SIZE);
	CandidateAddress locCandidateAddress = parseCandidateAddress(inBuffer + PEER_NODE_ADDR_OFFSET, PEER_NODE_ADDR_SIZE);
	peer.mAddress = locCandidateAddress;
	peer.mLocus = locLocus;
	return peer;
}

ListResult parseListResult(char * inBuffer, unsigned short int inSize){
	ListResult listResult;
	Locus locLocus ((unsigned char *) inBuffer, LOCUS_SIZE);
	DataType locDataType = parseDataType(inBuffer + LOCUS_SIZE, sizeof(DataType));
	listResult.mDataType = locDataType;
	listResult.mLocus = locLocus;
	return listResult;
}

void loadParameter(char * buffer, unsigned char inExtBits, const unsigned char inType, const unsigned short inSize, void * inData){
	unsigned short locSize = htons(inSize);
	memcpy(buffer, &inExtBits, EXTENSION_SIZE);
	memcpy(buffer + PARAMETER_OFFSET , &inType, PARAMETER_SIZE);
	memcpy(buffer + PARAMETER_LENGTH_OFFSET , &locSize, PARAMETER_LENGTH_SIZE);
	memcpy(buffer + PARAMETER_HEADER_SIZE, inData, inSize);
}

int loadFingerPrint(char * buffer, unsigned char inExtBits, const unsigned char inType, FingerPrint inFingerPrint){
	FingerPrint locFingerPrint = htonll(inFingerPrint);
	unsigned short locSize = sizeof(FingerPrint);
	loadParameter(buffer, inExtBits, inType, locSize, &locFingerPrint);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadNextProtocol(char * buffer, unsigned char inExtBits, const unsigned char inType, unsigned short inLocNextProtocol){
	unsigned short locNextProtocol= htons(inLocNextProtocol);
	unsigned short locSize = sizeof(unsigned short);
	loadParameter(buffer, inExtBits, inType, locSize, &locNextProtocol);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadCandidateAddress(char * buffer, unsigned char inExtBits, const unsigned char inType, CandidateAddress candidateAddress){
	unsigned short locSize = sizeof(CandidateAddress);
	loadParameter(buffer, inExtBits, inType, locSize, &candidateAddress);
	return locSize + PARAMETER_HEADER_SIZE;	
}
int loadLocus(char * buffer, unsigned char inExtBits, const unsigned char inType, Locus inLocus){
	unsigned char * locBuffer = inLocus.toBuffer();
	unsigned short locSize = static_cast<unsigned short>(LOCUS_SIZE);
	loadParameter(buffer, inExtBits, inType, locSize, locBuffer);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadChar(char * buffer, unsigned char inExtBits, const unsigned char inType, char inChar){
	unsigned short locSize = sizeof(char);
	loadParameter(buffer, inExtBits, inType, locSize, &inChar);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadString(char * buffer, unsigned char inExtBits, const unsigned char inType, char * inString){
	unsigned short locSize = strlen(inString);
	loadParameter(buffer, inExtBits, inType, locSize, inString);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadDataType(char * buffer, unsigned char inExtBits, const unsigned char inType, DataType inDataType){
	DataType locDataType = htonl(inDataType);
	unsigned short locSize = sizeof(DataType);
	loadParameter(buffer, inExtBits, inType, locSize, &locDataType);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadTime(char * buffer, unsigned char inExtBits, const unsigned char inType, Time inTime){
	Time locTime = htonl(inTime);
	unsigned short locSize = sizeof(Time);
	loadParameter(buffer, inExtBits, inType, locSize, &locTime);
	return locSize + PARAMETER_HEADER_SIZE;	
}

int loadPeerParameter(char * buffer, unsigned char inExtBits, const unsigned char inType, PeerParameter inPeer){
	unsigned char * locLocus = inPeer.mLocus.toBuffer();
	unsigned short locSize = htons(PEER_PARAMETER_SIZE);
	memcpy(buffer, &inExtBits, EXTENSION_SIZE);
	memcpy(buffer + PARAMETER_OFFSET , &inType, PARAMETER_SIZE);
	memcpy(buffer + PARAMETER_LENGTH_OFFSET , &locSize, PARAMETER_LENGTH_SIZE);
	memcpy(buffer + PARAMETER_HEADER_SIZE, locLocus, LOCUS_SIZE);
	memcpy(buffer+ PARAMETER_HEADER_SIZE + LOCUS_SIZE, &(inPeer.mAddress), sizeof(CandidateAddress));
	
	return PEER_PARAMETER_SIZE + PARAMETER_HEADER_SIZE;
}

int loadListResultParameter(char * buffer, unsigned char inExtBits, const unsigned char inType, ListResult inListResult){
	unsigned char * locLocus = inListResult.mLocus.toBuffer();
	DataType locDataType = htonl(inListResult.mDataType);
	unsigned short locSize = htons(LOCUS_SIZE + sizeof(DataType));
	memcpy(buffer, &inExtBits, EXTENSION_SIZE);
	memcpy(buffer + PARAMETER_OFFSET , &inType, PARAMETER_SIZE);
	memcpy(buffer + PARAMETER_LENGTH_OFFSET , &locSize, PARAMETER_LENGTH_SIZE);
	memcpy(buffer + PARAMETER_HEADER_SIZE, locLocus, LOCUS_SIZE);
	memcpy(buffer+ PARAMETER_HEADER_SIZE + LOCUS_SIZE, &(locDataType), sizeof(DataType));
	return LOCUS_SIZE + sizeof(DataType) + PARAMETER_HEADER_SIZE;
}

NotifyBlock::NotifyBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case CANDIDATE_ADDRESS_TYPE:
			mListeningAddress = parseCandidateAddress(parameter.mData, parameter.mLength);
			break;
		case SUCCESSOR_PEER_TYPE:{
			PeerParameter peer = parsePeer(parameter.mData, parameter.mLength);
			mSuccessorNodes.push_back(peer);
			break;
		}
		case PREDECESSOR_PEER_TYPE:{
			PeerParameter peer = parsePeer(parameter.mData, parameter.mLength);
			mPredecessorNode = peer;
			break;
		}
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * NotifyBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(CandidateAddress);
	totalLength += PARAMETER_HEADER_SIZE + PEER_PARAMETER_SIZE; //predecessor
	totalLength += (PARAMETER_HEADER_SIZE + PEER_PARAMETER_SIZE)*mSuccessorNodes.size();
	
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	locLoadOffset = loadCandidateAddress(buffer+paramOffset, 0, CANDIDATE_ADDRESS_TYPE, mListeningAddress);
	paramOffset += locLoadOffset;
	locLoadOffset = loadPeerParameter(buffer+paramOffset, 0, PREDECESSOR_PEER_TYPE, mPredecessorNode);
	paramOffset += locLoadOffset;
	for (unsigned int i = 0; i < mSuccessorNodes.size(); i++){
		locLoadOffset = loadPeerParameter(buffer+paramOffset, 0, SUCCESSOR_PEER_TYPE, mSuccessorNodes[i]);
		paramOffset += locLoadOffset;
	}
	return buffer;
}

NotifyBlock::~NotifyBlock(){
}


JoinBlock::JoinBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mPeerId = parseLocus(parameter.mData, parameter.mLength);
			break;
		case USERID_TYPE:
			mUserId = parseString(parameter.mData, parameter.mLength);
			break;
		case CANDIDATE_ADDRESS_TYPE:
			mListeningAddress = parseCandidateAddress(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

JoinBlock::~JoinBlock(){
}

char * JoinBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + strlen(mUserId);
	totalLength += PARAMETER_HEADER_SIZE + sizeof(CandidateAddress);
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mPeerId);
	paramOffset += locLoadOffset;
	locLoadOffset = loadString(buffer+paramOffset, 0, USERID_TYPE, mUserId);
	paramOffset += locLoadOffset;
	locLoadOffset = loadCandidateAddress(buffer+paramOffset, 0, CANDIDATE_ADDRESS_TYPE, mListeningAddress);
	paramOffset += locLoadOffset;
	
	return buffer;
}

StabilizeBlock::StabilizeBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mPeerId = parseLocus(parameter.mData, parameter.mLength);
			break;
		case CANDIDATE_ADDRESS_TYPE:
			mListeningAddress = parseCandidateAddress(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * StabilizeBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(CandidateAddress);
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mPeerId);
	paramOffset += locLoadOffset;
	locLoadOffset = loadCandidateAddress(buffer+paramOffset, 0, CANDIDATE_ADDRESS_TYPE, mListeningAddress);
	paramOffset += locLoadOffset;
		
	return buffer;
}


DirectoryBlock::DirectoryBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case LIST_RESULT_TYPE:{
			ListResult result = parseListResult(parameter.mData, parameter.mLength);
			mListResults.push_back(result);
			break;
		}
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * DirectoryBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += (PARAMETER_HEADER_SIZE + LOCUS_SIZE + sizeof(DataType))*mListResults.size();
	
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	for (unsigned int i = 0; i < mListResults.size(); i++){
		locLoadOffset = loadListResultParameter(buffer+paramOffset, 0, LIST_RESULT_TYPE, mListResults[i]);
		paramOffset += locLoadOffset;
	}
	return buffer;
}

ListBlock::ListBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * ListBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
		
	return buffer;
}

PingBlock::PingBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mPeerId = parseLocus(parameter.mData, parameter.mLength);
			break;
		case CHAR_TYPE:
			mSubType = parseChar(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * PingBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(char);
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mPeerId);
	paramOffset += locLoadOffset;
	locLoadOffset = loadChar(buffer+paramOffset, 0, CHAR_TYPE, mSubType);
	paramOffset += locLoadOffset;
		
	return buffer;
}

FetchBlock::FetchBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case DATATYPE_TYPE:
			mDataType = parseDataType(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * FetchBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(DataType);
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	locLoadOffset = loadDataType(buffer+paramOffset, 0, DATATYPE_TYPE, mDataType);
	paramOffset += locLoadOffset;
	
	return buffer;
}


StoreBlock::StoreBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case DATATYPE_TYPE:
			mDataType = parseDataType(parameter.mData, parameter.mLength);
			break;
		case DATA_TYPE:
			mData = parseData(parameter.mData, parameter.mLength);
			mDataLength = parameter.mLength;
			break;
		case TIME_TYPE:
			mTime = parseTime(parameter.mData, parameter.mLength);
			break;
		case CHAR_TYPE:
			mSubType = parseChar(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * StoreBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(Time);
	totalLength += PARAMETER_HEADER_SIZE + sizeof(DataType);
	totalLength += PARAMETER_HEADER_SIZE + mDataLength;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(unsigned char);
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	locLoadOffset = loadTime(buffer+paramOffset, 0, TIME_TYPE, mTime);
	paramOffset += locLoadOffset;
	locLoadOffset = loadChar(buffer+paramOffset, 0, CHAR_TYPE, mSubType);
	paramOffset += locLoadOffset;
	locLoadOffset = loadDataType(buffer+paramOffset, 0, DATATYPE_TYPE, mDataType);
	paramOffset += locLoadOffset;
	loadParameter(buffer+paramOffset, 0, DATA_TYPE, mDataLength, mData);
	paramOffset += mDataLength + PARAMETER_HEADER_SIZE;
	return buffer;
}

RetrievedBlock::RetrievedBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case DATATYPE_TYPE:
			mDataType = parseDataType(parameter.mData, parameter.mLength);
			break;
		case DATA_TYPE:
			mData = parseData(parameter.mData, parameter.mLength);
			mDataLength = parameter.mLength;
			break;
		case TIME_TYPE:
			mTime = parseTime(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * RetrievedBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned short locLoadOffset = 0;
	unsigned int paramOffset =0;
	
	//Compute the size of the parameters
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(Time);
	totalLength += PARAMETER_HEADER_SIZE + sizeof(DataType);
	totalLength += PARAMETER_HEADER_SIZE + mDataLength;
	mLength = totalLength;
	
	//Compute total size of bufer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;

	//Load the header and parameters into the buffer
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	locLoadOffset = loadTime(buffer+paramOffset, 0, TIME_TYPE, mTime);
	paramOffset += locLoadOffset;
	locLoadOffset = loadDataType(buffer+paramOffset, 0, DATATYPE_TYPE, mDataType);
	paramOffset += locLoadOffset;
	loadParameter(buffer+paramOffset, 0, DATA_TYPE, mDataLength, mData);
	paramOffset += mDataLength + PARAMETER_HEADER_SIZE;
	return buffer;
}

ConnectBlock::ConnectBlock(char * inBuffer){
	std::vector<CommandParameter> * parameterContainer = parse(inBuffer);
	for (unsigned int i = 0; i < parameterContainer->size(); i++){
		CommandParameter parameter = parameterContainer->at(i);
		switch(parameter.mType){
		case LOCUS_TYPE:
			mLocus = parseLocus(parameter.mData, parameter.mLength);
			break;
		case USERNAME_TYPE:
			mUserName = parseString(parameter.mData, parameter.mLength);
			break;
		case PASSWORD_TYPE:
			mPassword = parseString(parameter.mData, parameter.mLength);
			break;
		case ROLE_TYPE:
			mRole= parseString(parameter.mData, parameter.mLength);
			break;
		case FINGERPRINT_TYPE:
			mFingerPrint = parseFingerPrint(parameter.mData, parameter.mLength);
			break;
		case NEXT_PROTOCOL_TYPE:
			mNextProtocol = parseNextProtocol(parameter.mData, parameter.mLength);
			break;
		case CANDIDATE_ADDRESS_TYPE:
			CandidateAddress candidateAddress = parseCandidateAddress(parameter.mData, parameter.mLength);
			mCandidates.push_back(candidateAddress);
			break;
		case CHAR_TYPE:
			mResponse= parseChar(parameter.mData, parameter.mLength);
			break;
		default:
			break;
		}
	}
	delete parameterContainer;
}

char * ConnectBlock::toBuffer(int & inLength){
	char * buffer;	
	unsigned int totalLength = 0;
	unsigned int paramOffset =0;
	unsigned short locLoadOffset = 0;
	
	totalLength += PARAMETER_HEADER_SIZE + strlen(mUserName);
	totalLength += PARAMETER_HEADER_SIZE + strlen(mPassword);
	totalLength += PARAMETER_HEADER_SIZE + strlen(mRole);
	totalLength += PARAMETER_HEADER_SIZE + LOCUS_SIZE;
	totalLength += PARAMETER_HEADER_SIZE + sizeof(mFingerPrint);
	totalLength += PARAMETER_HEADER_SIZE + sizeof(mNextProtocol);
	totalLength += (PARAMETER_HEADER_SIZE + sizeof(CandidateAddress)) * mCandidates.size();
	totalLength += PARAMETER_HEADER_SIZE + sizeof(char);
	
	mLength = totalLength;
	
	//Create the new buffer
	totalLength += COMMAND_HEADER_SIZE;
	buffer = new char[totalLength];
	inLength = totalLength;
	
	//Set up header and parameters
	loadHeader(buffer);
	paramOffset += COMMAND_HEADER_SIZE;
	locLoadOffset = loadString(buffer+paramOffset, 0, USERNAME_TYPE, mUserName);
	paramOffset += locLoadOffset;
	locLoadOffset = loadString(buffer+paramOffset, 0, PASSWORD_TYPE, mPassword);
	paramOffset += locLoadOffset;
	locLoadOffset = loadString(buffer+paramOffset, 0, ROLE_TYPE, mRole);
	paramOffset += locLoadOffset;
	locLoadOffset = loadFingerPrint(buffer+paramOffset, 0, FINGERPRINT_TYPE, mFingerPrint);
	paramOffset += locLoadOffset;
	locLoadOffset = loadNextProtocol(buffer+paramOffset, 0, NEXT_PROTOCOL_TYPE, mNextProtocol);
	paramOffset += locLoadOffset;
	locLoadOffset = loadLocus(buffer+paramOffset, 0, LOCUS_TYPE, mLocus);
	paramOffset += locLoadOffset;
	locLoadOffset = loadChar(buffer+paramOffset, 0, CHAR_TYPE, mResponse);
	paramOffset += locLoadOffset;
	
	for (unsigned int i = 0; i < mCandidates.size(); i++){
		locLoadOffset  =  loadCandidateAddress(buffer+paramOffset, 0, CANDIDATE_ADDRESS_TYPE, mCandidates[i]);
		paramOffset += locLoadOffset;
	}	
	return buffer;
}


char * CommandBlock::toBuffer( int & inLength){
	return NULL;
}


CommandBlock * p2p::parseCommandBlock(char * inBuffer){
	char locCommandType;
	memcpy(&locCommandType, inBuffer + COMMAND_OFFSET, COMMAND_SIZE);
	switch(locCommandType)
	{
	case JOIN_TYPE:{
		JoinBlock * joinBlock = new JoinBlock(inBuffer);
		return joinBlock;
	}
	case PING_TYPE:{
		PingBlock * pingBlock = new PingBlock(inBuffer);
		return pingBlock;
	}
	case FETCH_TYPE:{
		FetchBlock * fetchBlock = new FetchBlock(inBuffer);
		return fetchBlock;
	}
	case STORE_TYPE:{
		StoreBlock * storeBlock = new StoreBlock(inBuffer);
		return storeBlock;	
	}
	case CONNECT_TYPE:{
		ConnectBlock * connectBlock = new ConnectBlock(inBuffer);
		return connectBlock;	
	}
	case NOTIFY_TYPE:{
		NotifyBlock * notifyBlock = new NotifyBlock(inBuffer);
		return notifyBlock;
	}
	case STABILIZE_TYPE:{
		StabilizeBlock * block = new StabilizeBlock(inBuffer);
		return block;
	}
	case RETRIEVED_TYPE:{
		RetrievedBlock * block = new RetrievedBlock(inBuffer);
		return block;
	}
	case LIST_TYPE:{
		ListBlock * block = new ListBlock(inBuffer);
		return block;
	}
	case DIRECTORY_TYPE:{
		DirectoryBlock * block = new DirectoryBlock(inBuffer);
		return block;
	}
	default:
		break;
	}
	
	return NULL;
}