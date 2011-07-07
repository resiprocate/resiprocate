#if !defined(StunServerEndPoint_hxx)
#define StunServerEndPoint_hxx


#include "EndPoint.hxx"
#include "resip/stack/Uri.hxx"
#include "tfm/TransportDriver.hxx"

#include "tfm/StunServer.hxx"
#include "tfm/StunEvent.hxx"

#include <deque>

class ActionBase;
class StunAction;
class StunMakeResponse;

class StunEndPoint: public EndPoint,
                    public TransportDriver::Client,
                    public StunSink
{
   public:
      StunEndPoint(const resip::Data& primaryIp,
                   const resip::Data& secondaryIp,
                   int primaryPort,
                   int secondaryPort);

      ~StunEndPoint();

      int init();

      void close();

      void clean();

      virtual void process(resip::FdSet& fdset);

      virtual void buildFdSet(resip::FdSet& fdset);

      virtual resip::Data getName() const;

      resip::Uri getUri() const { return mAddr; }

      // actions
      ActionBase* generateBindingResponse(const resip::Uri& mappedAddr);

      ActionBase* generateBindingResponse();

      ActionBase* generateSymmetricBindingResponse();

      ActionBase* generateAllocateResponse();

      ActionBase* generateAllocateErrorResponse300(const resip::Data& ip, int port);

      ActionBase* generateAllocateErrorResponse(int code);

      ActionBase* generateSendResponse();

      ActionBase* generateSetActiveDestinationResponse();

      // expects
      ExpectBase* expect(StunEvent::Type type,
                         int timeoutMs,
                         ActionBase* expectAction);

   private:
      void generateBindingResponseDelegate();

      void generateBindingResponseDelegate(const resip::Uri& mappedAddr);

      void generateAllocateResponseDelegate();

      void generateSymmetricBindingResponseDelegate();

      void generateAllocateErrorResponse300Delegate(const resip::Data& ip, int port);

      void generateAllocateErrorResponseDelegate(int code);

      void generateSendResponseDelegate();

      void generateSetActiveDestinationResponseDelegate();

   private:
      // StunSink interface
      virtual void onBindingRequest(boost::shared_ptr<StunRequestContext> request);

      virtual void onAllocateRequest(boost::shared_ptr<StunRequestContext> request);

      virtual void onSendRequest(boost::shared_ptr<StunRequestContext> request);

      virtual void onSetActiveDestinationRequest(boost::shared_ptr<StunRequestContext> request);

      virtual void onUnknownRequest(boost::shared_ptr<StunRequestContext> request);

      virtual void onParseMessageFailed();

      virtual void onReceiveMessageFailed();

      virtual void onResponseError();

   private:
      StunServer mStunServer;

      std::deque<boost::shared_ptr<StunRequestContext> > mRequests;

      resip::Uri mAddr;
      resip::Uri mResponseAddr;
};

#endif
