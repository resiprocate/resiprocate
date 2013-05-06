
#include "rutil/Logger.hxx"
#include "resip/stack/WsFrameExtractor.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


const int WsFrameExtractor::mMaxHeaderLen = 14;

WsFrameExtractor::WsFrameExtractor(Data::size_type maxMessage)
   : mMaxMessage(maxMessage),
     mMessageSize(0),
     mHaveHeader(false),
     mHeaderLen(0)
{
   // we re-use this for multiple messages throughout
   // the lifetime of this parser object
   mWsHeader = new UInt8[mMaxHeaderLen];
}

WsFrameExtractor::~WsFrameExtractor()
{
   // FIXME - delete any objects left in the queues
   delete [] mWsHeader;

   while(!mFrames.empty()) 
   {
      delete [] mFrames.front()->data();
      delete mFrames.front();
      mFrames.pop();
   }
      
   while(!mMessages.empty())
   {
      delete [] mMessages.front()->data();
      delete mMessages.front();
      mMessages.pop();
   }

}

char *
WsFrameExtractor::processBytes(UInt8 *input, Data::size_type len, bool& dropConnection)
{
   dropConnection = false;
   Data::size_type pos = 0;
   while(input != 0 && pos < len)
   {
      while(!mHaveHeader && pos < len)
      {
         // Append bytes to the header buffer
         int needed = parseHeader();
         if(mHeaderLen == mMaxHeaderLen)
         {
            WarningLog(<<"WS Frame header too long");
            dropConnection = true;
            return 0;
         }
         for( ; needed-- > 0 && pos < len; )
            mWsHeader[mHeaderLen++] = input[pos++];
         if(needed > 0)
            return 0;
      }
      if(mHaveHeader)
      {
         // Process input bytes to output buffer, unmasking if necessary
         if(mMessageSize + mPayloadLength > mMaxMessage)
         {
            WarningLog(<<"WS frame header describes a payload size bigger than messageSizeMax, max = " << mMaxMessage 
                 << ", dropping connection");
            dropConnection = true;
            return 0;
         }

         Data::size_type takeBytes = len - pos;
         if(takeBytes > mPayloadLength)
            takeBytes = mPayloadLength;

         if(mPayload == 0)
         {
            mPayload = (UInt8*)new char[mPayloadLength];
            mPayloadPos = 0;
         }

         if(mMasked)
         {
            Data::size_type endOffset = mPayloadPos + takeBytes;
            for( ; mPayloadPos < endOffset; mPayloadPos++)
            {
               mPayload[mPayloadPos] = (input[pos++] ^ mWsMaskKey[(mPayloadPos & 3)]);
            }
         }
         else
         {
            memmove(&mPayload[mPayloadPos], &input[pos], takeBytes);
            pos += takeBytes;
         }

         if(mPayloadPos == mPayloadLength)
         {
            mMessageSize += mPayloadLength;
            Data *mFrame = new Data(Data::Borrow, (char *)mPayload, mPayloadLength);
            mFrames.push(mFrame);
            mHaveHeader = false;
            mPayload = 0;
            if(mFinalFrame)
            {
               joinFrames();
            }
         }
      }
   }
   if(mMessages.empty())
   {
      return 0;
   }
   Data *msg = mMessages.front();
   mMessages.pop();
   char *_msg = (char *)msg->data();
   delete msg;
   return _msg;
}

int
WsFrameExtractor::parseHeader()
{
   if(mHeaderLen < 2)
   {
      StackLog(<< "Too short to contain ws data [0]");
      return (2 - mHeaderLen);
   }

   UInt64 hdrPos = 2;

   mFinalFrame = (mWsHeader[0] >> 7);
   mMasked = (mWsHeader[1] >> 7);

   if(mWsHeader[0] & 0x40 || mWsHeader[0] & 0x20 || mWsHeader[0] & 0x10)
   {
      WarningLog(<< "Unknown extension: " << ((mWsHeader[0] >> 4) & 0x07));
      // do not exit
   }

   mPayloadLength = mWsHeader[1] & 0x7F;
   if(mPayloadLength == 126)
   {
      if(mHeaderLen < 4)
      {
         StackLog(<< "Too short to contain ws data [1]");
         return (4 - mHeaderLen);
      }
      mPayloadLength = (mWsHeader[hdrPos] << 8 | mWsHeader[hdrPos + 1]);
      hdrPos += 2;
   }
   else if(mPayloadLength == 127)
   {
      if(mHeaderLen < 8)
      {
         StackLog(<< "Too short to contain ws data [2]");
         return (8 - mHeaderLen);
      }
      mPayloadLength = (((UInt64)mWsHeader[hdrPos]) << 56 | ((UInt64)mWsHeader[hdrPos + 1]) << 48 | ((UInt64)mWsHeader[hdrPos + 2]) << 40 | ((UInt64)mWsHeader[hdrPos + 3]) << 32 | ((UInt64)mWsHeader[hdrPos + 4]) << 24 | ((UInt64)mWsHeader[hdrPos + 5]) << 16 | ((UInt64)mWsHeader[hdrPos + 6]) << 8 || ((UInt64)mWsHeader[hdrPos + 7]));
      hdrPos += 8;
   }

   if(mMasked)
   {
      if((mHeaderLen - hdrPos) < 4)
      {
         return ((hdrPos + 4) - mHeaderLen);
      }
      mWsMaskKey[0] = mWsHeader[hdrPos];
      mWsMaskKey[1] = mWsHeader[hdrPos + 1];
      mWsMaskKey[2] = mWsHeader[hdrPos + 2];
      mWsMaskKey[3] = mWsHeader[hdrPos + 3];
      hdrPos += 4;
   }

   mHaveHeader = true;
   mPayload = 0;
   return 0;
}

void
WsFrameExtractor::joinFrames()
{
   if(mFrames.empty())
      return;

   Data *mFrame = mFrames.front();
   Data *msg = new Data(Data::Borrow, mFrame->data(), mFrame->size());
   mFrames.pop();
   delete mFrame;
   msg->reserve(mMessageSize);
   while(!mFrames.empty())
   {
      mFrame = mFrames.front();
      mFrames.pop();
      msg->append(mFrame->data(), mFrame->size());
      delete [] mFrame->data();
      delete mFrame;
   }

   mMessages.push(msg);
   mMessageSize = 0;
}

