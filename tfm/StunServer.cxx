
#include "tfm/StunServer.hxx"

#include "rutil/Logger.hxx"
#include "rutil/stun/Udp.hxx"
#include "rutil/Lock.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;
using namespace std;
using namespace boost;

#define TURN_MAGIC_COOKIE      0x72c64bc6
#define TURN_MAX_BANDWIDTH     1000
#define TURN_MAX_LIFETIME      500

ostream& operator<<(ostream& strm, const StunAtrAddress4& adr)
{
   strm << adr.ipv4;
   return strm;
}

ostream& operator<<(ostream& strm, const StunMessage& msg)
{
   strm << msg.msgHdr;
   if( msg.hasMappedAddress )
      strm << ", MAPPED-ADDRESS: " << msg.mappedAddress;
   if( msg.hasSourceAddress )
      strm << ", SOURCE-ADDRESS: " << msg.sourceAddress;
   if( msg.hasChangedAddress )
      strm << ", CHANGED-ADDRESS: " << msg.changedAddress;

   return strm;
}

StunServer::StunServer(StunSink* eventSink,
                       const resip::Data& primaryIp,
                       const resip::Data& secondaryIp,
                       int primaryPort,
                       int secondaryPort) :
   mEventSink(eventSink)
{
   stunParseServerName((char*)primaryIp.c_str(), mPrimary);
   mPrimary.port = primaryPort;
   stunParseServerName((char*)secondaryIp.c_str(), mSecondary);
   mSecondary.port = secondaryPort;
}

StunServer::~StunServer()
{
}

int
StunServer::init()
{
   Lock autoLock(mMutexLock);
   bool ok = stunInitServer(mInfo, mPrimary, mSecondary, 0, false);
   InfoLog(<< "STUN listen sockets: " << mInfo.myFd << ", " << mInfo.altPortFd << ", "
      << mInfo.altIpFd << ", " << mInfo.altIpPortFd);

   if( ok )
      return STUN_SUCCESS;
   else
      return STUN_ERROR;
}

void
StunServer::close()
{
   Lock autoLock(mMutexLock);
   stunStopServer(mInfo);

   mInfo.myFd = INVALID_SOCKET;
   mInfo.altPortFd = INVALID_SOCKET;
   mInfo.altIpFd = INVALID_SOCKET;
   mInfo.altIpPortFd = INVALID_SOCKET;
}

void 
StunServer::buildFdSet(FdSet& fdset)
{
   Lock autoLock(mMutexLock);
   if( INVALID_SOCKET != mInfo.myFd )
       fdset.setRead(mInfo.myFd);

   if( INVALID_SOCKET != mInfo.altPortFd )
      fdset.setRead(mInfo.altPortFd);

   if( INVALID_SOCKET != mInfo.altIpFd )
      fdset.setRead(mInfo.altIpFd);

   if( INVALID_SOCKET != mInfo.altIpPortFd )
      fdset.setRead(mInfo.altIpPortFd);
}

void
StunServer::process(FdSet& fdset)
{
   Lock autoLock(mMutexLock);
   bool ok = false;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = sizeof(msg);
   boost::shared_ptr<StunRequestContext> request = boost::shared_ptr<StunRequestContext>(new StunRequestContext());

   resip_assert(mEventSink);

   request->recvAltIp = false;
   request->recvAltPort = false;
   
   if( INVALID_SOCKET != mInfo.myFd && fdset.readyToRead(mInfo.myFd) )
   {
      DebugLog(<< "received on A1:P1");
      request->recvAltIp = false;
      request->recvAltPort = false;
      ok = getMessage( mInfo.myFd, msg, &msgLen, &request->from.addr, &request->from.port,true);
   }
   else if( INVALID_SOCKET != mInfo.altPortFd && fdset.readyToRead(mInfo.altPortFd) )
   {
      DebugLog(<< "received on A1:P2");
      request->recvAltIp = false;
      request->recvAltPort = true;
      ok = getMessage( mInfo.altPortFd, msg, &msgLen, &request->from.addr, &request->from.port,true );
   }
   else if( (INVALID_SOCKET != mInfo.altIpFd) && fdset.readyToRead(mInfo.altIpFd) )
   {
      DebugLog(<< "received on A2:P1");
      request->recvAltIp = true;
      request->recvAltPort = false;
      ok = getMessage( mInfo.altIpFd, msg, &msgLen, &request->from.addr, &request->from.port ,true);
   }
   else if( (INVALID_SOCKET != mInfo.altIpPortFd) && fdset.readyToRead(mInfo.altIpPortFd) )
   {
      DebugLog(<< "received on A2:P2");
      request->recvAltIp = true;
      request->recvAltPort = true;
      ok = getMessage( mInfo.altIpPortFd, msg, &msgLen, &request->from.addr, &request->from.port,true );
   }
   else
      return;

   if( !ok )
   {
      mEventSink->onReceiveMessageFailed();
      return;
   }

   DebugLog(<< "Receive request of size " << msgLen << " from " << request->from);

   request->data.append(msg, msgLen);

   ok = stunParseMessage(msg, msgLen, request->msg, false);

   if( !ok )
   {
      mEventSink->onParseMessageFailed();
      return;
   }

   DebugLog(<< "Message type: " << request->msg.msgHdr.msgType);
   switch( request->msg.msgHdr.msgType )
   {
      case BindRequestMsg:
         mEventSink->onBindingRequest(request);
         break;

      case TurnAllocateRequest:
         mEventSink->onAllocateRequest(request);
         break;

      case TurnSendRequest:
         mEventSink->onSendRequest(request);
         break;

      case TurnSetActiveDestinationRequest:
         mEventSink->onSetActiveDestinationRequest(request);
         break;

      default:
         mEventSink->onUnknownRequest(request);
   }
}

void
StunServer::sendStunResponse(boost::shared_ptr<StunRequestContext> request, const resip::Uri& mappedAddr)
{
   Lock autoLock(mMutexLock);
   resip_assert(mEventSink);

   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   if( !createResponse(request, response) )
   {
      mEventSink->onResponseError();
      return;
   }

   if( mappedAddr.host() != "0.0.0.0" )
   {
      StunAddress4 m;
      stunParseServerName((char*)mappedAddr.host().c_str(), m);
      response->msg.mappedAddress.ipv4.addr = m.addr;
      response->msg.mappedAddress.ipv4.port = mappedAddr.port();
   }

   send(response, request);
}

void
StunServer::sendTurnAllocateErrorResponse(boost::shared_ptr<StunRequestContext> request, int code)
{
   Lock autoLock(mMutexLock);
   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   response->msg.msgHdr.msgType = TurnAllocateErrorResponse;
   for ( int i=0; i<16; i++ )
      response->msg.msgHdr.id.octet[i] = request->msg.msgHdr.id.octet[i];

   response->msg.hasTurnMagicCookie = true;
   response->msg.turnMagicCookie = TURN_MAGIC_COOKIE;

   response->msg.hasErrorCode = true;
   response->msg.errorCode.errorClass = code / 100;
   response->msg.errorCode.number = code % 100;
   Data reason = getReason(code);
   response->msg.errorCode.sizeReason = reason.size();
   strcpy(response->msg.errorCode.reason, reason.c_str());

   response->dest = request->from;

   send(response, request);
}

void
StunServer::sendTurnAllocateErrorResponse300(boost::shared_ptr<StunRequestContext> request, const Data& ip, int port)
{
   Lock autoLock(mMutexLock);
   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   response->msg.msgHdr.msgType = TurnAllocateErrorResponse;
   for ( int i=0; i<16; i++ )
      response->msg.msgHdr.id.octet[i] = request->msg.msgHdr.id.octet[i];

   response->msg.hasTurnMagicCookie = true;
   response->msg.turnMagicCookie = TURN_MAGIC_COOKIE;

   response->msg.hasErrorCode = true;
   response->msg.errorCode.errorClass = 3;
   response->msg.errorCode.number = 0;
   Data reason = getReason(300);
   response->msg.errorCode.sizeReason = reason.size();
   strcpy(response->msg.errorCode.reason, reason.c_str());


   response->msg.hasTurnAlternateServer = true;
   response->msg.turnAlternateServer.family = IPv4Family;
   //stunParseServerName((char*)altAddr.host().c_str(),
   //response->msg.turnAlternateServer.ipv4);
   stunParseServerName((char*)ip.c_str(), response->msg.turnAlternateServer.ipv4);
   response->msg.turnAlternateServer.ipv4.port = port;
   
   response->dest = request->from;

   send(response, request);
}

void
StunServer::sendTurnAllocateResponse(boost::shared_ptr<StunRequestContext> request, int iPort)
{
   Lock autoLock(mMutexLock);
   DebugLog(<< "sending Turn Allocate Reponse");
   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   response->msg.msgHdr.msgType = TurnAllocateResponse;
   for ( int i=0; i<16; i++ )
      response->msg.msgHdr.id.octet[i] = request->msg.msgHdr.id.octet[i];

   response->msg.hasTurnMagicCookie = true;
   response->msg.turnMagicCookie = TURN_MAGIC_COOKIE;

   response->msg.hasMappedAddress = true;
   response->msg.mappedAddress.ipv4 = mPrimary;
   response->msg.mappedAddress.ipv4.port = iPort;
   response->msg.mappedAddress.family = IPv4Family;

   int bandwidth = TURN_MAX_BANDWIDTH;
   if( request->msg.hasTurnBandwidth )
      bandwidth = request->msg.turnBandwidth;

   response->msg.hasTurnBandwidth = true;
   response->msg.turnBandwidth = bandwidth;

   int lifetime = TURN_MAX_LIFETIME;
   if( request->msg.hasTurnLifetime )
   {
      if( (lifetime = request->msg.turnLifetime) > TURN_MAX_LIFETIME )
         lifetime = TURN_MAX_LIFETIME;
   }

   response->msg.hasTurnLifetime = true;
   response->msg.turnLifetime = lifetime;

   response->dest = request->from;

   send(response, request);
}

void
StunServer::sendTurnSendResponse(boost::shared_ptr<StunRequestContext> request)
{
   Lock autoLock(mMutexLock);
   sendTurnResponse(request, TurnSendResponse);
}

void
StunServer::sendTurnSetActiveDestinationResponse(boost::shared_ptr<StunRequestContext> request)
{
   Lock autoLock(mMutexLock);
   sendTurnResponse(request, TurnSetActiveDestinationResponse);
}

void
StunServer::sendTurnResponse(boost::shared_ptr<StunRequestContext> request, UInt16 messageType)
{
   Lock autoLock(mMutexLock);
   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   response->msg.msgHdr.msgType = messageType;
   for ( int i=0; i<16; i++ )
      response->msg.msgHdr.id.octet[i] = request->msg.msgHdr.id.octet[i];

   response->msg.hasTurnMagicCookie = true;
   response->msg.turnMagicCookie = TURN_MAGIC_COOKIE;

   response->dest = request->from;

   send(response, request);
}

void 
StunServer::sendStunResponse(boost::shared_ptr<StunRequestContext> request)
{
   Lock autoLock(mMutexLock);
   resip_assert(mEventSink);

   boost::shared_ptr<StunResponseContext> response = boost::shared_ptr<StunResponseContext>(new StunResponseContext());

   if( !createResponse(request, response) )
   {
      mEventSink->onResponseError();
      return;
   }

   send(response, request);
}

bool
StunServer::createResponse(boost::shared_ptr<StunRequestContext> request, boost::shared_ptr<StunResponseContext> response)
{
   Lock autoLock(mMutexLock);
   response->changePort = false;
   response->changeIp = false;

   response->hmacPassword.sizeValue = 0;

   response->secondary.port = 0;
   response->secondary.addr = 0;

   return stunServerProcessMsg((char*)request->data.data(), 
                               request->data.size(), 
                               request->from, 
                               response->secondary,
                               request->recvAltIp ? mInfo.altAddr : mInfo.myAddr,
                               request->recvAltIp ? mInfo.myAddr : mInfo.altAddr,
                               &response->msg,
                               &response->dest,
                               &response->hmacPassword,
                               &response->changePort,
                               &response->changeIp,
                               false );
}

void 
StunServer::send(boost::shared_ptr<StunResponseContext> response, boost::shared_ptr<StunRequestContext> request)
{
   Lock autoLock(mMutexLock);
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = sizeof(buf);

   len = stunEncodeMessage(response->msg, buf, len, response->hmacPassword, false );
     
   bool ok = true;

   if ( response->dest.addr == 0 )
      ok=false;
   if ( response->dest.port == 0 ) 
      ok=false;

   if( !ok )
   {
      mEventSink->onResponseError();
      return;
   }

   bool sendAltIp   = request->recvAltIp;   // send on the received IP address
   bool sendAltPort = request->recvAltPort; // send on the received port

   if( response->changeIp )
      sendAltIp   = !sendAltIp;   // if need to change IP, then flip logic
   if( response->changePort ) 
      sendAltPort = !sendAltPort; // if need to change port, then flip logic

   Socket sendFd;

   if( !sendAltPort )
   {
      if( !sendAltIp )
         sendFd = mInfo.myFd;
      else
         sendFd = mInfo.altIpFd;
   }
   else
   {
      if( !sendAltIp )
         sendFd = mInfo.altPortFd;
      else
         sendFd = mInfo.altIpPortFd;
   }

   if( INVALID_SOCKET == sendFd )
   {
      mEventSink->onResponseError();
      return;
   }

   DebugLog(<< "Sending response to " << response->dest << ", MA: " << response->msg.mappedAddress.ipv4);
   sendMessage( sendFd, buf, len, response->dest.addr, response->dest.port, true );
}

Data
StunServer::getReason(int code)
{
   switch(code)
   {
      case 300:
         return "Try Alternate";
      case 431:
         return "Shared Secret Error";
      case 434:
         return "Missing Realm";
      case 435:
         return "Missing Nonce";
      case 436:
         return "Unknown UserName";
      case 437:
         return "No Binding";
      case 438:
         return "Send Failed";
      case 439:
         return "Transitioning";
      case 440:
         return "No Destination";
      default:
         return "Unknown";
   }
}
