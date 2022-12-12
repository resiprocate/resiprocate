#if !defined TestRtp_hxx
#define TestRtp_hxx

#include "tfm/EndPoint.hxx"

#include "tfm/RtpEvent.hxx"

#include "rutil/Socket.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/SdpContents.hxx"
#include "rutil/ThreadIf.hxx"

#include <memory>

static const int MaxBufferSize = 8192;
typedef resip::SdpContents::Session::Medium Medium;

typedef enum
{
   MEDIA_NONE = 0,
   MEDIA_ACTIVE = 1<<0,
   MEDIA_HOLD = 1<<1,
   MEDIA_INACTIVE = 1<<2,     // media-direction='inactive'
   MEDIA_HOLD_PEER = 1<<3,

   MEDIA_DISABLE = 1<<4,      // port=0
} MediaType;

struct RtpHeader
{
   uint8_t mVersion;
   uint8_t mPayloadType;
   uint16_t mSequenceNumber;
   uint32_t mTimeStamp;
   uint32_t mSsrc;
};

struct RtcpHeader
{
   uint8_t mVersion;
   uint8_t mPayloadType;
   uint16_t mSequenceNumber;
   uint32_t mSsrc;
};

struct Packet
{
   Packet(timeval timeStamp, resip::Data data) :
      mTimeStamp(timeStamp),
      mData(data)
   {}

   struct timeval mTimeStamp;
   resip::Data mData;
};

struct PacketSenderStatistics
{
   PacketSenderStatistics();
   void clear();

   int mSent;
};

struct PacketReceiverStatistics
{
   PacketReceiverStatistics();
   void clear();

   int mReceived;
   int mLost;
};


class SdpHelper
{
public:
   enum MediaDirection {MD_None, MD_SendOnly, MD_RecvOnly, MD_SendRecv, MD_Inactive};

   static resip::Data AudioMediaType;
   static resip::Data VideoMediaType;
   static resip::Data ControlMediaType;

   static resip::Data RtpAvpProtocol;
   static resip::Data TcpMcProtocol;


   static bool hasIce(std::shared_ptr<resip::SipMessage> msg);
   static bool isMediaInactive(std::shared_ptr<resip::SipMessage> msg);
   static resip::Data getConnectionAddr(std::shared_ptr<resip::SipMessage> msg);
   static bool isHold2543(std::shared_ptr<resip::SipMessage> msg);
   static bool isMediaSendOnly(std::shared_ptr<resip::SipMessage> msg);
   static bool isMediaRecvOnly(std::shared_ptr<resip::SipMessage> msg);
   static unsigned int getMediaCount(std::shared_ptr<resip::SipMessage> msg);
   // m-line
   static unsigned int getMLineCount(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName);
   // codec
   static unsigned int getCodecsCount(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName);
   static bool hasPayloadNumber(std::shared_ptr<resip::SipMessage> msg, int payloadNumber);

   /**
    * Media index starts at zero. Usually 0 means audio, 1 video, etc.
   */
   // port
   static int getPort(std::shared_ptr<resip::SipMessage> msg, unsigned int index = 0);
   static int getPort(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName);
   static void setPort(std::shared_ptr<resip::SdpContents> sdp, const char * szMediaName, unsigned long);
   // attribute a=
   static void addAttr(std::shared_ptr<resip::SdpContents> sdp, const char * szMediaName, const char* szAttrField, const char* szAttrValue);

   static MediaDirection getMediaDirection(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName);
   static MediaDirection getMediaDirectionFromString(const char * szMediaName);
};

///////////////////////////////////////////////////////////////////////////////
class TestRtp : public EndPoint,
                public resip::ThreadIf
{
   public:
      TestRtp();

      virtual ~TestRtp() = default;

      // class implementation
      void open();
      void close();
      void bind();

      /**
       * Set SDP back to orginal state.
       */
      void clean();

      /**
       * Set both local address and port. RTCP port is RTP port incremented by one.
       */
      void setLocalAddr(const resip::Uri& addr);

      void setPtime(int ptime) { mPtime = ptime; }
      void setSessionName(const char* s)  {mSessionName = resip::Data(resip::Data::Share, s);}
      const resip::Data& getSessionName() const {return mSessionName;}

      /**
       * add codec to a particular m-line (determined by media-name: 'audio',
       * 'video'). If multiple m-lines of the same type are present, will add
       * to the LAST m-line of that type.
       */
      void addCodec(
         const resip::Data& mediaName,
         const resip::Data& codecName,
         int payload,
         int rate,
         const resip::Data& params = resip::Data::Empty,
         int port = 0,
         const char* proto = 0);
      void addAudioCodec(const resip::Data& name, int pt, int rate, const resip::Data& params = resip::Data::Empty)
         { addCodec(SdpHelper::AudioMediaType, name, pt, rate, params); }
      void addVideoCodec(const resip::Data& name, int pt, int rate, const resip::Data& params = resip::Data::Empty)
         { addCodec(SdpHelper::VideoMediaType, name, pt, rate, params); }
      void addBasicAudioCodecs();

      void addControlMLine();

      void removeCodecs(const resip::Data& mediaName = resip::Data::Empty);   // removes ALL codecs by default.

      /**
       * Set local address only. Port is unchanged. RTCP port is RTP port incremented by one.
       */
      void setLocalAddr(const resip::Data& addr);

      void loadStream(const resip::Data& file, uint32_t ssrc);

      /**
       * Set to use hold method described by RFC 2543 (settings connections to 0.0.0.0)
       */
      void setHold2543(bool hold2543 = true) { mHold2543 = hold2543; }
      void setDescribeWellKnownCodec(bool val = true) { mDescribeWellKnownCodec = val; }

      std::shared_ptr<resip::SdpContents> getLocalSdp() const;
      std::shared_ptr<resip::SdpContents> getLocalSdp(unsigned long, unsigned long = MEDIA_NONE) const;

      void setRemoteAddr(const resip::Tuple& addr);

      virtual resip::Data getName() const;

      virtual void thread();

      // expects
      ExpectBase* expect(RtpEvent::Type type, int timeoutMs, ActionBase* expectAction);

      // actions
      ActionBase* enableSending(bool enable);

   private:
      bool isRtcpPacket(const resip::Data& packet, uint32_t ssrc) const;

      bool isRtpPacket(const resip::Data& packet, uint32_t ssrc) const;

      void enableSendingDelegate(bool enable);

      resip::Socket openSocket(resip::TransportType type);

      int bindSocket(resip::Socket fd, resip::Tuple& addr);

      resip::Data recvPacket(resip::Socket fd, resip::Tuple& addr);

      bool parsePacket(const resip::Data& data);

      void sendPacket(resip::Socket fd, resip::Tuple& dest, const resip::Data& data);

      /**
       * Set the SSRC of the outgoing data packet. It is assumed that
       * the packet has RTP hader and just overwrite SSRC
       */
      void overrideSsrc(resip::Data& packet);

      uint16_t getSeqNo(const Packet& packet);

      void RtpPacketInfo();

   private:
      std::shared_ptr<resip::SdpContents> mLocalSdp;

      resip::Socket mFdRtp;
      resip::Socket mFdRtcp;
      resip::Socket mFdVideoRtp;
      resip::Socket mFdVideoRtcp;
      resip::Tuple mLocalAddrRtp;
      resip::Tuple mLocalAddrRtcp;
      resip::Tuple mLocalAddrVideoRtp;
      resip::Tuple mLocalAddrVideoRtcp;
      resip::Tuple mRemoteAddrRtp;
      resip::Tuple mRemoteAddrRtcp;
      uint32_t mLocalSsrc;
      uint32_t mRemoteSsrc;
      uint8_t mRemoteCodec;

      PacketSenderStatistics mSenderStatistics;
      PacketReceiverStatistics mReceiverStatistics;

      bool mHold2543;
      bool mDescribeWellKnownCodec;
      int mPtime;
      resip::Data mSessionName;

      std::list<Packet> mPacketsRtp;
      std::list<Packet> mPacketsRtcp;

      bool mSendPackets;
};

#endif
