#if !defined(Kurento_Object_hxx)
#define Kurento_Object_hxx

#include <functional>
#include <string>

#include "Continuation.hxx"
#include "Event.hxx"
#include "KurentoConnection.hxx"
#include "KurentoResponseHandler.hxx"

namespace kurento
{

class MediaProfileSpecType
{
   static const char *JPEG_VIDEO_ONLY;
   static const char *KURENTO_SPLIT_RECORDER;
   static const char *MP4;
   static const char *MP4_AUDIO_ONLY;
   static const char *MP4_VIDEO_ONLY;
   static const char *WEBM;
   static const char *WEBM_AUDIO_ONLY;
   static const char *WEBM_VIDEO_ONLY;
};

class Object : public KurentoResponseHandler
{
   public:
      bool valid() { return !mId.empty(); };
      const std::string& getName() const { return mName; };
      const std::string& getId() const { return mId; };

      std::shared_ptr<KurentoConnection> getConnection() { return mConnection; };

      void processResponse(const std::string& id, std::shared_ptr<KurentoResponseHandler> krh, const json::Object& message) override;

      virtual void create(ContinuationVoid c) { createObject(c); };
      virtual void release(ContinuationVoid c);

      virtual void onEvent(const std::string& eventType, const json::Object& message);
      void sendNotifications(const std::string& eventType, const json::Object& message);

   protected:
      Object(const std::string& name, std::shared_ptr<KurentoConnection> connection);
      virtual ~Object();

      typedef std::function<void(const json::Object& message) >
      ContinuationInternal;

      void createObject(ContinuationVoid c, const json::Object& constructorParams = json::Object());
      void invokeVoidMethod(const std::string& methodName, ContinuationVoid c, const json::Object& methodParams = json::Object());
      void invokeStringMethod(const std::string& methodName, ContinuationString c, const json::Object& methodParams = json::Object());

      void addListener(const std::string& eventName, std::shared_ptr<EventListener> l, ContinuationVoid c);

   private:

      std::string makeRpcCallStatic(const std::string& methodName, const json::Object& params, ContinuationInternal c);
      std::string makeRpcCall(const std::string& methodName, json::Object& params, ContinuationInternal c);

      void subscribe(const std::string& eventName, ContinuationVoid c);

      void onConstructorSuccess(ContinuationVoid c, const json::Object& message);
      void onSubscribeSuccess(ContinuationVoid c, const json::Object& message);
      void onVoidSuccess(ContinuationVoid c, const json::Object& message);
      void onStringSuccess(ContinuationString c, const json::Object& message);

      std::string mName;
      std::string mId;
      std::shared_ptr<KurentoConnection> mConnection;

      typedef std::map<std::string, ContinuationInternal> ContinuationMap;
      ContinuationMap mContinuations;

      typedef std::vector<std::shared_ptr<EventListener> > EventListenerList;
      typedef std::map<std::string, EventListenerList> EventListenerMap;
      EventListenerMap mEventListeners;
};

class MediaPipeline : public Object
{
   public:
      MediaPipeline(std::shared_ptr<KurentoConnection> connection);
      virtual ~MediaPipeline();
};

class MediaElement : public Object
{
   public:
      virtual void create(ContinuationVoid c) override;

      void connect(ContinuationVoid c, MediaElement& element);
      void disconnect(ContinuationVoid c, MediaElement& element);
      void disconnect(ContinuationVoid c);

      void addErrorListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addConnectionStateChangedListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addMediaStateChangedListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addMediaTranscodingStateChangeListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addMediaFlowInStateChangeListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addMediaFlowOutStateChangeListener(std::shared_ptr<EventListener> l, ContinuationVoid c);

   protected:
      MediaElement(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline);
      virtual ~MediaElement();
      virtual void create(ContinuationVoid c, const json::Object& extraPparams);

   private:
      virtual void setConnectedTo(const std::string& connectedTo) { mConnectedTo = connectedTo; };

      std::shared_ptr<MediaPipeline> mMediaPipeline;
      std::string mConnectedTo;
};

class PassThroughElement : public MediaElement
{
   public:
      PassThroughElement(std::shared_ptr<MediaPipeline> mediaPipeline);
      virtual ~PassThroughElement();
};

class GStreamerFilter : public MediaElement
{
   public:
      GStreamerFilter(std::shared_ptr<MediaPipeline> mediaPipeline, const std::string& command);
      virtual ~GStreamerFilter();
      virtual void create(ContinuationVoid c) override;
   private:
      std::string mCommand;
};

class Endpoint : public MediaElement
{
   protected:
      Endpoint(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline);
      virtual ~Endpoint();
};

class UriEndpoint : public Endpoint
{
   public:
	  virtual void create(ContinuationVoid c) override;

	  void pause(ContinuationVoid c);
	  void stop(ContinuationVoid c);

   protected:
      UriEndpoint(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline, const std::string& uri);
      virtual ~UriEndpoint();
      virtual void create(ContinuationVoid c, const json::Object& extraParams) override;

   private:
      std::string mUri;
};

class PlayerEndpoint : public UriEndpoint
{
   public:
 	   PlayerEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline, const std::string& uri);
	   virtual ~PlayerEndpoint();
	   void play(ContinuationVoid c);
};

class RecorderEndpoint : public UriEndpoint
{
   public:
      RecorderEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline, const std::string& uri, const char *mediaProfile);
      virtual ~RecorderEndpoint();
      virtual void create(ContinuationVoid c) override;
      void record(ContinuationVoid c);
      void stopAndWait(ContinuationVoid c);
   private:
      const char *mMediaProfile;
};

class BaseRtpEndpoint : public Endpoint
{
   protected:
      BaseRtpEndpoint(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline);

   public:
      virtual ~BaseRtpEndpoint();
      void setExternalIPv4(ContinuationVoid c, const std::string& addr);
      void setExternalIPv6(ContinuationVoid c, const std::string& addr);
      void generateOffer(ContinuationString s);
      void processAnswer(ContinuationString s, const std::string& sdp);
      void processOffer(ContinuationString s, const std::string& sdp);
      void getLocalSessionDescriptor(ContinuationString s);
      void sendPictureFastUpdate(ContinuationVoid c);

      void addKeyframeRequiredListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
};

class RtpEndpoint : public BaseRtpEndpoint
{
   public:
      RtpEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline);
      virtual ~RtpEndpoint();
};

class SipRtpEndpoint : public BaseRtpEndpoint
{
   public:
      SipRtpEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline, bool cryptoAgnostic = true);
      virtual ~SipRtpEndpoint();
      virtual void create(ContinuationVoid c) override;
   private:
      bool mCryptoAgnostic;
};

class WebRtcEndpoint : public BaseRtpEndpoint
{
   public:
      WebRtcEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline);
      virtual ~WebRtcEndpoint();

      void gatherCandidates(ContinuationVoid c);
      void addIceCandidate(ContinuationVoid c, const std::string& candidate, const std::string& mid, unsigned int lineIndex);

      void addOnIceCandidateFoundListener(std::shared_ptr<EventListener> l, ContinuationVoid c);
      void addOnIceGatheringDoneListener(std::shared_ptr<EventListener> l, ContinuationVoid c);

};

}

#endif

/* ====================================================================

 Copyright (c) 2021, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
