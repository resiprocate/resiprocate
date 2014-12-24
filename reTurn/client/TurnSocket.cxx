#include "TurnSocket.hxx"
#include "ErrorCode.hxx"
#include <rutil/Lock.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "../ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

#define UDP_RT0 100  // RTO - Estimate of Roundtrip time - 100ms is recommened for fixed line transport - the initial value should be configurable
                     // Should also be calculation this on the fly
#define UDP_MAX_RETRANSMITS    7       // Defined by RFC5389 (Rc) - should be configurable
#define TCP_RESPONSE_TIME      39500   // Defined by RFC5389 (Ti) - should be configurable
#define UDP_Rm                 16      // Defined by RFC5389 - should be configurable
#define UDP_FINAL_REQUEST_TIME (UDP_RT0 * UDP_Rm)  // Defined by RFC5389

//#define TURN_CHANNEL_BINDING_REFRESH_SECONDS 20   // TESTING only
#define TURN_CHANNEL_BINDING_REFRESH_SECONDS 240   // 4 minuntes - this is one minute before the permission will expire, Note:  ChannelBinding refreshes also refresh permissions

#define SOFTWARE_STRING "reTURN Sync Client 0.3 - RFC5389/turn-12"

namespace reTurn {

// Initialize static members
unsigned int TurnSocket::UnspecifiedLifetime = 0xFFFFFFFF;
unsigned int TurnSocket::UnspecifiedBandwidth = 0xFFFFFFFF; 
unsigned short TurnSocket::UnspecifiedToken = 0;
asio::ip::address TurnSocket::UnspecifiedIpAddress = asio::ip::address::from_string("0.0.0.0");

TurnSocket::TurnSocket(const asio::ip::address& address, unsigned short port) : 
   mLocalBinding(StunTuple::None /* Set properly by sub class */, address, port),
   mHaveAllocation(false),
   mActiveDestination(0),
   mReadTimer(mIOService),
   mConnected(false)
{
}

TurnSocket::~TurnSocket()
{
}

asio::error_code 
TurnSocket::requestSharedSecret(char* username, unsigned int usernameSize, 
                                char* password, unsigned int passwordSize)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Should we check here if TLS and deny?

   // Ensure Connected
   if(!mConnected)
   {
      return asio::error_code(reTurn::NotConnected, asio::error::misc_category); 
   }

   // Form Shared Secret request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::SharedSecretMethod);

   // Get Response
   StunMessage* response = sendRequestAndGetResponse(request, errorCode, false);
   if(response == 0)
   {
      return errorCode;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::error::misc_category);
      delete response;
      return errorCode;
   }

   // Copy username and password to callers buffer - checking sizes first
   if(!response->mHasUsername || !response->mHasPassword)
   {
      WarningLog(<< "Stun response message for SharedSecretRequest is missing username and/or password!");
      errorCode = asio::error_code(reTurn::MissingAuthenticationAttributes, asio::error::misc_category);  
      delete response;
      return errorCode;
   }

   if(response->mUsername->size() > usernameSize || response->mPassword->size() > passwordSize)
   {
      WarningLog( << "Stun response message for SharedSecretRequest contains data that is too large to return!");
      errorCode = asio::error_code(reTurn::BufferTooSmall, asio::error::misc_category);   
      delete response;
      return errorCode;
   }

   // Copy username and password to passed in buffers
   memcpy(username, response->mUsername->c_str(), response->mUsername->size()+1);
   memcpy(password, response->mPassword->c_str(), response->mPassword->size()+1);

   // All was well - return 0 errorCode
   delete response;
   return errorCode;
}

void 
TurnSocket::setUsernameAndPassword(const char* username, const char* password, bool shortTermAuth)
{
   mUsername = username;
   mPassword = password;
   if(shortTermAuth)
   {
      // If we are using short term auth, then use short term password as HMAC key
      mHmacKey = password;
   }
}

asio::error_code 
TurnSocket::bindRequest()
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Ensure Connected
   if(!mConnected)
   {
      return asio::error_code(reTurn::NotConnected, asio::error::misc_category); 
   }

   // Form Stun Bind request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::BindMethod);

   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   mReflexiveTuple.setTransportType(mLocalBinding.getTransportType());
   if(response->mHasXorMappedAddress)
   {
      StunMessage::setTupleFromStunAtrAddress(mReflexiveTuple, response->mXorMappedAddress);
   }
   else if(response->mHasMappedAddress)  // Only look at MappedAddress if XorMappedAddress is not found - for backwards compatibility
   {
      StunMessage::setTupleFromStunAtrAddress(mReflexiveTuple, response->mMappedAddress);
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::error::misc_category);
   }

   delete response;
   return errorCode;
}

asio::error_code 
TurnSocket::createAllocation(unsigned int lifetime,
                             unsigned int bandwidth,
                             unsigned char requestedProps, 
                             UInt64 reservationToken,
                             StunTuple::TransportType requestedTransportType)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Store Allocation Properties
   mRequestedLifetime = lifetime;
   mRequestedBandwidth = bandwidth;
   mRequestedProps = requestedProps;
   mReservationToken = reservationToken;
   mRequestedTransportType = requestedTransportType;

   // Ensure Connected
   if(!mConnected)
   {
      return asio::error_code(reTurn::NotConnected, asio::error::misc_category); 
   }

   if(mHaveAllocation)
   {
      return asio::error_code(reTurn::AlreadyAllocated, asio::error::misc_category); 
   }

   // Form Turn Allocate request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnAllocateMethod);
   if(mRequestedLifetime != UnspecifiedLifetime)
   {
      request.mHasTurnLifetime = true;
      request.mTurnLifetime = mRequestedLifetime;
   }

   if(mRequestedBandwidth != UnspecifiedBandwidth)
   {
      request.mHasTurnBandwidth = true;
      request.mTurnBandwidth = mRequestedBandwidth;
   }

   if(mRequestedTransportType == StunTuple::None)
   {
      mRequestedTransportType = mLocalBinding.getTransportType();
   }
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
      return asio::error_code(reTurn::InvalidRequestedTransport, asio::error::misc_category); 
   }

   if(mRequestedProps != StunMessage::PropsNone)
   {
      request.mHasTurnEvenPort = true;
      request.mTurnEvenPort.propType = mRequestedProps;
   }
   else if(mReservationToken != 0)
   {
      request.mHasTurnReservationToken = true;
      request.mTurnReservationToken = mReservationToken;
   }

   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   if(response->mHasXorMappedAddress)
   {
      mReflexiveTuple.setTransportType(mLocalBinding.getTransportType());
      StunMessage::setTupleFromStunAtrAddress(mReflexiveTuple, response->mXorMappedAddress);
   }
   if(response->mHasTurnXorRelayedAddress)
   {
      // Transport Type is requested type or socket type
      if(request.mHasTurnRequestedTransport)
      {
         mRelayTuple.setTransportType(request.mTurnRequestedTransport == StunMessage::RequestedTransportUdp ? StunTuple::UDP : StunTuple::TCP);
      }
      else
      {
         mRelayTuple.setTransportType(mLocalBinding.getTransportType());  
      }
      StunMessage::setTupleFromStunAtrAddress(mRelayTuple, response->mTurnXorRelayedAddress);
   }
   if(response->mHasTurnLifetime)
   {
      mLifetime = response->mTurnLifetime;
   }
   if(response->mHasTurnBandwidth)
   {
      mBandwidth = response->mTurnBandwidth;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::error::misc_category);
      delete response;
      return errorCode;
   }

   // All was well - return 0 errorCode
   if(mLifetime != 0)
   {
      mHaveAllocation = true;
      mAllocationRefreshTime = time(0) + ((mLifetime*5)/8);  // Allocation refresh should sent before 3/4 lifetime - use 5/8 lifetime
   }
   delete response;
   return errorCode;
}

asio::error_code 
TurnSocket::refreshAllocation()
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // Form Turn Allocate request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnRefreshMethod);
   if(mRequestedLifetime != UnspecifiedLifetime)
   {
      request.mHasTurnLifetime = true;
      request.mTurnLifetime = mRequestedLifetime;
   }
   if(mRequestedBandwidth != UnspecifiedBandwidth)
   {
      request.mHasTurnBandwidth = true;
      request.mTurnBandwidth = mRequestedBandwidth;
   }

   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      if(mRequestedLifetime != 0 ||
         (response->mErrorCode.errorClass == 4 && response->mErrorCode.number == 37))  // if we receive a 437 response to a refresh, we should ensure the allocation is destroyed
      {
         mHaveAllocation = false;
      }

      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::error::misc_category);
      delete response;
      return errorCode;
   }

   // All was well - return 0 errorCode
   if(mLifetime != 0)
   {
      mHaveAllocation = true;
      mAllocationRefreshTime = time(0) + ((mLifetime*5)/8);  // Allocation refresh should sent before 3/4 lifetime - use 5/8 lifetime
   }
   else
   {
      mHaveAllocation = false;
   }
   delete response;
   return errorCode;
}

asio::error_code 
TurnSocket::destroyAllocation()
{
   resip::Lock lock(mMutex);
   if(mHaveAllocation)
   {
      mRequestedLifetime = 0;
      mRequestedBandwidth = UnspecifiedBandwidth;
      mRequestedProps = StunMessage::PropsNone;
      mReservationToken = UnspecifiedToken;
      mRequestedTransportType = StunTuple::None;

      return refreshAllocation();
   }
   else
   {
      return asio::error_code(reTurn::NoAllocation, asio::error::misc_category); 
   }
}

StunTuple& 
TurnSocket::getRelayTuple()
{
   return mRelayTuple;
}

StunTuple& 
TurnSocket::getReflexiveTuple()
{
   return mReflexiveTuple;
}

unsigned int 
TurnSocket::getLifetime()
{
   return mLifetime;
}

unsigned int 
TurnSocket::getBandwidth()
{
   return mBandwidth;
}

asio::error_code 
TurnSocket::setActiveDestination(const asio::ip::address& address, unsigned short port)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      if(mConnected)
      {
         // TODO - Disconnect
      }
      return connect(address.to_string(), port);
      //return asio::error_code(reTurn::NoAllocation, asio::error::misc_category); 
   }

   // Ensure Connected
   if(!mConnected)
   {
      return asio::error_code(reTurn::NotConnected, asio::error::misc_category); 
   }

   // Setup Remote Peer 
   StunTuple remoteTuple(mRelayTuple.getTransportType(), address, port);
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(remotePeer)
   {
      mActiveDestination = remotePeer;
   }
   else
   {
      // No remote peer yet (ie. not data sent or received from remote peer) - so create one
      mActiveDestination = mChannelManager.createChannelBinding(remoteTuple);
      resip_assert(mActiveDestination);
      errorCode = channelBind(*mActiveDestination);
   }

   return errorCode;
}

asio::error_code
TurnSocket::channelBind(RemotePeer& remotePeer)
{
   asio::error_code ret;

   // Form Channel Bind request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnChannelBindMethod);

   // Set headers
   request.mHasTurnChannelNumber = true;
   request.mTurnChannelNumber = remotePeer.getChannel();
   request.mCntTurnXorPeerAddress = 1;
   StunMessage::setStunAtrAddressFromTuple(request.mTurnXorPeerAddress[0], remotePeer.getPeerTuple());

   StunMessage* response = sendRequestAndGetResponse(request, ret);
   if(response == 0)
   {
      return ret;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      ret = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::error::misc_category);
      delete response;
      return ret;
   }

   remotePeer.refresh();
   remotePeer.setChannelConfirmed();
   mChannelBindingRefreshTimes[remotePeer.getChannel()] = time(0) + TURN_CHANNEL_BINDING_REFRESH_SECONDS;

   return ret;
}

asio::error_code 
TurnSocket::clearActiveDestination()
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

asio::error_code 
TurnSocket::send(const char* buffer, unsigned int size)
{
   // Allow raw data to be sent if there is no allocation
   if(!mHaveAllocation && mConnected)
   {
      return rawWrite(buffer, size);
   }

   if(!mActiveDestination)
   {
      return asio::error_code(reTurn::NoActiveDestination, asio::error::misc_category); 
   }

   return sendTo(*mActiveDestination, buffer, size);
}

asio::error_code 
TurnSocket::sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   resip::Lock lock(mMutex);
   // ensure there is an allocation 
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::error::misc_category); 
   }

   // Setup Remote Peer 
   StunTuple remoteTuple(mRelayTuple.getTransportType(), address, port);
   RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
   if(!remotePeer)
   {
      // No remote peer - then just create a temp one for sending
      // For blocking TurnSocket we do not send ChannelBind unless setActiveDestination is used
      // to avoid blocking on the sendTo call
      RemotePeer remotePeer(remoteTuple, 0 /* channel - not important */, 0 /* lifetime - not important */);
      return sendTo(remotePeer, buffer, size);
   }
   else
   {
      return sendTo(*remotePeer, buffer, size);
   }
}

asio::error_code 
TurnSocket::sendTo(RemotePeer& remotePeer, const char* buffer, unsigned int size)
{
   resip::Lock lock(mMutex);

   // Check to see if an allocation refresh is required - if so send it and make sure it was sucessful
   asio::error_code ret = checkIfAllocationRefreshRequired();
   if(ret)
   {
      return ret;
   }

   // Check to see if a channel binding refresh is required - if so send it and make sure it was successful
   ret = checkIfChannelBindingRefreshRequired();
   if(ret)
   {
      return ret;
   }

   if(remotePeer.isChannelConfirmed())
   {
      // send framed data to active destination
      char framing[4];
      unsigned short channelNumber = remotePeer.getChannel();
      channelNumber = htons(channelNumber);
      memcpy(&framing[0], &channelNumber, 2);
      if(mLocalBinding.getTransportType() == StunTuple::UDP)
      {
         // No size in header for UDP
         framing[2] = 0x00;
         framing[3] = 0x00;
      }
      else
      {
         UInt16 turnDataSize = size;
         turnDataSize = htons(turnDataSize);
         memcpy((void*)&framing[2], &turnDataSize, 2);
      }
      std::vector<asio::const_buffer> bufs;
      bufs.push_back(asio::buffer(framing, sizeof(framing)));
      bufs.push_back(asio::buffer(buffer, size));

      return rawWrite(bufs);
   }
   else
   {
      // Data must be wrapped in a Send Indication
      // Wrap data in a SendInd
      StunMessage ind;
      ind.createHeader(StunMessage::StunClassIndication, StunMessage::TurnSendMethod);
      ind.mCntTurnXorPeerAddress = 1;
      ind.mTurnXorPeerAddress[0].port = remotePeer.getPeerTuple().getPort();
      if(remotePeer.getPeerTuple().getAddress().is_v6())
      {
         ind.mTurnXorPeerAddress[0].family = StunMessage::IPv6Family;
         memcpy(&ind.mTurnXorPeerAddress[0].addr.ipv6, remotePeer.getPeerTuple().getAddress().to_v6().to_bytes().data(), sizeof(ind.mTurnXorPeerAddress[0].addr.ipv6));
      }
      else
      {
         ind.mTurnXorPeerAddress[0].family = StunMessage::IPv4Family;
         ind.mTurnXorPeerAddress[0].addr.ipv4 = remotePeer.getPeerTuple().getAddress().to_v4().to_ulong();
      }
      if(size > 0)
      {
         ind.setTurnData(buffer, size);
      }

      // Send indication to Turn Server
      unsigned int msgsize = ind.stunEncodeMessage(mWriteBuffer, sizeof(mWriteBuffer));
      return rawWrite(mWriteBuffer, msgsize);
   }
}

asio::error_code 
TurnSocket::receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;
   resip::Lock lock(mMutex);
   bool done = false;

   // TODO - rethink this scheme so that we don't need to copy recieved data
   // TODO - if we loop around more than once - timeout needs to be adjusted

   while(!done)
   {
      done = true;

      // Wait for response
      unsigned int readSize;
      errorCode = rawRead(timeout, &readSize, sourceAddress, sourcePort); // Note: SourceAddress and sourcePort may be overwritten below if from Turn Relay
      if(errorCode)
      {
         return errorCode;
      }

      // Note if this is a UDP RFC3489 back compat socket, then allocations are not allowed and handleRawData will always be used
      if(!mHaveAllocation)
      {
         return handleRawData(mReadBuffer, readSize, readSize, buffer, size);
      }

      if(readSize > 4)
      {
         // Stun Message has first two bits as 00 
         if((mReadBuffer[0] & 0xC0) == 0)
         {
            // StunMessage
            StunMessage* stunMsg = new StunMessage(mLocalBinding, mConnectedTuple, &mReadBuffer[0], readSize);
            unsigned int tempsize = size;
            errorCode = handleStunMessage(*stunMsg, buffer, tempsize, sourceAddress, sourcePort);
            if(!errorCode && tempsize == 0)  // Signifies that a Stun/Turn request was received and there is nothing to return to receive caller
            {
               done = false;
            }
            else
            {
               size = tempsize;
            }
         }
         else // Channel Data Message
         {
            unsigned short channelNumber;
            memcpy(&channelNumber, &mReadBuffer[0], 2);
            channelNumber = ntohs(channelNumber);

            RemotePeer* remotePeer = mChannelManager.findRemotePeerByChannel(channelNumber);
            if(remotePeer)
            {
               UInt16 dataLen;
               memcpy(&dataLen, &mReadBuffer[2], 2);
               dataLen = ntohs(dataLen);
   
               if(sourceAddress)
               {
                  *sourceAddress = remotePeer->getPeerTuple().getAddress();
               }
               if(sourcePort)
               {
                  *sourcePort = remotePeer->getPeerTuple().getPort();
               }
               errorCode = handleRawData(&mReadBuffer[4], readSize-4, dataLen, buffer, size);
            }
            else
            {
               // Invalid Channel - teardown?
               errorCode = asio::error_code(reTurn::InvalidChannelNumberReceived, asio::error::misc_category);  
               done = true;
            }
         }
      }
      else  // size <= 4
      {
         // Less data than frame size received
         errorCode = asio::error_code(reTurn::FrameError, asio::error::misc_category);
         done = true;
      }
   }
   return errorCode;
}

asio::error_code 
TurnSocket::receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout)
{
   asio::ip::address sourceAddress;
   unsigned short sourcePort;
   bool done = false;
   asio::error_code errorCode;
   resip::Lock lock(mMutex);

   while(!done)
   {
      done = true;
      errorCode = receive(buffer, size, timeout, &sourceAddress, &sourcePort);
      if(!errorCode)
      {
         if(sourceAddress != address || sourcePort != port)
         {
            WarningLog(<< "Recevied message but not from requested address/port - Discarding.");
            done = false;
         }
      }
   }
   return errorCode;
}

asio::error_code 
TurnSocket::handleRawData(char* data, unsigned int dataSize, unsigned int expectedSize, char* buffer, unsigned int& bufferSize)
{
   asio::error_code errorCode;

   if(dataSize != expectedSize)
   {
      // TODO - fix read logic so that we can read in chuncks
      WarningLog(<< "Did not read entire message: read=" << dataSize << " wanted=" << expectedSize);
      return asio::error_code(reTurn::ReadError, asio::error::misc_category); 
   }

   if(dataSize > bufferSize) 
   {
     // Passed in buffer is not large enough
     WarningLog(<< "Passed in buffer not large enough.");
     return asio::error_code(reTurn::BufferTooSmall, asio::error::misc_category); 
   }

   // Copy data to return buffer
   memcpy(buffer, data, dataSize);
   bufferSize = dataSize;

   return errorCode;
}

asio::error_code 
TurnSocket::handleStunMessage(StunMessage& stunMessage, char* buffer, unsigned int& size, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;
   if(stunMessage.isValid())
   {
      if(stunMessage.mClass == StunMessage::StunClassIndication && stunMessage.mMethod == StunMessage::TurnDataMethod)
      {
         if(stunMessage.mUnknownRequiredAttributes.numAttributes > 0)
         {
            // Unknown Comprehension-Required Attributes found
            WarningLog(<< "DataInd with unknown comprehension required attributes.");
            return asio::error_code(reTurn::UnknownRequiredAttributes, asio::error::misc_category);
         }

         if(stunMessage.mCntTurnXorPeerAddress == 0 || !stunMessage.mHasTurnData)
         {
            // Missing RemoteAddress or TurnData attribute
            WarningLog(<< "DataInd missing attributes.");
            return asio::error_code(reTurn::MissingAttributes, asio::error::misc_category);
         }

         StunTuple remoteTuple;
         remoteTuple.setTransportType(mRelayTuple.getTransportType());
         StunMessage::setTupleFromStunAtrAddress(remoteTuple, stunMessage.mTurnXorPeerAddress[0]);

         RemotePeer* remotePeer = mChannelManager.findRemotePeerByPeerAddress(remoteTuple);
         if(!remotePeer)
         {
            // Remote Peer not found - discard data
            WarningLog(<< "Data received from unknown RemotePeer - discarding");
            return asio::error_code(reTurn::UnknownRemoteAddress, asio::error::misc_category);
         }

         if(stunMessage.mTurnData->size() > size)
         {
            // Passed in buffer is not large enough
            WarningLog(<< "Passed in buffer not large enough.");
            return asio::error_code(reTurn::BufferTooSmall, asio::error::misc_category);
         }

         memcpy(buffer, stunMessage.mTurnData->data(), stunMessage.mTurnData->size());
         size = (unsigned int)stunMessage.mTurnData->size();

         if(sourceAddress != 0)
         {
            *sourceAddress = remoteTuple.getAddress();
         }
         if(sourcePort != 0)
         {
            *sourcePort = remoteTuple.getPort();
         }
      }
      else if(stunMessage.mClass == StunMessage::StunClassRequest && stunMessage.mMethod == StunMessage::BindMethod)
      {
         // Note: handling of BindRequest is not fully backwards compatible with RFC3489 - it is inline with RFC5389
         StunMessage response;

         // form the outgoing message
         response.mMethod = StunMessage::BindMethod;

         // Copy over TransactionId
         response.mHeader.magicCookieAndTid = stunMessage.mHeader.magicCookieAndTid;

         if(stunMessage.mUnknownRequiredAttributes.numAttributes > 0)
         {
            // Unknown Comprehension-Required Attributes found
            WarningLog(<< "BindRequest with unknown comprehension required attributes.");

            response.mClass = StunMessage::StunClassErrorResponse;

            // Add unknown attributes
            response.mHasUnknownAttributes = true;
            response.mUnknownAttributes = stunMessage.mUnknownRequiredAttributes;
         }
         else
         {
            response.mClass = StunMessage::StunClassSuccessResponse;

            // Add XOrMappedAddress to response 
            response.mHasXorMappedAddress = true;
            StunMessage::setStunAtrAddressFromTuple(response.mXorMappedAddress, stunMessage.mRemoteTuple);
         }

         // Add Software Attribute
         response.setSoftware(SOFTWARE_STRING);

         // send bind response to local client
         unsigned int bufferSize = 512;  // enough room for Stun Header + XorMapped Address (v6) or Unknown Attributes + Software Attribute;
         resip::Data buffer(bufferSize, resip::Data::Preallocate);
         unsigned int writeSize = response.stunEncodeMessage((char*)buffer.data(), bufferSize);

         errorCode = rawWrite(buffer.data(), writeSize);
         size = 0; // go back to receiving
      }
      else if(stunMessage.mClass == StunMessage::StunClassIndication && stunMessage.mMethod == StunMessage::BindMethod)
      {
         // Nothing to do
         size = 0;
      }
      else if(stunMessage.mClass == StunMessage::StunClassSuccessResponse || stunMessage.mClass == StunMessage::StunClassErrorResponse)
      {
         // Received a stray or response retranmission - ignore
         size = 0;
      }
      else
      {
         // TODO - handle others ????
      }
   }
   else
   {
      WarningLog(<< "Read Invalid StunMsg.");
      return asio::error_code(reTurn::ErrorParsingMessage, asio::error::misc_category);
   }
   return errorCode;
}

void 
TurnSocket::startReadTimer(unsigned int timeout)
{
   if(timeout != 0)
   {
      mReadTimer.expires_from_now(boost::posix_time::milliseconds(timeout));
      mReadTimer.async_wait(boost::bind(&TurnSocket::handleRawReadTimeout, this, asio::placeholders::error));
   }
}

void 
TurnSocket::handleRawRead(const asio::error_code& errorCode, size_t bytesRead)
{
   mBytesRead = bytesRead;
   mReadErrorCode = errorCode;
   mReadTimer.cancel();
}

void 
TurnSocket::handleRawReadTimeout(const asio::error_code& errorCode)
{
   if(!errorCode)
   {
      cancelSocket();
   }
}

asio::error_code 
TurnSocket::checkIfAllocationRefreshRequired()
{
   if(mHaveAllocation && (time(0) >= mAllocationRefreshTime))
   {
      // Do allocation refresh
      return refreshAllocation();
   }
   return asio::error_code();  // 0
}

asio::error_code 
TurnSocket::checkIfChannelBindingRefreshRequired()
{
   asio::error_code ret; // 0
   if(mHaveAllocation)
   {
      time_t now = time(0);
      ChannelBindingRefreshTimeMap::iterator it = mChannelBindingRefreshTimes.begin();
      for(;it!=mChannelBindingRefreshTimes.end();it++)
      {
         if(it->second != 0 && now >= it->second)
         {
            it->second = 0;  // will be reset by channelBind if success

            // Do channel binding refresh
            RemotePeer* remotePeer = mChannelManager.findRemotePeerByChannel(it->first);
            if(remotePeer)
            {
               ret = channelBind(*remotePeer);
            }
         }
      }
   }
   return ret;
}

StunMessage* 
TurnSocket::sendRequestAndGetResponse(StunMessage& request, asio::error_code& errorCode, bool addAuthInfo)
{
   bool sendRequest = true;
   bool reliableTransport = mLocalBinding.getTransportType() != StunTuple::UDP;
   unsigned int timeout = reliableTransport ? TCP_RESPONSE_TIME : UDP_RT0;
   unsigned int totalTime = 0;
   unsigned int requestsSent = 0;
   unsigned int readsize = 0;

   // Add Software Attribute
   request.setSoftware(SOFTWARE_STRING);

   if(addAuthInfo && !mUsername.empty() && !mHmacKey.empty())
   {
      request.mHasMessageIntegrity = true;
      request.setUsername(mUsername.c_str()); 
      request.mHmacKey = mHmacKey;
      if(!mRealm.empty())
      {
         request.setRealm(mRealm.c_str());
      }
      if(!mNonce.empty())
      {
         request.setNonce(mNonce.c_str());
      }
   }

   unsigned int writesize = request.stunEncodeMessage(mWriteBuffer, sizeof(mWriteBuffer));

   while(true)
   {
      if(sendRequest)
      {
         // Send request to Turn Server
         if(requestsSent > 0)
         {
            DebugLog(<< "TurnSocket: retranmitting request...");
         }
         requestsSent++;
         errorCode = rawWrite(mWriteBuffer, writesize);
         if(errorCode)
         {
            return 0;
         }
         sendRequest = false;
      }

      // Wait for response
      errorCode = rawRead(timeout, &readsize);
      if(errorCode)
      {
         if(errorCode == asio::error::operation_aborted)
         {
            totalTime += timeout;
            if(reliableTransport || requestsSent == UDP_MAX_RETRANSMITS)
            {
               InfoLog(<< "Timed out waiting for Stun response!");
               errorCode = asio::error_code(reTurn::ResponseTimeout, asio::error::misc_category);
               return 0;
            }

            // timed out and should retransmit - calculate next timeout
            if(requestsSent == UDP_MAX_RETRANSMITS - 1)
            {
               timeout = UDP_FINAL_REQUEST_TIME;
            } 
            else
            {
               timeout = (timeout*2);
            }
            sendRequest = true;
            continue;
         }
         return 0;
      }

      if(readsize > 4)
      {
         // Stun Message has first two bits as 00 
         if((mReadBuffer[0] & 0xC0) == 0)
         {
            StunMessage* response = new StunMessage(mLocalBinding, mConnectedTuple, &mReadBuffer[0], readsize);

            // If response is valid and has no unknown comprehension-required attributes then we can process it
            if(response->isValid() && response->mUnknownRequiredAttributes.numAttributes == 0) 
            {
               if(!response->checkMessageIntegrity(request.mHmacKey))
               {
                  WarningLog(<< "Stun response message integrity is bad!");
                  delete response;
                  errorCode = asio::error_code(reTurn::BadMessageIntegrity, asio::error::misc_category);
                  return 0;
               }
   
               // Check that TID matches request
               if(!(response->mHeader.magicCookieAndTid == request.mHeader.magicCookieAndTid))
               {
                  InfoLog(<< "Stun response TID does not match request - discarding!");
                  delete response;
                  continue;  // read next message
               }
   
               // If response is a 401, then store Realm, nonce, and hmac key
               // If a realm and nonce attributes are present and the response is a 401 or 438 (Nonce Expired), 
               // then re-issue request with new auth attributes
               if(response->mHasRealm &&
                  response->mHasNonce &&
                  response->mHasErrorCode && 
                  response->mErrorCode.errorClass == 4 &&
                  ((response->mErrorCode.number == 1 && mHmacKey.empty()) ||  // Note if 401 error then ensure we haven't already tried once - if we've tried then mHmacKey will be populated
                    response->mErrorCode.number == 38))
               {
                  mNonce = *response->mNonce;
                  mRealm = *response->mRealm;
                  response->calculateHmacKey(mHmacKey, mUsername, mRealm, mPassword);

                  // Re-Issue reques (with new TID)
                  request.createHeader(request.mClass, request.mMethod);  // updates TID
                  delete response;
                  return sendRequestAndGetResponse(request, errorCode, true);
               }

               errorCode = asio::error_code(reTurn::Success, asio::error::misc_category);
               return response;
            }
            else
            {
               WarningLog(<< "Stun response message is invalid!");
               delete response;
               errorCode = asio::error_code(reTurn::ErrorParsingMessage, asio::error::misc_category);  
               return 0;
            }
         }
         else  // Channel Data Message
         {
            // TODO - handle buffering of Turn data that is receive while waiting for a Request/Response
            errorCode = asio::error_code(reTurn::FrameError, asio::error::misc_category);  
            return 0;
         }
      }
      else
      {
         // Less data than frame size received
         errorCode = asio::error_code(reTurn::FrameError, asio::error::misc_category);  
         return 0;
      }
   }
}

} // namespace


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
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
