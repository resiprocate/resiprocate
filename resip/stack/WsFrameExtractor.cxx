
#include "rutil/Logger.hxx"
#include "resip/stack/WsFrameExtractor.hxx"
#include "rutil/WinLeakCheck.hxx"

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

std::auto_ptr<Data>
WsFrameExtractor::processBytes(UInt8 *input, Data::size_type len, bool& dropConnection)
{
   std::auto_ptr<Data> ret(0);
   dropConnection = false;
   Data::size_type pos = 0;
   while(input != 0 && pos < len)
   {
      while(!mHaveHeader && pos < len)
      {
         StackLog(<<"Need a header, parsing bytes...");
         // Append bytes to the header buffer
         int needed = parseHeader();
         if(mHeaderLen >= mMaxHeaderLen)
         {
            WarningLog(<<"WS Frame header too long");
            dropConnection = true;
            return ret;
         }
         for( ; needed > 0 && pos < len; needed-- )
         {
            mWsHeader[mHeaderLen++] = input[pos++];
         }
         if(needed > 0)
         {
            StackLog(<<"Not enough bytes available to form a full header");
            return ret;
         }
      }
      if(mHaveHeader)
      {
         StackLog(<<"have header, parsing payload data...");
         // Process input bytes to output buffer, unmasking if necessary
         if(mMessageSize + mPayloadLength > mMaxMessage)
         {
            WarningLog(<<"WS frame header describes a payload size bigger than messageSizeMax, max = " << mMaxMessage 
                 << ", dropping connection");
            dropConnection = true;
            return ret;
         }

         if(mPayload == 0)
         {
            StackLog(<<"starting new frame buffer");
            // Include an extra byte at the end for null terminator
            mPayload = (UInt8*)new char[mPayloadLength + 1];
            mPayloadPos = 0;
         }

         Data::size_type takeBytes = len - pos;
         if(takeBytes > mPayloadLength - mPayloadPos)
         {
            takeBytes = mPayloadLength - mPayloadPos;
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
            mPayloadPos += takeBytes;
         }

         if(mPayloadPos == mPayloadLength)
         {
            StackLog(<<"Got a whole frame, queueing it");
            mMessageSize += mPayloadLength;
            Data *mFrame = new Data(Data::Borrow, (char *)mPayload, mPayloadLength, mPayloadLength + 1);
            mFrames.push(mFrame);
            mHaveHeader = false;
            mHeaderLen = 0;
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
      StackLog(<<"no full messages available in queue"); 
      return ret;
   }
   ret = std::auto_ptr<Data>(mMessages.front());
   mMessages.pop();
   StackLog(<<"returning a message, size = " << ret->size());
   return ret;
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

   mFinalFrame = (mWsHeader[0] >> 7) != 0;
   mMasked = (mWsHeader[1] >> 7) != 0;

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
         return (4 - mHeaderLen) + (mMasked ? 4 : 0);
      }
      mPayloadLength = (mWsHeader[hdrPos] << 8 | mWsHeader[hdrPos + 1]);
      hdrPos += 2;
   }
   else if(mPayloadLength == 127)
   {
      if(mHeaderLen < 8)
      {
         StackLog(<< "Too short to contain ws data [2]");
         return (8 - mHeaderLen) + (mMasked ? 4 : 0);
      }
      mPayloadLength = (((UInt64)mWsHeader[hdrPos]) << 56 | ((UInt64)mWsHeader[hdrPos + 1]) << 48 | ((UInt64)mWsHeader[hdrPos + 2]) << 40 | ((UInt64)mWsHeader[hdrPos + 3]) << 32 | ((UInt64)mWsHeader[hdrPos + 4]) << 24 | ((UInt64)mWsHeader[hdrPos + 5]) << 16 | ((UInt64)mWsHeader[hdrPos + 6]) << 8 || ((UInt64)mWsHeader[hdrPos + 7]));
      hdrPos += 8;
   }

   if(mMasked)
   {
      if((mHeaderLen - hdrPos) < 4)
      {
         StackLog(<< "Too short to contain ws data [3]");
         return (int)((hdrPos + 4) - mHeaderLen);
      }
      mWsMaskKey[0] = mWsHeader[hdrPos];
      mWsMaskKey[1] = mWsHeader[hdrPos + 1];
      mWsMaskKey[2] = mWsHeader[hdrPos + 2];
      mWsMaskKey[3] = mWsHeader[hdrPos + 3];
      hdrPos += 4;
   }

   StackLog(<< "successfully processed a WebSocket frame header, payload length = " << mPayloadLength
            << ", masked = "<< mMasked << ", final frame = "<< mFinalFrame);

   mHaveHeader = true;
   mPayload = 0;
   return 0;
}

void
WsFrameExtractor::joinFrames()
{
   StackLog(<<"trying to join frames");
   if(mFrames.empty())
   {
      ErrLog(<<"No frames to join!");
      return;
   }

   Data *msg = mFrames.front();
   mFrames.pop();
   if(!mFrames.empty())
   {
      // must expand buffer because there are multiple frames
      // can't use Data::reserve() to increase the buffer, because the
      // ShareEnum will change to Take when expanded
      char *_msg = (char *)msg->data();
      Data::size_type frameSize = msg->size();
      delete msg;

      // allow extra byte for null terminator
      char *newBuf = new char [mMessageSize + 1]; 
      memcpy(newBuf, _msg, frameSize);

      msg = new Data(Data::Borrow, newBuf, frameSize, mMessageSize + 1);
   }
   while(!mFrames.empty())
   {
      Data *mFrame = mFrames.front();
      mFrames.pop();
      msg->append(mFrame->data(), mFrame->size());
      delete [] mFrame->data();
      delete mFrame;
   }

   // It is safe to cast because we used Borrow:
   char *_msg = (char *)msg->data();
   // MsgHeaderScanner expects space for an extra byte at the end:
   _msg[mMessageSize] = 0;

   mMessages.push(msg);

   // Ready to start examinging first frame of next message...
   mMessageSize = 0;
}

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

