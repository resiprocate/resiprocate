#ifndef COMMANDBLOCK_H_
#define COMMANDBLOCK_H_

#include <iostream>
#include <string>
#include "Locus.h"
#include "Socket.h"
#include "ForwardBlock.h"
namespace p2p{
typedef unsigned int TransactionId;


struct CommandParameter{
	char mExtBits;
	char mType;
	unsigned short mLength;
	char * mData;
	char * mBuffer;
};

//Command Types
const char JOIN_TYPE = 0;
const char PING_TYPE = 1;
const char FETCH_TYPE = 2;
const char STORE_TYPE = 3;
const char CONNECT_TYPE = 4;
const char NOTIFY_TYPE = 5;
const char STABILIZE_TYPE = 6;
const char RETRIEVED_TYPE = 7;
const char DIRECTORY_TYPE = 8;
const char LIST_TYPE = 9;

//Parameter Types
const char LOCUS_TYPE = 0;
const char USERID_TYPE = 1;
const char DATATYPE_TYPE = 2;
const char DATA_TYPE = 3;
const char TIME_TYPE = 4;
const char CANDIDATE_ADDRESS_TYPE = 5;
const char FINGERPRINT_TYPE = 6;
const char NEXT_PROTOCOL_TYPE = 7;
const char USERNAME_TYPE = 8;
const char PASSWORD_TYPE = 9;
const char ROLE_TYPE = 10;
const char SUCCESSOR_PEER_TYPE = 11;
const char PREDECESSOR_PEER_TYPE = 12;
const char LIST_RESULT_TYPE = 13;
const char CHAR_TYPE = 14;

typedef long long FingerPrint;
typedef Address CandidateAddress;

const int PEER_PARAMETER_SIZE = LOCUS_SIZE + sizeof(CandidateAddress);

class PeerParameter
{
public:
	PeerParameter(){};
	~PeerParameter(){};
	Locus mLocus;
	CandidateAddress mAddress;
};

class CommandBlock{
  public:
	CommandBlock();
	virtual ~CommandBlock()=0;
	
    unsigned char mExtBits;
    unsigned char mType;
    unsigned short mLength;
    TransactionId mTransactionId;
    virtual char * toBuffer(int&)=0;
    std::vector<CommandParameter> * parse(char *);
    int loadHeader(char *);
  private:
	 int parseHeader(char *);	  
	 std::vector<CommandParameter> * parseParameters(char *);
  };
    
  class JoinBlock: public p2p::CommandBlock
  {
  public:
	  JoinBlock()
	  {
		  mType = JOIN_TYPE;
	  }
	  JoinBlock(char *);
	  ~JoinBlock();
	  Locus mPeerId;
	  char * mUserId;
	  CandidateAddress mListeningAddress;
	  char * toBuffer(int&);
  };
  
  
  
  class NotifyBlock: public p2p::CommandBlock
    {
    public:
      NotifyBlock()
  	  {
    	  mType = NOTIFY_TYPE;
  	  }
      NotifyBlock(char *);
  	  ~NotifyBlock();
  	  Locus mLocus;
  	  CandidateAddress mListeningAddress;
  	  PeerParameter mPredecessorNode;
  	  std::vector<PeerParameter> mSuccessorNodes;
  	  char * toBuffer(int&);
    };
    
  const unsigned char PING_REQUEST = 0;  
  const unsigned char PING_RESPONSE = 1;
  const unsigned char PING_REQUEST_DISCOVER = 2;
  const unsigned char PING_RESPONSE_DISCOVER = 3;
  
  class PingBlock: public p2p::CommandBlock
  {
  public:
	  PingBlock()
	  {
		  mType = PING_TYPE;
	  };
	  PingBlock(char *);
	  virtual ~PingBlock(){};
	  Locus mPeerId;
	  unsigned char mSubType;
	  char * toBuffer(int&);
  };
  
  class StabilizeBlock: public p2p::CommandBlock
  {
  public:
	  StabilizeBlock()
	  {
		  mType = STABILIZE_TYPE;
	  };
	  StabilizeBlock(char *);
	  virtual ~StabilizeBlock(){};
	  Locus mPeerId;
	  CandidateAddress mListeningAddress;
	  char * toBuffer(int&);
  };
  typedef unsigned int DataType;

  class FetchBlock: public p2p::CommandBlock
  {
  public:
	  FetchBlock()
	  {
		  mType = FETCH_TYPE;
	  };
	  FetchBlock(char *);
	  virtual ~FetchBlock(){};
	  Locus mLocus;
	  DataType mDataType; 
	  char * toBuffer(int&);

  };
    
  typedef unsigned int Time;
  
  class RetrievedBlock: public p2p::CommandBlock
   {
   public:
 	  enum StorageType{STRING};

 	  RetrievedBlock()
 	  {
 		  mType = RETRIEVED_TYPE;
 	  };
 	  RetrievedBlock(char *);
 	  virtual ~RetrievedBlock(){};
 	  Locus mLocus;
 	  Time mTime;
 	  unsigned int mDataLength;
 	  DataType mDataType;
 	  void *mData;
 	  char * toBuffer(int&);
   };  

  const unsigned char STORE_REPLICA_TYPE = 0;
  const unsigned char STORE_ORIGINAL_TYPE = 1;
  
  class StoreBlock: public p2p::CommandBlock
  {
  public:
	  enum StorageType{STRING};
	  StoreBlock()
	  {
		  mType = STORE_TYPE;
	  };
	  StoreBlock(char *);
	  virtual ~StoreBlock(){};
	  Locus mLocus;
	  Time mTime;
	  unsigned char mSubType;
	  unsigned int mDataLength;
	  DataType mDataType;
	  void *mData;
	  char * toBuffer(int&);
  };  
  

  class ConnectBlock: public p2p::CommandBlock
  {
  public:
	  ConnectBlock()
	  {
		  mType = CONNECT_TYPE;
	  };
	  ConnectBlock(char *);
	  virtual ~ConnectBlock(){};

	  Locus mLocus;
	  unsigned char mResponse;
	  std::vector<CandidateAddress> mCandidates;
	  char * mUserName;
	  char * mPassword;
	  char * mRole;
	  FingerPrint mFingerPrint;
	  unsigned short mNextProtocol;
	  char * toBuffer(int&);
  };
  
  struct ListResult{
	  Locus mLocus;
	  DataType mDataType;
  };

  class DirectoryBlock: public p2p::CommandBlock
  {
  public:
	  DirectoryBlock()
	  {
		  mType = DIRECTORY_TYPE;
	  };
	  DirectoryBlock(char *);
	  virtual ~DirectoryBlock(){};
	  char * toBuffer(int&);
	  Locus mLocus;
	  std::vector<ListResult> mListResults;
  };
  
  class ListBlock: public p2p::CommandBlock
  {
  public:
	  ListBlock()
	  {
		  mType = LIST_TYPE;
	  };
	  ListBlock(char *);
	  virtual ~ListBlock(){};
	  char * toBuffer(int&);
	  Locus mLocus;
  };
  

CommandBlock * parseCommandBlock(char *);
  
}

#endif
