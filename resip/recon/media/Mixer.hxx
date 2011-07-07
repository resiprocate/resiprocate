#ifndef recon_Mixer_hxx
#define recon_Mixer_hxx

#include <vector>
#include <boost/shared_ptr.hpp>

namespace recon
{
class RtpStream;

/**
  The Mixer object is required to support local mixing of media for conferences.

  Recon adds RtpStreams to the Mixer as it adds Participants to a Conversation;
  thus the media for all participants in the Conversation is mixed.

  !jjg! issue: can't set gain for tones, files, streams, etc. independantly of
        the voice -- is this needed?  can it be spec'd
        elsewhere, e.g. MediaStream::playFile(.., .., gain)?
*/
class Mixer
{
public:
   typedef std::vector< boost::shared_ptr<RtpStream> > RtpStreams;

   /**
     Returns all of the RTP currently added to this Mixer.
   */
   virtual const RtpStreams& rtpStreams() const = 0;

   /**
     Add an RTP stream to this Mixer so that other RTP streams receive its media and
     so that it receives the media from all existing RTP streams.
   */
	virtual void addRtpStream(boost::shared_ptr<RtpStream> ms, unsigned int inputGain) = 0;

   /**
     Removes (incoming/outgoing) RTP stream from this Mixer.
   */
	virtual void removeRtpStream(boost::shared_ptr<RtpStream> ms) = 0;
   
   /**
     Modifies the contribution (gain) of a particular mixed RtpStream.
     May not be applicable to all media types.
   */
	virtual void modifyGain(boost::shared_ptr<RtpStream> ms, unsigned int inputGain, unsigned int outputGain) = 0;

   /**
     The startRecording method creates a file containing the output from the
     mixer.
    
     A video file will only be saved if all of width, height, and
     videoFrameRate are specified. If even one of them is not specified, video
     will not be recorded.
    
     @param filePath the full path and name for the file
     @param width the width of the video frame to record
     @param height the height of the video frame to record
     @param videoFrameRate the number of video frames per second to record
    
     @return true if the recording could be started.
    */
   virtual bool startRecording( const std::wstring& filePath,
      unsigned long width = 0,
      unsigned long height = 0,
      int videoFrameRate = 0 ) = 0;

   /**
     If the mixer recording is active, this method will stop the recording and
     close the file. If the mixer is not recording, the method will do nothing,
     but will return false in this case.
    
     @return true if the recording was stopped, false otherwise
    */
   virtual bool stopRecording() = 0;

   /**
    These method allows the local participant to add devices which
    will be locally mixed. Note that it should be impossible to add
    the same device multiple times.
    */
   virtual void addLocalDevices() = 0;

   /**
    Removes the local devices from the mix.
    */
   virtual void removeLocalDevices() = 0;
};
}

#endif // recon_Mixer_hxx