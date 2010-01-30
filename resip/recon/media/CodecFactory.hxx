#ifndef CodecFactory_hxx
#define CodecFactory_hxx

#include "rutil/Data.hxx"
#include "resip/stack/SdpContents.hxx"
#include "sdp/SdpMediaLine.hxx"
#include "sdp/SdpCodec.hxx"

namespace recon
{
/**
  Implemented by the application to provide recon with the current list of audio/video codecs
  that should be used in SDP offers/answers.
*/
class CodecFactory
{
public:
   /**
     Returns the complete list of audio/video codecs to be included in an SDP offer/answer,
     regardless of licensing.

     @param includeAudio If true, include audio codecs
     @param includeVideo If true, include video codecs
   */
   virtual void getCodecs(bool includeAudio, bool includeVideo, sdpcontainer::SdpMediaLine::CodecList& codecs) = 0;

   /**
     Increment the usage count of the codecs passed in, and if the usage count exceeds
     the number of available licenses for a given codec, remove it from the list.
    
     This is necessary for codecs that require per-channel licensing (e.g. G.729).
     If you are only using royalty-free codecs, then there is no need to implement this.
   */
   virtual void acquireLicenses(std::list<resip::SdpContents::Session::Codec>& codecs)
   {
   }

   virtual void releaseLicenses(const std::list<resip::SdpContents::Session::Codec>& codecs)
   {
   }

   /**
     From the list of local (offered) codecs, return the one that best matches the
     remote codec.
   */
   virtual sdpcontainer::SdpCodec* getBestMatchingCodec(
      const sdpcontainer::SdpMediaLine::CodecList& localCodecs,
      const sdpcontainer::SdpCodec& remoteCodec
   ) = 0;
};

class CodecConverter
{
public:
   static void toResipCodec(const sdpcontainer::SdpMediaLine::CodecList& inCodecs, std::list<resip::SdpContents::Session::Codec>& outCodecs)
   {
      sdpcontainer::SdpMediaLine::CodecList::const_iterator it = inCodecs.begin();
      for (; it != inCodecs.end(); ++it)
      {
         resip::SdpContents::Session::Codec c(
            it->getMimeSubtype(),
            it->getPayloadType(),
            it->getRate());
         c.parameters() = it->getFormatParameters();
         outCodecs.push_back(c);
      }
   }

   static void toSdpCodec(const std::list<resip::SdpContents::Session::Codec>& inCodecs, const resip::Data& mediaType, sdpcontainer::SdpMediaLine::CodecList& outCodecs)
   {
      std::list<resip::SdpContents::Session::Codec>::const_iterator it = inCodecs.begin();
      for (; it != inCodecs.end(); ++it)
      {
         sdpcontainer::SdpCodec c(
            it->payloadType(),
            mediaType.c_str(),
            it->getName().c_str(),
            it->getRate(),
            0,
            0,
            it->parameters().c_str());
         outCodecs.push_back(c);
      }
   }

   virtual ~CodecConverter() {}

private:
   CodecConverter() {}
};
}

#endif // CodecFactory_hxx