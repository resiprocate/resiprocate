#ifndef FORWARDBLOCK_H
#define FORWARDBLOCK_H
#include <iostream>
#include <vector>
#include "Locus.h"
#include "Util.h"
namespace p2p{

class ForwardBlock{
  public:
    ForwardBlock();
    ForwardBlock(unsigned int, char *);
    ~ForwardBlock();
        
    int parseBuffer(char *);
    char * toBuffer(int &);
    std::vector<unsigned int> mDestLabelStack;
    std::vector<unsigned int> mSrcLabelStack;
    char * mCmdBuffer;
    unsigned int mCmdLength;
    unsigned char mVer;
    unsigned char mResv;
    unsigned char mTtl;
    unsigned char mNetworkVer;
    unsigned int mNetworkId;
    unsigned int mLength;
  };

}

#endif
