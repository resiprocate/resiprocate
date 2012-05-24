#if !defined RtpEvent_hxx
#define RtpEvent_hxx

#include <tfm/Event.hxx>
#include <boost/shared_ptr.hpp>

class TestRtp;

typedef enum
{
   Rtp_StreamStarted,
   Rtp_AddressChanged,
   Rtp_CodecChanged,
   Rtp_SsrcChanged,
   Rtp_IncorrectPacket,
} RtpEventType;

static const char* RtpEventTypeText[] =
{
   "RTP Stream Started",
   "RTP Address Changed",
   "RTP Codec Changed",
   "RTP SSRC Changed",
   "RTP Incorrect Packet Received",
};

class RtpEvent : public Event
{
   public:
      typedef RtpEventType Type;

      RtpEvent(TestRtp* tua, Type event);

      virtual resip::Data toString() const
      {
         resip::Data buffer;
         {
            resip::DataStream strm(buffer);
            strm << "RtpEvent - " << RtpEventTypeText[mType];
         }
         return buffer;
      }

      virtual resip::Data briefString() const
      {
         return toString();
      }

      static resip::Data getName() { return "RtpEvent"; }
 
      static resip::Data getTypeName(Type type) { return RtpEventTypeText[type]; }

      Type getType() const { return mType; }

   private:
      Type mType;
};

#endif
