#ifndef RESIP_WsFrameExtractor_hxx
#define RESIP_WsFrameExtractor_hxx

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
      char *processBytes(UInt8 *input, Data::size_type len, bool& dropConnection);

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

