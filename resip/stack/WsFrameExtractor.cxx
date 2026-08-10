
#include "rutil/Logger.hxx"
#include "resip/stack/WsFrameExtractor.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


const int WsFrameExtractor::mMaxHeaderLen = 14;
const size_t WsFrameExtractor::mMaxFrames = 1024;

WsFrameExtractor::WsFrameExtractor(Data::size_type maxMessage)
   : mMaxMessage(maxMessage),
     mMessageSize(0),
     mHaveHeader(false),
     mHeaderLen(0),
     mFinalFrame(false),
     mMasked(false),
     mPayloadLength(0),
     mPayload(0),
     mPayloadPos(0)
{
   memset(mWsMaskKey, 0, sizeof(mWsMaskKey));

   // we re-use this for multiple messages throughout
   // the lifetime of this parser object
   mWsHeader = new uint8_t[mMaxHeaderLen];
}

WsFrameExtractor::~WsFrameExtractor()
{
   // FIXME - delete any objects left in the queues
   delete [] mWsHeader;

   // the buffer of a frame that was only partially received
   delete [] (char*)mPayload;

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

std::unique_ptr<Data>
WsFrameExtractor::processBytes(uint8_t *input, Data::size_type len, bool& dropConnection)
{
   std::unique_ptr<Data> ret;
   dropConnection = false;
   Data::size_type pos = 0;
   while(input != 0 && pos < len)
   {
      while(!mHaveHeader)
      {
         StackLog(<<"Need a header, parsing bytes...");
         // Append bytes to the header buffer.  parseHeader() only returns
         // zero once it has a complete header, so keep re-parsing after
         // every batch of bytes rather than waiting for more input to
         // arrive: an oversized frame has to be rejected as soon as its
         // header is known, not when its payload starts turning up.
         int needed = parseHeader();
         if(needed == 0)
         {
            break;
         }
         // only a header that is still incomplete once mMaxHeaderLen bytes
         // have been consumed is too long: a full header (64 bit length and
         // a mask key) occupies exactly mMaxHeaderLen bytes
         if((mHeaderLen + needed) > mMaxHeaderLen)
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
         // Process input bytes to output buffer, unmasking if necessary.
         // mPayloadLength is an attacker controlled 64 bit value, so the
         // check has to be written to avoid wrapping: comparing
         // mMessageSize + mPayloadLength against mMaxMessage would let a
         // peer bypass it with a length close to 2^64.
         if(mPayloadLength > mMaxMessage ||
            mMessageSize > mMaxMessage - mPayloadLength)
         {
            WarningLog(<<"WS frame header describes a payload size bigger than messageSizeMax, max = " << mMaxMessage
                 << ", dropping connection");
            dropConnection = true;
            return ret;
         }

         // safe to narrow now that it has been bounded by mMaxMessage
         const Data::size_type payloadLength = (Data::size_type)mPayloadLength;

         if(mPayload == 0)
         {
            StackLog(<<"starting new frame buffer");
            // Include an extra byte at the end for null terminator
            mPayload = (uint8_t*)new char[payloadLength + 1];
            mPayloadPos = 0;
         }

         Data::size_type takeBytes = len - pos;
         if(takeBytes > payloadLength - mPayloadPos)
         {
            takeBytes = payloadLength - mPayloadPos;
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

         if(mPayloadPos == payloadLength)
         {
            StackLog(<<"Got a whole frame, queueing it");
            mMessageSize += payloadLength;
            Data *mFrame = new Data(Data::Borrow, (char *)mPayload, payloadLength, payloadLength + 1);
            mFrames.push(mFrame);
            mHaveHeader = false;
            mHeaderLen = 0;
            mPayload = 0;
            if(mFinalFrame)
            {
               joinFrames();
            }
            else if(mFrames.size() >= mMaxFrames)
            {
               // empty and tiny continuation frames don't advance
               // mMessageSize fast enough (or at all) for the size check
               // above to ever terminate this, so bound the frame count too
               WarningLog(<<"WS message fragmented into more than " << mMaxFrames
                    << " frames, dropping connection");
               dropConnection = true;
               return ret;
            }
         }
      }
   }
   if(mMessages.empty())
   {
      StackLog(<<"no full messages available in queue"); 
      return ret;
   }
   ret = std::unique_ptr<Data>(mMessages.front());
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

   uint64_t hdrPos = 2;

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
      // the 64 bit length occupies mWsHeader[2] through mWsHeader[9], so
      // 10 bytes must have arrived before any of it can be read
      if(mHeaderLen < 10)
      {
         StackLog(<< "Too short to contain ws data [2]");
         return (10 - mHeaderLen) + (mMasked ? 4 : 0);
      }
      mPayloadLength = (((uint64_t)mWsHeader[hdrPos]) << 56 | ((uint64_t)mWsHeader[hdrPos + 1]) << 48 | ((uint64_t)mWsHeader[hdrPos + 2]) << 40 | ((uint64_t)mWsHeader[hdrPos + 3]) << 32 | ((uint64_t)mWsHeader[hdrPos + 4]) << 24 | ((uint64_t)mWsHeader[hdrPos + 5]) << 16 | ((uint64_t)mWsHeader[hdrPos + 6]) << 8 | ((uint64_t)mWsHeader[hdrPos + 7]));
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

      // allow extra byte for null terminator
      char *newBuf = new char [mMessageSize + 1];
      memcpy(newBuf, _msg, frameSize);
      delete msg;
      // the Data only borrowed this buffer, so it must be freed separately
      delete [] _msg;

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
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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

