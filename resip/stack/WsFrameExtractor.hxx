#ifndef RESIP_WsFrameExtractor_hxx
#define RESIP_WsFrameExtractor_hxx

#include <memory>
#include <queue>

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"

namespace resip
{

class WsFrameExtractor
{
   public:

      WsFrameExtractor(Data::size_type maxMessage);
      ~WsFrameExtractor();
      std::auto_ptr<Data> processBytes(UInt8 *input, Data::size_type len, bool& dropConnection);

   private:

      static const int mMaxHeaderLen;

      Data::size_type mMaxMessage;

      std::queue<Data*> mFrames;
      std::queue<Data*> mMessages;
      // for tracking the cumulative size of all full frames
      // not yet assembled into a message:
      Data::size_type mMessageSize;

      bool mHaveHeader;
      int mHeaderLen;
      UInt8 *mWsHeader;

      bool mFinalFrame;
      bool mMasked;
      UInt8 mWsMaskKey[4];
      Data::size_type mPayloadLength;

      UInt8 *mPayload;
      Data::size_type mPayloadPos;

      int parseHeader();
      void joinFrames();

};

}

#endif

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

