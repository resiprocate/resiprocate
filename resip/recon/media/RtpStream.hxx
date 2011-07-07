#ifndef recon_RtpStream_hxx
#define recon_RtpStream_hxx

#include <boost/signals.hpp>
#include "Stream.hxx"

namespace recon
{
class MediaStack;

/**
  Used to control the sending/receiving of RTP.
 */
class RtpStream : public recon::Stream
{
public:
	virtual bool isSendingRtp() = 0;
	virtual bool isReceivingRtp() = 0;

   /**
     Start sending RTP and RTCP to the specified endpoints using
     one of the specified codecs and payload types.
   */
	virtual void startRtpSend(
      const resip::Data& rtpIp, 
      unsigned int rtpPort, 
      const resip::Data& rtcpIp, 
      unsigned int rtcpPort,
      const sdpcontainer::SdpMediaLine::CodecList& localCodecs,
      const sdpcontainer::SdpMediaLine::CodecList& remoteCodecs) = 0;

   /**
     Stop sending RTP and RTCP outright.
   */
	virtual void stopRtpSend() = 0;

   /**
     Send only keep-alive packets.
   */
	virtual void pauseRtpSend( bool bAlsoPauseRTCP = false ) = 0;

   /**
     Resume sending media after pausing.
   */
	virtual void resumeRtpSend() = 0;

   /**
     @return True if the outgoing RTP stream is paused.
   */
   virtual bool isPausing() = 0;

   /**
     Start listening for incoming RTP packets.

     @param localCodecs List of local codecs negotiated with the other RTP endpoint.
   */
	virtual void startRtpReceive(const sdpcontainer::SdpMediaLine::CodecList& localCodecs) = 0;

   /**
     Stop listening for incoming RTP packets.
   */
	virtual void stopRtpReceive() = 0;

   /**
     Plays a DTMF tone on this RTP stream.
   */
	virtual void playTone(int toneId, bool playLocal, bool playRemote, bool sendInBand, bool sendOutOfBand) = 0;
	virtual void stopTone() = 0;

   /**
     Plays the file on this RTP stream, sending to the remote side.
   */
   virtual void playFile( const resip::Data& fileName, bool repeat ) = 0;
   virtual void stopFile() = 0;

   /**
     The media type of this media stream.
   */
   virtual MediaStack::MediaType mediaType() = 0;

   enum ClosedReason
   {
      ClosedReason_UserRequest,
      ClosedReason_TransportError,
      ClosedReason_SSRC_Collision,
      ClosedReason_Inactivity,
      ClosedReason_UnknownError
   };

   /**
     Signalled by the media stack to indicate that the RTP stream has closed.
     Used by recon to destroy the RemoteParticipant associated with this RtpStream.
   */
   virtual boost::signal<void (ClosedReason, asio::error_code)>& onClosed() = 0;

	// TODO: playStream, playMemory, playUrl ...

};
}

#endif // recon_RtpStream_hxx