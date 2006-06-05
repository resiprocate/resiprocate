#include "resip/stack/StunMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

StunMessage::StunMessage(const Transport* fromWire)
   : TransactionMessage(fromWire),
   mParsed(false),
   mTotalBufferSize(0)
{
}

StunMessage::StunMessage(const StunMessage& from)
{
   *this = from;
}

Message*
StunMessage::clone() const
{
   return new StunMessage(*this);
}

StunMessage& 
StunMessage::operator=(const StunMessage& rhs)
{
   if (this != &rhs)
   {
      this->cleanUp();
      TransactionMessage::operator=(rhs);

      mStunMessageStruct = rhs.mStunMessageStruct;
      if(mStunMessageStruct.hasTurnData)  // .slg. should probably encapsulate this in StunMessageStruct as operator=
      {
         // Copy turn data
         mStunMessageStruct.turnData = new Data(*rhs.mStunMessageStruct.turnData);
      }
   }

   return *this;
}

StunMessage::~StunMessage()
{
   cleanUp();
}

void StunMessage::addBuffer(char* buf, int len)
{
   mBufferList.push_back(pair<char*,int>(buf,len));
   mTotalBufferSize += len;
}

void
StunMessage::cleanUp()
{   
   for (vector<pair<char*,int> >::iterator i = mBufferList.begin();
        i != mBufferList.end(); i++)
   {
      delete [] i->first;
   }
   mBufferList.clear();
}

const Data& 
StunMessage::getTransactionId() const
{
   return mTransactionId;
}

bool StunMessage::parse()
{
   bool success = false;

   if(mParsed) return true;

   if(mBufferList.size() > 0)
   {
      if(mBufferList.size() == 1)
      {
         success = stunParseMessage(mBufferList.front().first, mBufferList.front().second, mStunMessageStruct, false);
      }
      else
      {
         // normally we will get entire stun message in one buffer, if fragmented we do this somewhat inefficient copying
         char *continuousBuffer = new char[mTotalBufferSize];
         if(continuousBuffer)
         {
            char *pos = continuousBuffer;

            // Copy Data
            for (vector<pair<char*,int> >::iterator i = mBufferList.begin();
                 i != mBufferList.end(); i++)
            {
               memcpy(pos, i->first, i->second);
               pos += i->second;
            }
            success = stunParseMessage(continuousBuffer, mTotalBufferSize, mStunMessageStruct, false);
            cleanUp();
            addBuffer(continuousBuffer, mTotalBufferSize);  
         }
      }
   }
   if(success)
   {
      mTransactionId = Data((const char*)&mStunMessageStruct.msgHdr.id, sizeof(mStunMessageStruct.msgHdr.id));
      if(mStunMessageStruct.msgHdr.msgType & 0x0100)
      {
         mResponse = true;
      }
      else
      {
         mRequest = true;
      }
      mParsed=true;
   }

   return success;
}
