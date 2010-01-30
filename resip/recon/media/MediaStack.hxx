#ifndef recon_MediaStack_hxx
#define recon_MediaStack_hxx

#include <boost/shared_ptr.hpp>
#include <rutil/Data.hxx>

namespace flowmanager
{
class Flow;
}

namespace recon
{
class Mixer;
class RtpStream;

/**
  Main interface to the application's media stack.
*/
class MediaStack
{
public:
   enum MediaType
   {
      MediaType_Audio,
      MediaType_Video,
      MediaType_None
   };

   /**
     Creates a new RTP stream for sending/receiving media.
     The application should perform the initialization required to render
     any media received immediately after this is called.

     @param mediaType The media type of the stream
     @param rtpFlow The RTP Flow which represents the socket endpoint used to
                    send/receive the media stream
     @param rtcpFlow The RTCP Flow corresponding to the RTP stream
     @param dataQOS The QoS for the media
     @param signallingQOS The QoS for the media signalling
   */
   virtual boost::shared_ptr<RtpStream> createRtpStream(
      MediaType mediaType,
      flowmanager::Flow* rtpFlow,
      flowmanager::Flow* rtcpFlow,
      const resip::Data& dataQOS = resip::Data::Empty,
      const resip::Data& signalingQOS = resip::Data::Empty) = 0;

   /**
     Used by recon to create the Mixer object for a Conversation.
     This is required to support local mixing for conferences.
   */
   virtual boost::shared_ptr<Mixer> createMixer() = 0;

   virtual int setSpeakerVolume(int volume) = 0;
   virtual int setMicrophoneGain(int gain) = 0;
   virtual int muteMicrophone(bool mute) = 0;
   virtual int setEchoCancellation(bool useEchoCancel) = 0;
   virtual int setAutomaticGainControl(bool useAGC) = 0;
   virtual int setNoiseReduction(bool useNoiseReduction) = 0;

};
}

#endif // recon_MediaStack_hxx