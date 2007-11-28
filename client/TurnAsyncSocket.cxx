#include "TurnAsyncSocket.hxx"
#include "../AsyncSocketBase.hxx"
#include "ErrorCode.hxx"
#include <boost/bind.hpp>
#include <rutil/SharedPtr.hxx>

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
   mAsyncSocketBase(asyncSocketBase)
{
   assert(mTurnAsyncSocketHandler);
}

TurnAsyncSocket::~TurnAsyncSocket()
{
}

void
TurnAsyncSocket::requestSharedSecret()
{
   asio::error_code errorCode;

   // Should we check here if TLS and deny?

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   // Form Shared Secret request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::SharedSecretMethod);

   // Send the Request and start transaction timers
   sendStunMessage(request);
}

void 
TurnAsyncSocket::setUsernameAndPassword(const char* username, const char* password)
{
   mUsername = username;
   mPassword = password;
}

void 
TurnAsyncSocket::bindRequest()
{
   asio::error_code errorCode;

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   // Form Stun Bind request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::BindMethod);

   if(!mUsername.empty())
   {
      request.mHasMessageIntegrity = true;
      request.setUsername(mUsername.c_str()); 
      request.mHmacKey = mPassword;
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
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Store Allocation Properties
   mRequestedTransportType = requestedTransportType;

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::NotConnected, asio::error::misc_category));
   }

   if(mHaveAllocation)
   {
      mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::AlreadyAllocated, asio::error::misc_category));
   }

   // Form Turn Allocate request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnAllocateMethod);
   if(lifetime != UnspecifiedLifetime)
   {
      request.mHasTurnLifetime = true;
      request.mTurnLifetime = lifetime;
   }
   if(bandwidth != UnspecifiedBandwidth)
   {
      request.mHasTurnBandwidth = true;
      request.mTurnBandwidth = bandwidth;
   }
   if(mRequestedTransportType != StunTuple::None && mRequestedTransportType != StunTuple::TLS)
   {      
      request.mHasTurnRequestedTransport = true;
      if(mRequestedTransportType == StunTuple::UDP)
      {
         request.mTurnRequestedTransport = StunMessage::RequestedTransportUdp;
      }
      else if(mRequestedTransportType == StunTuple::TCP &&
              mLocalBinding.getTransportType() != StunTuple::UDP)  // Ensure client is not requesting TCP over a UDP transport
      {
         request.mTurnRequestedTransport = StunMessage::RequestedTransportTcp;
      }
      else
      {
         mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(reTurn::InvalidRequestedTransport, asio::error::misc_category));
      }
   }
   if(requestedIpAddress != UnspecifiedIpAddress)
   {
      request.mHasTurnRequestedIp = true;
      request.mTurnRequestedIp.port = 0;  // Not relevant
      if(requestedIpAddress.is_v6())
      {
         request.mTurnRequestedIp.family = StunMessage::IPv6Family;  
         memcpy(&request.mTurnRequestedIp.addr.ipv6, requestedIpAddress.to_v6().to_bytes().c_array(), sizeof(request.mTurnRequestedIp.addr.ipv6));
      }
      else
      {
         request.mTurnRequestedIp.family = StunMessage::IPv4Family;  
         request.mTurnRequestedIp.addr.ipv4 = requestedIpAddress.to_v4().to_ulong();   
      }
   }
   if(requestedPortProps != StunMessage::PortPropsNone || requestedPort != UnspecifiedPort)
   {
      request.mHasTurnRequestedPortProps = true;
      request.mTurnRequestedPortProps.props = requestedPortProps;
      request.mTurnRequestedPortProps.port = requestedPort;
   }
   request.mHasMessageIntegrity = true;
   request.setUsername(mUsername.data()); 
   request.mHmacKey = mPassword;

   sendStunMessage(request);
}

void 
TurnAsyncSocket::refreshAllocation(unsigned int lifetime)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Form Turn Allocate request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnRefreshMethod);
   if(lifetime != UnspecifiedLifetime)
   {
      request.mHasTurnLifetime = true;
      request.mTurnLifetime = lifetime;
   }
   //if(mRequestedBandwidth != UnspecifiedBandwidth)
   //{
   //   request.mHasTurnBandwidth = true;
   //   request.mTurnBandwidth = mRequestedBandwidth;
   //}
   request.mHasMessageIntegrity = true;
   request.setUsername(mUsername.data()); 
   request.mHmacKey = mPassword;

   sendStunMessage(request);
}

void 
TurnAsyncSocket::destroyAllocation()
{
   resip::Lock lock(mMutex);
   if(mHaveAllocation)
   {
      refreshAllocation(0);
   }
   else
   {
      mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(NoAllocation, asio::error::misc_category));
   }
}

asio::error_code 
TurnAsyncSocket::setActiveDestination(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::error::misc_category); 
   }

   // Ensure Connected
   if(!mAsyncSocketBase.isConnected())
   {
      return asio::error_code(reTurn::NotConnected, asio::error::misc_category); 
   }

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

   return errorCode;
}

asio::error_code 
TurnAsyncSocket::clearActiveDestination()
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::error::misc_category); 
   }

   mActiveDestination = 0;

   return errorCode;
}

void
TurnAsyncSocket::sendStunMessage(StunMessage& message)
{
#define REQUEST_BUFFER_SIZE 1024
   SharedPtr<Data> buffer = AsyncSocketBase::allocateBuffer(REQUEST_BUFFER_SIZE);
   unsigned int bufferSize;
   if(mTurnFraming)  
   {
      bufferSize = message.stunEncodeFramedMessage((char*)buffer->data(), REQUEST_BUFFER_SIZE);
   }
   else
   {
      bufferSize = message.stunEncodeMessage((char*)buffer->data(), REQUEST_BUFFER_SIZE);
   }
   buffer->truncate(bufferSize);  // set size to real size

   send(buffer);

   // TODO - start retransmit, and response timeout timer - if a request
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
               mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
                                                         remotePeer->getPeerTuple().getAddress(), 
                                                         remotePeer->getPeerTuple().getPort(), 
                                                         &(*data)[4], data->size()-4);
            }
            else
            {
               cout << "TurnAsyncSocket::handleReceivedData: receive channel data for non-existing channel - discarding!" << endl;
            }
         }
      }
      else  // size <= 4
      {
         cout << "TurnAsyncSocket::handleReceivedData: not enought data received for framed message - discarding!" << endl;
         mTurnAsyncSocketHandler->onReceiveFailure(getSocketDescriptor(), asio::error_code(reTurn::FrameError, asio::error::misc_category));         
      }
   }
   else
   {
      // mTurnFraming is disabled - message should be a Stun Message
      StunMessage* stunMsg = new StunMessage(mLocalBinding, 
                                             StunTuple(mLocalBinding.getTransportType(), mAsyncSocketBase.getConnectedAddress(), mAsyncSocketBase.getConnectedPort()), 
                                             &(*data)[0], data->size());
      if(stunMsg->isValid())
      {
         handleStunMessage(*stunMsg);
      }
      else
      {
         // Not a stun message so assume normal data
         mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
                                                   address, 
                                                   port, 
                                                   &(*data)[0], data->size());
      }
      delete stunMsg;
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
         std::cout << "TurnAsyncSocket::handleStunMessage: Stun message integrity is bad!" << std::endl;
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
            StunMessage response;
            response.mClass = StunMessage::StunClassErrorResponse;
            response.setErrorCode(400, "Invalid Request Method");  
            // Copy over TransactionId
            response.mHeader.magicCookieAndTid = stunMessage.mHeader.magicCookieAndTid;
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
         break;

      default:
         // Illegal message class - ignore
         break;
      }
   }
   else
   {
      std::cout << "TurnAsyncSocket::handleStunMessage: Read Invalid StunMsg." << std::endl;
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
      std::cout << "TurnAsyncSocket::handleStunMessage: DataInd missing attributes." << std::endl;
      return asio::error_code(reTurn::MissingAttributes, asio::error::misc_category);
   }

   StunTuple remoteTuple;
   remoteTuple.setTransportType(mRelayTransportType);
   remoteTuple.setPort(stunMessage.mTurnPeerAddress.port);
   if(stunMessage.mTurnPeerAddress.family == StunMessage::IPv6Family)
   {
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), &stunMessage.mTurnPeerAddress.addr.ipv6, bytes.size());
      remoteTuple.setAddress(asio::ip::address_v6(bytes));
   }
   else
   {
      remoteTuple.setAddress(asio::ip::address_v4(stunMessage.mTurnPeerAddress.addr.ipv4));
   }

   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(!remotePeer)
   {
      // Remote Peer not found - discard data
      std::cout << "TurnAsyncSocket::handleStunMessage: Data received from unknown RemotePeer - discarding" << std::endl;
      return asio::error_code(reTurn::UnknownRemoteAddress, asio::error::misc_category);
   }

   if(remotePeer->getServerToClientChannel() != 0 && remotePeer->getServerToClientChannel() != stunMessage.mTurnChannelNumber)
   {
      // Mismatched channel number
      std::cout << "TurnAsyncSocket::handleStunMessage: Channel number received in DataInd (" << (int)stunMessage.mTurnChannelNumber << ") does not match existing number for RemotePeer (" << (int)remotePeer->getServerToClientChannel() << ")." << std::endl;
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
      StunMessage channelConfirmationInd;
      channelConfirmationInd.createHeader(StunMessage::StunClassIndication, StunMessage::TurnChannelConfirmationMethod);
      channelConfirmationInd.mHasTurnPeerAddress = true;
      channelConfirmationInd.mTurnPeerAddress = stunMessage.mTurnPeerAddress;
      channelConfirmationInd.mHasTurnChannelNumber = true;
      channelConfirmationInd.mTurnChannelNumber = stunMessage.mTurnChannelNumber;

      // send channelConfirmationInd to local client
      sendStunMessage(channelConfirmationInd);
   }

   if(stunMessage.mHasTurnData)
   {
      mTurnAsyncSocketHandler->onReceiveSuccess(getSocketDescriptor(), 
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
      std::cout << "TurnAsyncSocket::handleStunMessage: DataInd missing attributes." << std::endl;
      return asio::error_code(reTurn::MissingAttributes, asio::error::misc_category);
   }

   StunTuple remoteTuple;
   remoteTuple.setTransportType(mRelayTransportType);
   remoteTuple.setPort(stunMessage.mTurnPeerAddress.port);
   if(stunMessage.mTurnPeerAddress.family == StunMessage::IPv6Family)
   {
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), &stunMessage.mTurnPeerAddress.addr.ipv6, bytes.size());
      remoteTuple.setAddress(asio::ip::address_v6(bytes));
   }
   else
   {
      remoteTuple.setAddress(asio::ip::address_v4(stunMessage.mTurnPeerAddress.addr.ipv4));
   }

   RemotePeer* remotePeer = mChannelManager.findRemotePeerByClientToServerChannel(stunMessage.mTurnChannelNumber);
   if(!remotePeer)
   {
      // Remote Peer not found - discard
      std::cout << "TurnAsyncSocket::handleStunMessage: Received ChannelConfirmationInd for unknown channel (" << stunMessage.mTurnChannelNumber << ") - discarding" << std::endl;
      return asio::error_code(reTurn::InvalidChannelNumberReceived, asio::error::misc_category);
   }

   if(remotePeer->getPeerTuple() != remoteTuple)
   {
      // Mismatched remote address
      std::cout << "TurnAsyncSocket::handleStunMessage: RemoteAddress associated with channel (" << remotePeer->getPeerTuple() << ") does not match ChannelConfirmationInd (" << remoteTuple << ")." << std::endl;
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
         std::cout << "TurnAsyncSocket::handleSharedSecretResponse: Stun response message for SharedSecretRequest is missing username and/or password!" << std::endl;
         mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }

      mTurnAsyncSocketHandler->onSharedSecretSuccess(getSocketDescriptor(), stunMessage.mUsername->c_str(), stunMessage.mUsername->size(), 
                                                                            stunMessage.mPassword->c_str(), stunMessage.mPassword->size());
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         mTurnAsyncSocketHandler->onSharedSecretFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

asio::error_code
TurnAsyncSocket::handleBindRequest(StunMessage& stunMessage)
{
   // Note: handling of BindRequest is not fully backwards compatible with RFC3489 - it is inline with bis13
   StunMessage response;

   // form the outgoing message
   response.mClass = StunMessage::StunClassSuccessResponse;
   response.mMethod = StunMessage::BindMethod;

   // Copy over TransactionId
   response.mHeader.magicCookieAndTid = stunMessage.mHeader.magicCookieAndTid;

   // Add XOrMappedAddress to response 
   response.mHasXorMappedAddress = true;
   response.mXorMappedAddress.port = stunMessage.mRemoteTuple.getPort();
   if(stunMessage.mRemoteTuple.getAddress().is_v6())
   {
      response.mXorMappedAddress.family = StunMessage::IPv6Family;  
      memcpy(&response.mXorMappedAddress.addr.ipv6, stunMessage.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mXorMappedAddress.addr.ipv6));
   }
   else
   {
      response.mXorMappedAddress.family = StunMessage::IPv4Family;  
      response.mXorMappedAddress.addr.ipv4 = stunMessage.mRemoteTuple.getAddress().to_v4().to_ulong();   
   }

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
      if(stunMessage.mHasXorMappedAddress)
      {
         reflexiveTuple.setTransportType(mLocalBinding.getTransportType());
         reflexiveTuple.setPort(stunMessage.mXorMappedAddress.port);
         if(stunMessage.mXorMappedAddress.family == StunMessage::IPv6Family)
         {
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &stunMessage.mXorMappedAddress.addr.ipv6, bytes.size());
            reflexiveTuple.setAddress(asio::ip::address_v6(bytes));
         }
         else
         {
            reflexiveTuple.setAddress(asio::ip::address_v4(stunMessage.mXorMappedAddress.addr.ipv4));
         }            
      }
      else if(stunMessage.mHasMappedAddress)  // Only look at MappedAddress if XorMappedAddress is not found - for backwards compatibility
      {
         reflexiveTuple.setTransportType(mLocalBinding.getTransportType());
         reflexiveTuple.setPort(stunMessage.mMappedAddress.port);
         if(stunMessage.mMappedAddress.family == StunMessage::IPv6Family)
         {
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &stunMessage.mMappedAddress.addr.ipv6, bytes.size());
            reflexiveTuple.setAddress(asio::ip::address_v6(bytes));
         }
         else
         {
            reflexiveTuple.setAddress(asio::ip::address_v4(stunMessage.mMappedAddress.addr.ipv4));
         }            
      }
      else
      {
         mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
      mTurnAsyncSocketHandler->onBindSuccess(getSocketDescriptor(), reflexiveTuple);
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         mTurnAsyncSocketHandler->onBindFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
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
      // Transport Type is requested type or socket type
      if(mRequestedTransportType != StunTuple::None)
      {
         mRelayTransportType = mRequestedTransportType == StunMessage::RequestedTransportUdp ? StunTuple::UDP : StunTuple::TCP;
      }
      else
      {
         mRelayTransportType = mLocalBinding.getTransportType();
      }

      StunTuple reflexiveTuple;
      StunTuple relayTuple;
      if(stunMessage.mHasXorMappedAddress)
      {
         reflexiveTuple.setTransportType(mLocalBinding.getTransportType());
         reflexiveTuple.setPort(stunMessage.mXorMappedAddress.port);
         if(stunMessage.mXorMappedAddress.family == StunMessage::IPv6Family)
         {
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &stunMessage.mXorMappedAddress.addr.ipv6, bytes.size());
            reflexiveTuple.setAddress(asio::ip::address_v6(bytes));
         }
         else
         {
            reflexiveTuple.setAddress(asio::ip::address_v4(stunMessage.mXorMappedAddress.addr.ipv4));
         }            
      }
      if(stunMessage.mHasTurnRelayAddress)
      {
         relayTuple.setTransportType(mRelayTransportType);
         relayTuple.setPort(stunMessage.mTurnRelayAddress.port);
         if(stunMessage.mTurnRelayAddress.family == StunMessage::IPv6Family)
         {
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &stunMessage.mTurnRelayAddress.addr.ipv6, bytes.size());
            relayTuple.setAddress(asio::ip::address_v6(bytes));
         }
         else
         {
            relayTuple.setAddress(asio::ip::address_v4(stunMessage.mTurnRelayAddress.addr.ipv4));
         } 
      }
      if(stunMessage.mHasTurnLifetime)
      {
         mLifetime = stunMessage.mTurnLifetime;
      }
      else
      {
         mLifetime = 0;
      }
      if(stunMessage.mHasTurnBandwidth)
      {
         mBandwidth = stunMessage.mTurnBandwidth;
      }
      else
      {
         mBandwidth = 0;
      }

      // All was well - return 0 errorCode
      if(mLifetime != 0)
      {
         mHaveAllocation = true;
         mAllocationRefreshTime = time(0) + ((mLifetime*5)/8);  // Allocation refresh should sent before 3/4 lifetime - use 5/8 lifetime
         mTurnAsyncSocketHandler->onAllocationSuccess(getSocketDescriptor(), reflexiveTuple, relayTuple, mLifetime, mBandwidth);
      }
      else
      {
         mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
      }
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         mTurnAsyncSocketHandler->onAllocationFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
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
         mAllocationRefreshTime = time(0) + ((mLifetime*5)/8);  // Allocation refresh should sent before 3/4 lifetime - use 5/8 lifetime
         mTurnAsyncSocketHandler->onRefreshSuccess(getSocketDescriptor(), mLifetime);
      }
      else
      {
         mHaveAllocation = false;
         mTurnAsyncSocketHandler->onRefreshSuccess(getSocketDescriptor(), 0);
      }
   }
   else
   {
      // Check if success or not
      if(stunMessage.mHasErrorCode)
      {
         mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(stunMessage.mErrorCode.errorClass * 100 + stunMessage.mErrorCode.number, asio::error::misc_category));
      }
      else
      {
         mTurnAsyncSocketHandler->onRefreshFailure(getSocketDescriptor(), asio::error_code(MissingAttributes, asio::error::misc_category));
         return asio::error_code(MissingAttributes, asio::error::misc_category);
      }
   }
   return asio::error_code();
}

void
TurnAsyncSocket::send(const char* buffer, unsigned int size)
{
   // Allow raw data to be sent if there is no allocation
   if(!mHaveAllocation)
   {
      SharedPtr<Data> data(new Data(buffer, size));
      return send(data);
   }

   if(!mActiveDestination)
   {
      mTurnAsyncSocketHandler->onSendFailure(getSocketDescriptor(), asio::error_code(reTurn::NoActiveDestination, asio::error::misc_category));
      return;
   }

   return sendTo(*mActiveDestination, buffer, size);
}

void 
TurnAsyncSocket::sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   resip::Lock lock(mMutex);
   // ensure there is an allocation 
   if(!mHaveAllocation)
   {
      mTurnAsyncSocketHandler->onSendFailure(getSocketDescriptor(), asio::error_code(reTurn::NoAllocation, asio::error::misc_category));
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
   return sendTo(*remotePeer, buffer, size);
}

void
TurnAsyncSocket::sendTo(RemotePeer& remotePeer, const char* buffer, unsigned int size)
{
   resip::Lock lock(mMutex);

   if(remotePeer.isClientToServerChannelConfirmed())
   {
      // send framed data to active destination
      SharedPtr<Data> data(new Data(buffer, size));
      send(remotePeer.getClientToServerChannel(), data);
   }
   else
   {
      // Data must be wrapped in a Send Indication
      // Wrap data in a SendInd
      StunMessage ind;
      ind.createHeader(StunMessage::StunClassIndication, StunMessage::TurnSendMethod);
      ind.mHasTurnPeerAddress = true;
      ind.mTurnPeerAddress.port = remotePeer.getPeerTuple().getPort();
      if(remotePeer.getPeerTuple().getAddress().is_v6())
      {
         ind.mTurnPeerAddress.family = StunMessage::IPv6Family;
         memcpy(&ind.mTurnPeerAddress.addr.ipv6, remotePeer.getPeerTuple().getAddress().to_v6().to_bytes().c_array(), sizeof(ind.mTurnPeerAddress.addr.ipv6));
      }
      else
      {
         ind.mTurnPeerAddress.family = StunMessage::IPv4Family;
         ind.mTurnPeerAddress.addr.ipv4 = remotePeer.getPeerTuple().getAddress().to_v4().to_ulong();
      }
      ind.mHasTurnChannelNumber = true;
      ind.mTurnChannelNumber = remotePeer.getClientToServerChannel();
      if(size > 0)
      {
         ind.setTurnData(buffer, size);
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
TurnAsyncSocket::connect(const std::string& address, unsigned short port)
{
   mAsyncSocketBase.connect(address,port);
}

void
TurnAsyncSocket::close()
{
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


