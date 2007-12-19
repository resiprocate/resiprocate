#include "TurnAsyncSocket.hxx"
#include "../AsyncSocketBase.hxx"
#include "ErrorCode.hxx"
#include <boost/bind.hpp>
#include <rutil/SharedPtr.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

#define UDP_RT0 100  // RTO - Estimate of Roundtrip time - 100ms is recommened for fixed line transport - the initial value should be configurable
                     // Should also be calculation this on the fly
#define UDP_MAX_RETRANSMITS  7       // Defined by RFC3489-bis11
#define TCP_RESPONSE_TIME 7900       // Defined by RFC3489-bis11
#define UDP_FINAL_REQUEST_TIME (UDP_RT0 * 16)  // Defined by RFC3489-bis11

namespace reTurn {

// Initialize static members
unsigned int TurnAsyncSocket::UnspecifiedLifetime = 0xFFFFFFFF;
unsigned int TurnAsyncSocket::UnspecifiedBandwidth = 0xFFFFFFFF; 
unsigned short TurnAsyncSocket::UnspecifiedPort = 0;
asio::ip::address TurnAsyncSocket::UnspecifiedIpAddress = asio::ip::address::from_string("0.0.0.0");

TurnAsyncSocket::TurnAsyncSocket(asio::io_service& ioService, 
                                 AsyncSocketBase& asyncSocketBase,
                                 TurnAsyncSocketHandler* turnAsyncSocketHandler,
                                 const asio::ip::address& address, 
                                 unsigned short port,
                                 bool turnFraming) : 
   mIOService(ioService),
   mTurnAsyncSocketHandler(turnAsyncSocketHandler),
   mTurnFraming(turnFraming),
   mLocalBinding(StunTuple::None /* Set properly by sub class */, address, port),
   mHaveAllocation(false),
   mActiveDestination(0),
   mAsyncSocketBase(asyncSocketBase),
   mAllocationTimer(ioService)
{
}

TurnAsyncSocket::~TurnAsyncSocket()
{
   clearActiveRequestMap();
   cancelAllocationTimer();
   DebugLog(<< "TurnAsyncSocket::~TurnAsyncSocket destroyed!");
}

void
TurnAsyncSocket::disableTurnAsyncHandler()
{
   mTurnAsyncSocketHandler = 0;
}

void
TurnAsyncSocket::requestSharedSecret()
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doRequestSharedSecret, this));
}

void
TurnAsyncSocket::doRequestSharedSecret()
{
   // Should we check here if TLS and deny?

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   // Form Shared Secret request
   StunMessage* request = new StunMessage();
   request->createHeader(StunMessage::StunClassRequest, StunMessage::SharedSecretMethod);

   // Send the Request and start transaction timers
   sendStunMessage(request);
}

void
TurnAsyncSocket::setUsernameAndPassword(const char* username, const char* password)
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doSetUsernameAndPassword, this, new Data(username), new Data(password)));
}

void 
TurnAsyncSocket::doSetUsernameAndPassword(Data* username, Data* password)
{
   mUsername = *username;
   mPassword = *password;
   delete username;
   delete password;
}

void 
TurnAsyncSocket::bindRequest()
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doBindRequest, this));
}

void 
TurnAsyncSocket::doBindRequest()
{
   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   // Form Stun Bind request
   StunMessage* request = new StunMessage();
   request->createHeader(StunMessage::StunClassRequest, StunMessage::BindMethod);

   if(!mUsername.empty())
   {
      request->mHasMessageIntegrity = true;
      request->setUsername(mUsername.c_str()); 
      request->mHmacKey = mPassword;
   }

   sendStunMessage(request);
}

void
TurnAsyncSocket::createAllocation(unsigned int lifetime,
                                  unsigned int bandwidth,
                                  unsigned short requestedPortProps, 
                                  unsigned short requestedPort,
                                  StunTuple::TransportType requestedTransportType, 
                                  const asio::ip::address &requestedIpAddress)
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doCreateAllocation, this, lifetime, 
                                                                           bandwidth, 
                                                                           requestedPortProps, 
                                                                           requestedPort, 
                                                                           requestedTransportType, 
                                                                           requestedIpAddress));
}

void
TurnAsyncSocket::doCreateAllocation(unsigned int lifetime,
                                    unsigned int bandwidth,
                                    unsigned short requestedPortProps, 
                                    unsigned short requestedPort,
                                    StunTuple::TransportType requestedTransportType, 
                                    const asio::ip::address &requestedIpAddress)
{
   // Store Allocation Properties
   mRequestedTransportType = requestedTransportType;

   // Relay Transport Type is requested type or socket type
   if(mRequestedTransportType != StunTuple::None)
   {
      mRelayTransportType = mRequestedTransportType;
   }
   else
   {
      mRelayTransportType = mLocalBinding.getTransportType();
   }

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   if(mHaveAllocation)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::AlreadyAllocated, asio::error::misc_category));
   }

   // Form Turn Allocate request
   StunMessage* request = new StunMessage();
   request->createHeader(StunMessage::StunClassRequest, StunMessage::TurnAllocateMethod);
   if(lifetime != UnspecifiedLifetime)
   {
      request->mHasTurnLifetime = true;
      request->mTurnLifetime = lifetime;
   }
   if(bandwidth != UnspecifiedBandwidth)
   {
      request->mHasTurnBandwidth = true;
      request->mTurnBandwidth = bandwidth;
   }
   if(requestedTransportType != StunTuple::None && requestedTransportType != StunTuple::TLS)
   {      
      request->mHasTurnRequestedTransport = true;
      if(requestedTransportType == StunTuple::UDP)
      {
         request->mTurnRequestedTransport = StunMessage::RequestedTransportUdp;
      }
      else if(requestedTransportType == StunTuple::TCP &&
              mLocalBinding.getTransportType() != StunTuple::UDP)  // Ensure client is not requesting TCP over a UDP transport
      {
         request->mTurnRequestedTransport = StunMessage::RequestedTransportTcp;
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::InvalidRequestedTransport, asio::error::misc_category));
         delete request;
         return;
      }
   }
   if(requestedIpAddress != UnspecifiedIpAddress)
   {
      request->mHasTurnRequestedIp = true;
      StunTuple requestedIpTuple(StunTuple::None, requestedIpAddress, 0);
      StunMessage::setStunAtrAddressFromTuple(request->mTurnRequestedIp, requestedIpTuple);
   }
   if(requestedPortProps != StunMessage::PortPropsNone || requestedPort != UnspecifiedPort)
   {
      request->mHasTurnRequestedPortProps = true;
      request->mTurnRequestedPortProps.props = requestedPortProps;
      request->mTurnRequestedPortProps.port = requestedPort;
   }
   request->mHasMessageIntegrity = true;
   request->setUsername(mUsername.data()); 
   request->mHmacKey = mPassword;

   sendStunMessage(request);
}

void 
TurnAsyncSocket::refreshAllocation(unsigned int lifetime)
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doRefreshAllocation, this, lifetime));
}

void 
TurnAsyncSocket::doRefreshAllocation(unsigned int lifetime)
{
   if(!mHaveAllocation)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(NoAllocation, asio::error::misc_category));
      return;
   }

   // Form Turn Allocate request
   StunMessage* request = new StunMessage();
   request->createHeader(StunMessage::StunClassRequest, StunMessage::TurnRefreshMethod);
   if(lifetime != UnspecifiedLifetime)
   {
      request->mHasTurnLifetime = true;
      request->mTurnLifetime = lifetime;
   }
   //if(mRequestedBandwidth != UnspecifiedBandwidth)
   //{
   //   request.mHasTurnBandwidth = true;
   //   request.mTurnBandwidth = mRequestedBandwidth;
   //}
   request->mHasMessageIntegrity = true;
   request->setUsername(mUsername.data()); 
   request->mHmacKey = mPassword;

   sendStunMessage(request);
}

void 
TurnAsyncSocket::destroyAllocation()
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doDestroyAllocation, this));
}

void 
TurnAsyncSocket::doDestroyAllocation()
{
   doRefreshAllocation(0);
}

void
TurnAsyncSocket::setActiveDestination(const asio::ip::address& address, unsigned short port)
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doSetActiveDestination, this, address, port));
}

void
TurnAsyncSocket::doSetActiveDestination(const asio::ip::address& address, unsigned short port)
{
   // Setup Remote Peer 
   StunTuple remoteTuple(mRelayTransportType, address, port);
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(remotePeer)
   {
      mActiveDestination = remotePeer;
   }
   else
   {
      // No remote peer yet (ie. not data sent or received from remote peer) - so create one
      mActiveDestination = mChannelManager.createRemotePeer(remoteTuple, mChannelManager.getNextChannelNumber(), 0);
      assert(mActiveDestination);
   }
   DebugLog(<< "TurnAsyncSocket::doSetActiveDestination: Active Destination set to: " << remoteTuple);
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSetActiveDestinationSuccess(getSocketDescriptor());
}

void
TurnAsyncSocket::clearActiveDestination()
{
   mIOService.post(boost::bind(&TurnAsyncSocket::doClearActiveDestination, this));
}

void
TurnAsyncSocket::doClearActiveDestination()
{
   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onClearActiveDestinationFailure(getSocketDescriptor(), asio::error_code(reTurn::NoAllocation, asio::error::misc_category));
      return;
   }

   mActiveDestination = 0;
   if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onClearActiveDestinationSuccess(getSocketDescriptor());
}

void
TurnAsyncSocket::sendStunMessage(StunMessage* message, bool reTransmission)
{
#define REQUEST_BUFFER_SIZE 1024
   SharedPtr<Data> buffer = AsyncSocketBase::allocateBuffer(REQUEST_BUFFER_SIZE);
   unsigned int bufferSize;
   if(mTurnFraming)  
   {
      bufferSize = message->stunEncodeFramedMessage((char*)buffer->data(), REQUEST_BUFFER_SIZE);
   }
   else
   {
      bufferSize = message->stunEncodeMessage((char*)buffer->data(), REQUEST_BUFFER_SIZE);
   }
   buffer->truncate(bufferSize);  // set size to real size

   if(!reTransmission)
   {
      // If message is a request, then start appropriate transaction and retranmission timers
      if(message->mClass == StunMessage::StunClassRequest)
      {
         boost::shared_ptr<RequestEntry> requestEntry(new RequestEntry(mIOService, this, message));
         mActiveRequestMap[message->mHeader.magicCookieAndTid] = requestEntry;
         requestEntry->startTimer();
      }
      else
      {
         delete message;
      }
   }

   send(buffer);
}

void 
TurnAsyncSocket::handleReceivedData(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data)
{
   if(mTurnFraming)
   {
      if(data->size() > 4)
      {
         // Get Channel number
         unsigned short channelNumber;
         memcpy(&channelNumber, &(*data)[0], 2);
         channelNumber = ntohs(channelNumber);

         if(channelNumber == 0)
         {
            // Handle Stun Message
            StunMessage* stunMsg = new StunMessage(mLocalBinding, 
                                                   StunTuple(mLocalBinding.getTransportType(), mAsyncSocketBase.getConnectedAddress(), mAsyncSocketBase.getConnectedPort()), 
                                                   &(*data)[4], data->size()-4);
            handleStunMessage(*stunMsg);
            delete stunMsg;
         }
         else
         {
            RemotePeer* remotePeer = mChannelManager.findRemotePeerByServerToClientChannel(channelNumber);
            if(remotePeer)
            {
               if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
                                                         remotePeer->getPeerTuple().getAddress(), 
                                                         remotePeer->getPeerTuple().getPort(), 
                                                         &(*data)[4], data->size()-4);
            }
            else
            {
               WarningLog(<< "TurnAsyncSocket::handleReceivedData: receive channel data for non-existing channel - discarding!");
            }
         }
      }
      else  // size <= 4
      {
         WarningLog(<< "TurnAsyncSocket::handleReceivedData: not enought data received for framed message - discarding!");
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onReceiveFailure(getSocketDescriptor(), asio::error_code(reTurn::FrameError, asio::error::misc_category));         
      }
   }
   else
   {
      // mTurnFraming is disabled - message could be a Stun Message if first 2 bits are also 0
      if(((*data)[0] & 0xC0) == 0)
      {
         StunMessage* stunMsg = new StunMessage(mLocalBinding, 
                                                StunTuple(mLocalBinding.getTransportType(), mAsyncSocketBase.getConnectedAddress(), mAsyncSocketBase.getConnectedPort()), 
                                                &(*data)[0], data->size());
         if(stunMsg->isValid())
         {
            handleStunMessage(*stunMsg);
            delete stunMsg;
            return;
         }
         delete stunMsg;
      }

      // Not a stun message so assume normal data
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
                                                   address, 
                                                   port, 
                                                   &(*data)[0], data->size());
   }
}

asio::error_code 
TurnAsyncSocket::handleStunMessage(StunMessage& stunMessage)
{
   asio::error_code errorCode;
   if(stunMessage.isValid())
   {
      if(!stunMessage.checkMessageIntegrity(mPassword))
      {
         WarningLog(<< "TurnAsyncSocket::handleStunMessage: Stun message integrity is bad!");
         return asio::error_code(reTurn::BadMessageIntegrity, asio::error::misc_category);
      }

      // Request is authenticated, process it
      switch(stunMessage.mClass)
      { 
      case StunMessage::StunClassRequest:
         switch (stunMessage.mMethod) 
         {
         case StunMessage::BindMethod:
            errorCode = handleBindRequest(stunMessage);
            break;
         case StunMessage::SharedSecretMethod:
         case StunMessage::TurnAllocateMethod:
         case StunMessage::TurnRefreshMethod:
         default:
            // These requests are not handled by a client
            StunMessage* response = new StunMessage();
            response->mClass = StunMessage::StunClassErrorResponse;
            response->setErrorCode(400, "Invalid Request Method");  
            // Copy over TransactionId
            response->mHeader.magicCookieAndTid = stunMessage.mHeader.magicCookieAndTid;
            sendStunMessage(response);
            break;
         }
         break;

      case StunMessage::StunClassIndication:
         switch (stunMessage.mMethod) 
         {
         case StunMessage::TurnDataMethod: 
            errorCode = handleDataInd(stunMessage);
            break;
         case StunMessage::TurnChannelConfirmationMethod:
            errorCode = handleChannelConfirmation(stunMessage);
            break;        
         case StunMessage::BindMethod:
            // A Bind indication is simply a keepalive with no response required
            break;
         case StunMessage::TurnSendMethod:  // Don't need to handle these - only sent by client, never received
         default:
            // Unknown indication - just ignore
            break;
         }
         break;
   
      case StunMessage::StunClassSuccessResponse:
      case StunMessage::StunClassErrorResponse:
      {
         // First check if this response if for an active request
         RequestMap::iterator it = mActiveRequestMap.find(stunMessage.mHeader.magicCookieAndTid);
         if(it == mActiveRequestMap.end())
         {
            // Stray response - dropping
            return asio::error_code(reTurn::StrayResponse, asio::error::misc_category);
         }
         else
         {
            it->second->stopTimer();
            mActiveRequestMap.erase(it);
         }

         switch (stunMessage.mMethod) 
         {
         case StunMessage::BindMethod:
            errorCode = handleBindResponse(stunMessage);
            break;
         case StunMessage::SharedSecretMethod:
            errorCode = handleSharedSecretResponse(stunMessage);
            break;
         case StunMessage::TurnAllocateMethod:
            errorCode = handleAllocateResponse(stunMessage);
            break;
         case StunMessage::TurnRefreshMethod:
            errorCode = handleRefreshResponse(stunMessage);
            break;
         default:
            // Unknown method - just ignore
            break;
         }
      }
      break;

      default:
         // Illegal message class - ignore
         break;
      }
   }
   else
   {
      WarningLog(<< "TurnAsyncSocket::handleStunMessage: Read Invalid StunMsg.");
      return asio::error_code(reTurn::ErrorParsingMessage, asio::error::misc_category);
   }
   return errorCode;
}

asio::error_code
TurnAsyncSocket::handleDataInd(StunMessage& stunMessage)
{
   if(!stunMessage.mHasTurnPeerAddress || !stunMessage.mHasTurnChannelNumber)
   {
      // Missing RemoteAddress or ChannelNumber attribute
      WarningLog(<< "TurnAsyncSocket::handleDataInd: DataInd missing attributes.");
      return asio::error_code(reTurn::MissingAttributes, asio::error::misc_category);
   }

   StunTuple remoteTuple;
   remoteTuple.setTransportType(mRelayTransportType);
   StunMessage::setTupleFromStunAtrAddress(remoteTuple, stunMessage.mTurnPeerAddress);

   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(!remotePeer)
   {
      // Remote Peer not found - discard data
      WarningLog(<< "TurnAsyncSocket::handleDataInd: Data received from unknown RemotePeer " << remoteTuple << " - discarding");
      return asio::error_code(reTurn::UnknownRemoteAddress, asio::error::misc_category);
   }

   if(remotePeer->getServerToClientChannel() != 0 && remotePeer->getServerToClientChannel() != stunMessage.mTurnChannelNumber)
   {
      // Mismatched channel number
      WarningLog(<< "TurnAsyncSocket::handleDataInd: Channel number received in DataInd (" << (int)stunMessage.mTurnChannelNumber << ") does not match existing number for RemotePeer (" << (int)remotePeer->getServerToClientChannel() << ").");
      return asio::error_code(reTurn::InvalidChannelNumberReceived, asio::error::misc_category);
   }

   if(!remotePeer->isServerToClientChannelConfirmed())
   {
      remotePeer->setServerToClientChannel(stunMessage.mTurnChannelNumber);
      remotePeer->setServerToClientChannelConfirmed();
      mChannelManager.addRemotePeerServerToClientChannelLookup(remotePeer);
   }

   if(mLocalBinding.getTransportType() == StunTuple::UDP)
   {
      // If UDP, then send TurnChannelConfirmationInd
      StunMessage* channelConfirmationInd = new StunMessage();
      channelConfirmationInd->createHeader(StunMessage::StunClassIndication, StunMessage::TurnChannelConfirmationMethod);
      channelConfirmationInd->mHasTurnPeerAddress = true;
      channelConfirmationInd->mTurnPeerAddress = stunMessage.mTurnPeerAddress;
      channelConfirmationInd->mHasTurnChannelNumber = true;
      channelConfirmationInd->mTurnChannelNumber = stunMessage.mTurnChannelNumber;

      // send channelConfirmationInd to local client
      sendStunMessage(channelConfirmationInd);
   }

   if(stunMessage.mHasTurnData)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
         remoteTuple.getAddress(), 
         remoteTuple.getPort(), 
         stunMessage.mTurnData->data(), (unsigned int)stunMessage.mTurnData->size());
   }

   return asio::error_code();
}

asio::error_code
TurnAsyncSocket::handleChannelConfirmation(StunMessage &stunMessage)
{
   if(!stunMessage.mHasTurnPeerAddress || !stunMessage.mHasTurnChannelNumber)
   {
      // Missing RemoteAddress or ChannelNumber attribute
      WarningLog(<< "TurnAsyncSocket::handleChannelConfirmation: DataInd missing attributes.");
      return asio::error_code(reTurn::MissingAttributes, asio::error::misc_category);
   }

   StunTuple remoteTuple;
   remoteTuple.setTransportType(mRelayTransportType);
   StunMessage::setTupleFromStunAtrAddress(remoteTuple, stunMessage.mTurnPeerAddress);

   RemotePeer* remotePeer = mChannelManager.findRemotePeerByClientToServerChannel(stunMessage.mTurnChannelNumber);
   if(!remotePeer)
   {
      // Remote Peer not found - discard
      WarningLog(<< "TurnAsyncSocket::handleChannelConfirmation: Received ChannelConfirmationInd for unknown channel (" << stunMessage.mTurnChannelNumber << ") - discarding");
      return asio::error_code(reTurn::InvalidChannelNumberReceived, asio::error::misc_category);
   }

   if(remotePeer->getPeerTuple() != remoteTuple)
   {
      // Mismatched remote address
      WarningLog(<< "TurnAsyncSocket::handleChannelConfirmation: RemoteAddress associated with channel (" << remotePeer->getPeerTuple() << ") does not match ChannelConfirmationInd (" << remoteTuple << ").");
      return asio::error_code(reTurn::UnknownRemoteAddress, asio::error::misc_category);
   }

   remotePeer->setClientToServerChannelConfirmed();

   return asio::error_code();
}

asio::error_code 
TurnAsyncSocket::handleSharedSecretResponse(StunMessage& stunMessage)
{
   if(stunMessage.mClass == StunMessage::StunClassSuccessResponse)
   {
      // Copy username and password to callers buffer - checking sizes first
      if(!stunMessage.mHasUsername || !stunMessage.mHasPassword)
      {
         WarningLog(<< "TurnAsyncSocket::handleSharedSecretResponse: Stun response message for SharedSecretRequest is missing username and/or password!");
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }

      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretSuccess(getSocketDescriptor(), stunMessage.mUsername->c_str(), stunMessage.mUsername->size(), 
                                                                            stunMessage.mPassword->c_str(), stunMessage.mPassword->size());
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

asio::error_code
TurnAsyncSocket::handleBindRequest(StunMessage& stunMessage)
{
   // Note: handling of BindRequest is not fully backwards compatible with RFC3489 - it is inline with bis13
   StunMessage* response = new StunMessage();

   // form the outgoing message
   response->mClass = StunMessage::StunClassSuccessResponse;
   response->mMethod = StunMessage::BindMethod;

   // Copy over TransactionId
   response->mHeader.magicCookieAndTid = stunMessage.mHeader.magicCookieAndTid;

   // Add XOrMappedAddress to response 
   response->mHasXorMappedAddress = true;
   StunMessage::setStunAtrAddressFromTuple(response->mXorMappedAddress, stunMessage.mRemoteTuple);

   // send bindResponse to local client
   sendStunMessage(response);

   return asio::error_code();
}

asio::error_code 
TurnAsyncSocket::handleBindResponse(StunMessage& stunMessage)
{
   if(stunMessage.mClass == StunMessage::StunClassSuccessResponse)
   {
      StunTuple reflexiveTuple;
      reflexiveTuple.setTransportType(mLocalBinding.getTransportType());
      if(stunMessage.mHasXorMappedAddress)
      {
         StunMessage::setTupleFromStunAtrAddress(reflexiveTuple, stunMessage.mXorMappedAddress);
      }
      else if(stunMessage.mHasMappedAddress)  // Only look at MappedAddress if XorMappedAddress is not found - for backwards compatibility
      {
         StunMessage::setTupleFromStunAtrAddress(reflexiveTuple, stunMessage.mMappedAddress);
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindSuccess(getSocketDescriptor(), reflexiveTuple);
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

asio::error_code 
TurnAsyncSocket::handleAllocateResponse(StunMessage& stunMessage)
{
   if(stunMessage.mClass == StunMessage::StunClassSuccessResponse)
   {
      StunTuple reflexiveTuple;
      StunTuple relayTuple;
      if(stunMessage.mHasXorMappedAddress)
      {
         reflexiveTuple.setTransportType(mLocalBinding.getTransportType());
         StunMessage::setTupleFromStunAtrAddress(reflexiveTuple, stunMessage.mXorMappedAddress);
      }
      if(stunMessage.mHasTurnRelayAddress)
      {
         relayTuple.setTransportType(mRelayTransportType);
         StunMessage::setTupleFromStunAtrAddress(relayTuple, stunMessage.mTurnRelayAddress);
      }
      if(stunMessage.mHasTurnLifetime)
      {
         mLifetime = stunMessage.mTurnLifetime;
      }
      else
      {
         mLifetime = 0;
      }

      // All was well - return 0 errorCode
      if(mLifetime != 0)
      {
         mHaveAllocation = true;
         startAllocationTimer();
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationSuccess(getSocketDescriptor(), reflexiveTuple, relayTuple, mLifetime, stunMessage.mHasTurnBandwidth ? stunMessage.mTurnBandwidth : 0);
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
      }
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

asio::error_code 
TurnAsyncSocket::handleRefreshResponse(StunMessage& stunMessage)
{
   if(stunMessage.mClass == StunMessage::StunClassSuccessResponse)
   {
      if(stunMessage.mHasTurnLifetime)
      {
         mLifetime = stunMessage.mTurnLifetime;
      }
      else
      {
         mLifetime = 0;
      }
      if(mLifetime != 0)
      {
         mHaveAllocation = true;
         startAllocationTimer();
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshSuccess(getSocketDescriptor(), mLifetime);
      }
      else
      {
         cancelAllocationTimer();
         mHaveAllocation = false;
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshSuccess(getSocketDescriptor(), 0);
      }
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

void
TurnAsyncSocket::send(const char* buffer, unsigned int size)
{
   SharedPtr<Data> data(new Data(buffer, size));
   mIOService.post(boost::bind(&TurnAsyncSocket::doSend, this, data));
}

void
TurnAsyncSocket::doSend(resip::SharedPtr<resip::Data> data)
{
   // Allow raw data to be sent if there is no allocation
   if(!mHaveAllocation)
   {
      return send(data);
   }

   if(!mActiveDestination)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSendFailure(getSocketDescriptor(), asio::error_code(reTurn::NoActiveDestination, asio::error::misc_category));
      return;
   }

   return sendTo(*mActiveDestination, data);
}

void 
TurnAsyncSocket::sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   SharedPtr<Data> data(new Data(buffer, size));
   mIOService.post(boost::bind(&TurnAsyncSocket::doSendTo, this, address, port, data));
}

void 
TurnAsyncSocket::doSendTo(const asio::ip::address& address, unsigned short port, resip::SharedPtr<resip::Data> data)
{
   // ensure there is an allocation 
   if(!mHaveAllocation)
   {
      if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSendFailure(getSocketDescriptor(), asio::error_code(reTurn::NoAllocation, asio::error::misc_category));
      return; 
   }

   // Setup Remote Peer 
   StunTuple remoteTuple(mRelayTransportType, address, port);
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(!remotePeer)
   {
      // No remote peer yet (ie. not data sent or received from remote peer) - so create one
      remotePeer = mChannelManager.createRemotePeer(remoteTuple, mChannelManager.getNextChannelNumber(), 0);
      assert(remotePeer);
   }
   return sendTo(*remotePeer, data);
}

void
TurnAsyncSocket::sendTo(RemotePeer& remotePeer, resip::SharedPtr<resip::Data> data)
{
   if(remotePeer.isClientToServerChannelConfirmed())
   {
      // send framed data to active destination
      send(remotePeer.getClientToServerChannel(), data);
   }
   else
   {
      // Data must be wrapped in a Send Indication
      // Wrap data in a SendInd
      StunMessage* ind = new StunMessage();
      ind->createHeader(StunMessage::StunClassIndication, StunMessage::TurnSendMethod);
      ind->mHasTurnPeerAddress = true;
      StunMessage::setStunAtrAddressFromTuple(ind->mTurnPeerAddress, remotePeer.getPeerTuple());
      ind->mHasTurnChannelNumber = true;
      ind->mTurnChannelNumber = remotePeer.getClientToServerChannel();
      if(data->size() > 0)
      {
         ind->setTurnData(data->data(), data->size());
      }

      // If not using UDP - then mark channel as confirmed
      if(mLocalBinding.getTransportType() != StunTuple::UDP)
      {
         remotePeer.setClientToServerChannelConfirmed();
      }

      // Send indication to Turn Server
      sendStunMessage(ind);
   }
}

void
TurnAsyncSocket::connect(const std::string& address, unsigned short port, bool turnFraming)
{
   mTurnFraming = turnFraming;
   mAsyncSocketBase.connect(address,port);
}

void
TurnAsyncSocket::close()
{
   clearActiveRequestMap();
   cancelAllocationTimer();
   mAsyncSocketBase.close();
}

void 
TurnAsyncSocket::turnReceive()
{
   if(mTurnFraming)
   {
      mAsyncSocketBase.framedReceive();
   }
   else
   {
      mAsyncSocketBase.receive();
   }
}

void 
TurnAsyncSocket::send(resip::SharedPtr<resip::Data> data)
{
   StunTuple destination(mLocalBinding.getTransportType(), mAsyncSocketBase.getConnectedAddress(), mAsyncSocketBase.getConnectedPort());
   mAsyncSocketBase.send(destination, data);
}

void 
TurnAsyncSocket::send(unsigned short channel, resip::SharedPtr<resip::Data> data)
{
   StunTuple destination(mLocalBinding.getTransportType(), mAsyncSocketBase.getConnectedAddress(), mAsyncSocketBase.getConnectedPort());
   mAsyncSocketBase.send(destination, channel, data);
}

TurnAsyncSocket::RequestEntry::RequestEntry(asio::io_service& ioService, 
                                            TurnAsyncSocket* turnAsyncSocket, 
                                            StunMessage* requestMessage) : 
   mIOService(ioService), 
   mTurnAsyncSocket(turnAsyncSocket), 
   mRequestMessage(requestMessage), 
   mRequestTimer(ioService),
   mRequestsSent(1)
{
   mTimeout = mTurnAsyncSocket->mLocalBinding.getTransportType() == StunTuple::UDP ? UDP_RT0 : TCP_RESPONSE_TIME;
}

void
TurnAsyncSocket::RequestEntry::startTimer()
{
   // start the request timer
   mRequestTimer.expires_from_now(boost::posix_time::milliseconds(mTimeout));  
   mRequestTimer.async_wait(boost::bind(&TurnAsyncSocket::RequestEntry::requestTimerExpired, shared_from_this(), asio::placeholders::error));
}

void
TurnAsyncSocket::RequestEntry::stopTimer()
{
   // stop the request timer
   mRequestTimer.cancel();
}

TurnAsyncSocket::RequestEntry::~RequestEntry() 
{ 
   delete mRequestMessage; 
}

void 
TurnAsyncSocket::RequestEntry::requestTimerExpired(const asio::error_code& e)
{
   if(!e)
   {
      if(mTurnAsyncSocket->mLocalBinding.getTransportType() != StunTuple::UDP || mRequestsSent == UDP_MAX_RETRANSMITS)
      {
         mTurnAsyncSocket->requestTimeout(mRequestMessage->mHeader.magicCookieAndTid);
         return;
      }
      // timed out and should retransmit - calculate next timeout
      if(mRequestsSent == UDP_MAX_RETRANSMITS - 1)
      {
          mTimeout = UDP_FINAL_REQUEST_TIME;
      } 
      else
      {
          mTimeout = (mTimeout*2);
      }
      // retransmit
      DebugLog(<< "RequestEntry::requestTimerExpired: retransmitting...");
      mRequestsSent++;
      mTurnAsyncSocket->sendStunMessage(mRequestMessage, true);

      startTimer();
   }
}

void 
TurnAsyncSocket::requestTimeout(UInt128 tid)
{
   RequestMap::iterator it = mActiveRequestMap.find(tid);
   if(it != mActiveRequestMap.end())
   {
      switch(it->second->mRequestMessage->mMethod)
      {
      case StunMessage::BindMethod:
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(reTurn::ResponseTimeout, asio::error::misc_category));
         break;
      case StunMessage::SharedSecretMethod:
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(reTurn::ResponseTimeout, asio::error::misc_category));
         break;
      case StunMessage::TurnAllocateMethod:
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::ResponseTimeout, asio::error::misc_category));
         break;
      case StunMessage::TurnRefreshMethod:
         if(mTurnAsyncSocketHandler) mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(reTurn::ResponseTimeout, asio::error::misc_category));
         break;
      default:
         assert(false);
      }
      mActiveRequestMap.erase(it);
   }
}

void
TurnAsyncSocket::clearActiveRequestMap()
{
   // Clear out active request map - !slg! TODO this really should happen from the io service thread
   RequestMap::iterator it = mActiveRequestMap.begin();
   for(;it != mActiveRequestMap.end(); it++)
   {
      it->second->stopTimer();
   }
   mActiveRequestMap.clear();
}

void
TurnAsyncSocket::startAllocationTimer()
{
   mAllocationTimer.expires_from_now(boost::posix_time::seconds((mLifetime*5)/8));  // Allocation refresh should sent before 3/4 lifetime - use 5/8 lifetime 
   mAllocationTimer.async_wait(boost::bind(&TurnAsyncSocket::allocationTimerExpired, this, asio::placeholders::error));
}

void
TurnAsyncSocket::cancelAllocationTimer()
{
   mAllocationTimer.cancel();
}

void 
TurnAsyncSocket::allocationTimerExpired(const asio::error_code& e)
{
   if(!e)
   {
      doRefreshAllocation(mLifetime);
   }
}

} // namespace


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */


