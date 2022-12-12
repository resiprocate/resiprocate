#if !defined StunServer_hxx
#define StunServer_hxx

#include "rutil/stun/Stun.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Mutex.hxx"

#include <iostream>
#include <memory>

std::ostream& operator<<(std::ostream& strm, const StunMessage& msg);
std::ostream& operator<<(std::ostream& strm, const StunAtrAddress4& adr);

#define STUN_SUCCESS   0
#define STUN_ERROR    -1
#define STUN_NOEVENT  -2

struct StunRequestContext
{
   StunRequestContext() :
      data(),
      msg(),
      recvAltIp(false),
      recvAltPort(false)
   {
      from.port = 0;
      from.addr = 0;
   }

   resip::Data data;
   StunMessage msg;
   StunAddress4 from;
   bool recvAltIp;
   bool recvAltPort;
};

struct StunResponseContext
{
   StunResponseContext() :
      msg(),
      changePort(false),
      changeIp(false)
   {
      dest.port = 0;
      dest.addr = 0;
      hmacPassword.sizeValue = 0;
      secondary.port = 0;
      secondary.addr = 0;
   }

   StunMessage msg;
   StunAddress4 dest;
   StunAtrString hmacPassword;
   StunAddress4 secondary;
   bool changePort;
   bool changeIp;
};

class StunSink
{
   public:
      virtual ~StunSink() {}
      
      virtual void onBindingRequest(std::shared_ptr<StunRequestContext> request) = 0;

      virtual void onAllocateRequest(std::shared_ptr<StunRequestContext> request) = 0;

      virtual void onSendRequest(std::shared_ptr<StunRequestContext> request) = 0;

      virtual void onSetActiveDestinationRequest(std::shared_ptr<StunRequestContext> request) = 0;

      virtual void onUnknownRequest(std::shared_ptr<StunRequestContext> request) = 0;

      virtual void onParseMessageFailed() = 0;

      virtual void onReceiveMessageFailed() = 0;

      virtual void onResponseError() = 0;
};

class StunServer
{
   public:
      StunServer(StunSink* eventSink,
                 const resip::Data& primaryIp,
                 const resip::Data& secondaryIp,
                 int primaryPort,
                 int secondaryPort);

      ~StunServer();

      /**
       * Opening file descriptors.
       */
      int init();

      /**
       * Closing file descriptors.
       */
      void close();

      void buildFdSet(resip::FdSet& fdset);

      void process(resip::FdSet& fdset);

      void sendStunResponse(std::shared_ptr<StunRequestContext> request);

      void sendStunResponse(std::shared_ptr<StunRequestContext> request, const resip::Uri& mappedAddr);

      void sendTurnAllocateResponse(std::shared_ptr<StunRequestContext> request, int iPort = 8000);

      void sendTurnAllocateErrorResponse(std::shared_ptr<StunRequestContext> request, int code);

      //void
      //sendTurnAllocateErrorResponse300(std::shared_ptr<StunRequestContext>
      //request, const resip::Uri& altAddr);
      void sendTurnAllocateErrorResponse300(std::shared_ptr<StunRequestContext> request, const resip::Data& ip, int port);

      void sendTurnSendResponse(std::shared_ptr<StunRequestContext> request);

      void sendTurnSetActiveDestinationResponse(std::shared_ptr<StunRequestContext> request);

   private:
      void sendTurnResponse(std::shared_ptr<StunRequestContext> request, uint16_t messageType);

      bool createResponse(std::shared_ptr<StunRequestContext> request, std::shared_ptr<StunResponseContext> response);

      void send(std::shared_ptr<StunResponseContext> response, std::shared_ptr<StunRequestContext> request);

      resip::Data getReason(int code);

   private:
      resip::Mutex mMutexLock;
      StunAddress4 mPrimary;
      StunAddress4 mSecondary;
      StunServerInfo mInfo;

      StunSink* mEventSink;
};
#endif
