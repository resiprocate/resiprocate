#include "TurnSocket.hxx"
#include "ErrorCode.hxx"

using namespace std;

namespace reTurn {

// Initialize static members
unsigned int TurnSocket::UnspecifiedLifetime = 0xFFFFFFFF;
unsigned int TurnSocket::UnspecifiedBandwidth = 0xFFFFFFFF; 
unsigned short TurnSocket::UnspecifiedPort = 0;
asio::ip::address TurnSocket::UnspecifiedIpAddress = asio::ip::address::from_string("0.0.0.0");

TurnSocket::TurnSocket(const asio::ip::address& address, unsigned short port) : 
   mLocalBinding(StunTuple::None, address, port),
   mHaveAllocation(false)
{
}

TurnSocket::~TurnSocket()
{
}

StunMessage* 
TurnSocket::sendRequestAndGetResponse(StunMessage& request, asio::error_code& errorCode)
{
   unsigned int size = request.stunEncodeMessage(mBuffer, sizeof(mBuffer));

   //cout << "writting " << size << " bytes..."  << endl;

   // Send request to Turn Server
   errorCode = rawWrite(mBuffer, size);
   if(errorCode != 0)
   {
      return 0;
   }

   // Wait for response
   errorCode = rawRead(mBuffer, sizeof(mBuffer), &size);
   if(errorCode != 0)
   {
      return 0;
   }

   StunMessage* response = new StunMessage(mLocalBinding, mTurnServer, mBuffer, size);

   if(response->isValid())
   {
      if(!response->checkMessageIntegrity(request.mHmacKey))
      {
         std::cout << "Stun response message integrity is bad!" << std::endl;
         delete response;
         errorCode = asio::error_code(reTurn::BadMessageIntegrity, asio::misc_ecat);
         return 0;
      }

      errorCode = asio::error_code(reTurn::Success, asio::misc_ecat);
      return response;
   }
   else
   {
      std::cout << "Stun response message is invalid!" << std::endl;
      delete response;
      errorCode = asio::error_code(reTurn::ErrorParsingMessage, asio::misc_ecat);  
      return 0;
   }
}

asio::error_code 
TurnSocket::requestSharedSecret(const asio::ip::address& turnServerAddress, 
                                unsigned short turnServerPort, 
                                char* username, unsigned int usernameSize, 
                                char* password, unsigned int passwordSize)
{
   asio::error_code errorCode;

   // Should we check here if TLS and deny?

   mTurnServer.setTransportType(mLocalBinding.getTransportType());
   mTurnServer.setAddress(turnServerAddress);
   mTurnServer.setPort(turnServerPort);

   // Connect socket to Turn Server
   errorCode = connect(turnServerAddress, turnServerPort);
   if(errorCode != 0)
   {
      return errorCode;
   }

   // Form Shared Secret request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::SharedSecretRequest);
   request.mHasFingerprint = true;

   // Get Response
   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::misc_ecat);
      delete response;
      return errorCode;
   }

   // Copy username and password to callers buffer - checking sizes first
   if(!response->mHasUsername || !response->mHasPassword)
   {
      std::cout << "Stun response message for SharedSecretRequest is missing username and/or password!" << std::endl;
      errorCode = asio::error_code(reTurn::MissingAuthenticationAttributes, asio::misc_ecat);  
      delete response;
      return errorCode;
   }

   if(response->mUsername->size() > usernameSize || response->mPassword->size() > passwordSize)
   {
      std::cout << "Stun response message for SharedSecretRequest contains data that is too large to return!" << std::endl;
      errorCode = asio::error_code(reTurn::BufferTooSmall, asio::misc_ecat);   
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

asio::error_code 
TurnSocket::createAllocation(const asio::ip::address& turnServerAddress, 
                             unsigned short turnServerPort, 
                             char* username,
                             char* password,
                             unsigned int lifetime,
                             unsigned int bandwidth,
                             unsigned short requestedPortProps, 
                             unsigned short requestedPort,
                             StunTuple::TransportType requestedTransportType, 
                             const asio::ip::address &requestedIpAddress)
{
   asio::error_code errorCode;

   mHaveAllocation = false;  // toggled to true if successful

   // Store Allocation Properties
   mTurnServer.setTransportType(mLocalBinding.getTransportType());
   mTurnServer.setAddress(turnServerAddress);
   mTurnServer.setPort(turnServerPort);
   mUsername = username;
   mPassword = password;
   mRequestedLifetime = lifetime;
   mRequestedBandwidth = bandwidth;
   mRequestedPortProps = requestedPortProps;
   mRequestedPort = requestedPort;
   mRequestedTransportType = requestedTransportType;
   mRequestedIpAddress = requestedIpAddress;

   // Connect socket to Turn Server
   errorCode = connect(turnServerAddress, turnServerPort);
   if(errorCode != 0)
   {
      return errorCode;
   }

   // Perform allocation
   return refreshAllocation();
}

asio::error_code 
TurnSocket::refreshAllocation()
{
   asio::error_code errorCode;

   // Form Turn Allocate request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnAllocateRequest);
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
   if(mRequestedTransportType != StunTuple::None)
   {
      // TODO - could do some validation here
      request.mHasTurnRequestedTransport = true;
      request.mTurnRequestedTransport = mRequestedTransportType;
   }
   if(mRequestedIpAddress != UnspecifiedIpAddress)
   {
      request.mHasTurnRequestedIp = true;
      request.mTurnRequestedIp.port = 0;  // Not relevant
      if(mRequestedIpAddress.is_v6())
      {
         request.mTurnRequestedIp.family = StunMessage::IPv6Family;  
         memcpy(&request.mTurnRequestedIp.addr.ipv6, mRequestedIpAddress.to_v6().to_bytes().c_array(), sizeof(request.mTurnRequestedIp.addr.ipv6));
      }
      else
      {
         request.mTurnRequestedIp.family = StunMessage::IPv4Family;  
         request.mTurnRequestedIp.addr.ipv4 = mRequestedIpAddress.to_v4().to_ulong();   
      }
   }
   if(mRequestedPortProps != StunMessage::PortPropsNone || mRequestedPort != UnspecifiedPort)
   {
      request.mHasTurnRequestedPortProps = true;
      request.mTurnRequestedPortProps.props = mRequestedPortProps;
      request.mTurnRequestedPortProps.port = mRequestedPort;
   }
   request.mHasFingerprint = true;
   request.mHasMessageIntegrity = true;

   request.setUsername(mUsername.data()); 
   request.mHmacKey = mPassword;

   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   if(response->mHasXorMappedAddress)
   {
      response->applyXorToAddress(response->mXorMappedAddress, response->mXorMappedAddress);
      mReflexiveTuple.setTransportType(mLocalBinding.getTransportType());
      mReflexiveTuple.setPort(response->mXorMappedAddress.port);
      if(response->mXorMappedAddress.family == StunMessage::IPv6Family)
      {
         asio::ip::address_v6::bytes_type bytes;
         memcpy(bytes.c_array(), &response->mXorMappedAddress.addr.ipv6, bytes.size());
         mReflexiveTuple.setAddress(asio::ip::address_v6(bytes));
      }
      else
      {
         mReflexiveTuple.setAddress(asio::ip::address_v4(response->mXorMappedAddress.addr.ipv4));
      }            
   }
   if(response->mHasTurnRelayAddress)
   {
      mRelayTuple.setTransportType(request.mHasTurnRequestedTransport ? (StunTuple::TransportType)request.mTurnRequestedTransport : mLocalBinding.getTransportType());  // Transport Type is requested type or socket type
      mRelayTuple.setPort(response->mTurnRelayAddress.port);
      if(response->mTurnRelayAddress.family == StunMessage::IPv6Family)
      {
         asio::ip::address_v6::bytes_type bytes;
         memcpy(bytes.c_array(), &response->mTurnRelayAddress.addr.ipv6, bytes.size());
         mRelayTuple.setAddress(asio::ip::address_v6(bytes));
      }
      else
      {
         mRelayTuple.setAddress(asio::ip::address_v4(response->mTurnRelayAddress.addr.ipv4));
      } 
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
      if(mRequestedLifetime != 0)
      {
         mHaveAllocation = false;
      }

      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::misc_ecat);
      delete response;
      return errorCode;
   }

   // All was well - return 0 errorCode
   if(mLifetime != 0)
   {
      mHaveAllocation = true;
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
   if(mHaveAllocation)
   {
      mRequestedLifetime = 0;
      mRequestedBandwidth = UnspecifiedBandwidth;
      mRequestedPortProps = StunMessage::PortPropsNone;
      mRequestedPort = UnspecifiedPort;
      mRequestedTransportType = StunTuple::None;
      mRequestedIpAddress = UnspecifiedIpAddress;

      return refreshAllocation();
   }
   else
   {
      return asio::error_code(reTurn::NoAllocation, asio::misc_ecat); 
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

   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::misc_ecat); 
   }

   // Form Set Active Destination request
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnSetActiveDestinationRequest);
   request.mHasTurnRemoteAddress = true;
   request.mTurnRemoteAddress.port = port;
   if(address.is_v6())
   {
      request.mTurnRemoteAddress.family = StunMessage::IPv6Family;
      memcpy(&request.mTurnRemoteAddress.addr.ipv6, address.to_v6().to_bytes().c_array(), sizeof(request.mTurnRemoteAddress.addr.ipv6));
   }
   else
   {
      request.mTurnRemoteAddress.family = StunMessage::IPv4Family;
      request.mTurnRemoteAddress.addr.ipv4 = address.to_v4().to_ulong();
   }
   request.mHasFingerprint = true;
   request.mHasMessageIntegrity = true;

   request.setUsername(mUsername.data()); 
   request.mHmacKey = mPassword;

   // Get Response
   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::misc_ecat);
      delete response;
      return errorCode;
   }

   // All was well - return 0 errorCode
   mActiveDestination.setTransportType(mRelayTuple.getTransportType());
   mActiveDestination.setAddress(address);
   mActiveDestination.setPort(port);

   delete response;
   return errorCode;
}

asio::error_code 
TurnSocket::clearActiveDestination()
{
   asio::error_code errorCode;

   // ensure there is an allocation
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::misc_ecat); 
   }

   // Form Set Active Destination request with no destination (clear)
   StunMessage request;
   request.createHeader(StunMessage::StunClassRequest, StunMessage::TurnSetActiveDestinationRequest);
   request.mHasFingerprint = true;
   request.mHasMessageIntegrity = true;

   request.setUsername(mUsername.data()); 
   request.mHmacKey = mPassword;

   // Get Response
   StunMessage* response = sendRequestAndGetResponse(request, errorCode);
   if(response == 0)
   {
      return errorCode;
   }

   // Check if success or not
   if(response->mHasErrorCode)
   {
      errorCode = asio::error_code(response->mErrorCode.errorClass * 100 + response->mErrorCode.number, asio::misc_ecat);
      delete response;
      return errorCode;
   }

   // All was well - return 0 errorCode
   mActiveDestination.setTransportType(StunTuple::None);
   mActiveDestination.setAddress(UnspecifiedIpAddress);
   mActiveDestination.setPort(0);
   delete response;
   return errorCode;
}

asio::error_code 
TurnSocket::send(const char* buffer, unsigned int size)
{
   // Allow raw data to be sent if there is no allocation
   if(!mHaveAllocation)
   {
      return rawWrite(buffer, size);
   }

   // !slg! TODO - ensure active destination is set 

   if(mLocalBinding.getTransportType() == StunTuple::UDP)
   {
      // send non-stun data to active destination (or connection) as is
      return rawWrite(buffer, size);
   }
   else
   {
      // send TCP framed data to active destination
      char framing[4];
      framing[0] = 0x03;
      framing[1] = 0x00;  // reserved
      UInt16 turnDataSize = size;
      turnDataSize = htons(turnDataSize);
      memcpy((void*)&framing[2], &turnDataSize, 2);
      std::vector<asio::const_buffer> bufs;
      bufs.push_back(asio::buffer(framing, sizeof(framing)));
      bufs.push_back(asio::buffer(buffer, size));

      return rawWrite(bufs);
   }
}

asio::error_code 
TurnSocket::sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   // ensure there is an allocation 
   if(!mHaveAllocation)
   {
      return asio::error_code(reTurn::NoAllocation, asio::misc_ecat); 
   }

   // Wrap data in a SendInd
   StunMessage ind;
   ind.createHeader(StunMessage::StunClassIndication, StunMessage::TurnSendInd);
   ind.mHasTurnRemoteAddress = true;
   ind.mTurnRemoteAddress.port = port;
   if(address.is_v6())
   {
      ind.mTurnRemoteAddress.family = StunMessage::IPv6Family;
      memcpy(&ind.mTurnRemoteAddress.addr.ipv6, address.to_v6().to_bytes().c_array(), sizeof(ind.mTurnRemoteAddress.addr.ipv6));
   }
   else
   {
      ind.mTurnRemoteAddress.family = StunMessage::IPv4Family;
      ind.mTurnRemoteAddress.addr.ipv4 = address.to_v4().to_ulong();
   }
   ind.setTurnData(buffer, size);
   ind.mHasFingerprint = true;

   // Send indication to Turn Server
   unsigned int msgsize = ind.stunEncodeMessage(mBuffer, sizeof(mBuffer));
   return rawWrite(mBuffer, msgsize);
}

asio::error_code 
TurnSocket::receive(char* buffer, unsigned int& size, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;
   // TODO - rethink this scheme so that we don't need to copy recieved data

   // Wait for response
   unsigned int readSize;
   errorCode = rawRead(mBuffer, sizeof(mBuffer), &readSize, sourceAddress, sourcePort); // Note: SourceAddress and sourcePort may be overwritten below if from Turn Relay
   if(errorCode != 0)
   {
      return errorCode;
   }

   if(!mHaveAllocation)
   {
      return handleRawData(mBuffer, readSize, readSize, buffer, size);
   }

   // If data is received on Client UDP socket then check for Magic Cookie - if present process as StunMessage
   if(mLocalBinding.getTransportType() == StunTuple::UDP)
   {
      // Check if first byte is 0x00 - if so data might be STUN message
      if(readSize >= 1 && mBuffer[0] == 0x00)
      {
         // check for existance of StunMagicCookie
         if(readSize > 8 )  
         {
            unsigned int magicCookie;  // Stun magic cookie is in bytes 4-8
            memcpy(&magicCookie, &mBuffer[4], sizeof(magicCookie));
            //magicCookie = ntohl(magicCookie);
            if(magicCookie == StunMessage::StunMagicCookie)
            {
               // StunMessage
               StunMessage* stunMsg = new StunMessage(mLocalBinding, mTurnServer, mBuffer, readSize);
               return handleStunMessage(*stunMsg, buffer, size, sourceAddress, sourcePort);
            }
         }
      }
      
      return handleRawData(mBuffer, readSize, readSize, buffer, size, sourceAddress, sourcePort);
   }
   else
   {
      // Process TCP Framed Data
      // Note:  we only accept the following:
      //        1.  Unframed Stun Requests - first octet 0x00
      //        2.  Framed Stun Requests - first octet 0x02
      //        3.  Framed Data - first octet 0x03
      if(readSize >= 4 && mBuffer[0] == 0x00)  // Unframed Stun
      {
         // This is likely a StunMessage - length will be in bytes 3 and 4
         UInt16 stunMsgLen;
         memcpy(&stunMsgLen, &mBuffer[2], 2);
         stunMsgLen = ntohs(stunMsgLen) + 20;  // 20 bytes for header
         if(readSize != stunMsgLen)
         {
            // TODO - fix read logic so that we can read in chuncks
            std::cout << "Did not read entire message: read=" << readSize << " wanted=" << stunMsgLen << std::endl;
            return asio::error_code(reTurn::ReadError, asio::misc_ecat); 
         }
         // check for existance of StunMagicCookie
         if(readSize > 8 )  
         {
            unsigned int magicCookie;  // Stun magic cookie is in bytes 4-8
            memcpy(&magicCookie, &mBuffer[4], sizeof(magicCookie));
            //magicCookie = ntohl(magicCookie);
            if(magicCookie == StunMessage::StunMagicCookie)
            {
               // StunMessage
               StunMessage* stunMsg = new StunMessage(mLocalBinding, mTurnServer, mBuffer, readSize);
               return handleStunMessage(*stunMsg, buffer, size, sourceAddress, sourcePort);
            }
         }
         // Treat as data
         return handleRawData(mBuffer, readSize, readSize, buffer, size, sourceAddress, sourcePort);
      }
      else if(mBuffer[0] == 0x02) // Framed Stun Request
      {
         // This is a StunMessage - length will be in bytes 3 and 4
         UInt16 stunMsgLen;
         memcpy(&stunMsgLen, &mBuffer[2], 2);
         stunMsgLen = ntohs(stunMsgLen);  // Framed length will be entire size of StunMessage

         if(readSize != (unsigned int)stunMsgLen+4)
         {
            // TODO - fix read logic so that we can read in chuncks
            std::cout << "Did not read entire message: read=" << readSize << " wanted=" << stunMsgLen+4 << std::endl;
            return asio::error_code(reTurn::ReadError, asio::misc_ecat);
         }

         // Process Stun Message
         StunMessage* stunMsg = new StunMessage(mLocalBinding, mTurnServer, mBuffer+4, stunMsgLen);
         return handleStunMessage(*stunMsg, buffer, size, sourceAddress, sourcePort);
      }
      else if(mBuffer[0] == 0x03) // Framed Data
      {
         UInt16 dataLen;
         memcpy(&dataLen, &mBuffer[2], 2);
         dataLen = ntohs(dataLen);

         return handleRawData(&mBuffer[4], readSize-4, dataLen, buffer, size, sourceAddress, sourcePort);
      }
      else
      {
         std::cout << "Invalid data on TCP connection." << std::endl;
         return asio::error_code(reTurn::ReadError, asio::misc_ecat);
      }
   }
   return errorCode;
}

asio::error_code 
TurnSocket::receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size)
{
   asio::ip::address sourceAddress;
   unsigned short sourcePort;
   bool done = false;
   asio::error_code errorCode;

   while(!done)
   {
      done = true;
      errorCode = receive(buffer, size, &sourceAddress, &sourcePort);
      if(errorCode == 0)
      {
         if(sourceAddress != address || sourcePort != port)
         {
            std::cout << "Recevied message but not from requested address/port - Discarding." << std::endl;            
            done = false;
         }
      }
   }
   return errorCode;
}

asio::error_code 
TurnSocket::handleRawData(char* data, unsigned int dataSize, unsigned int expectedSize, char* buffer, unsigned int& bufferSize, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;

   if(dataSize != expectedSize)
   {
      // TODO - fix read logic so that we can read in chuncks
      std::cout << "Did not read entire message: read=" << dataSize << " wanted=" << expectedSize << std::endl;
      return asio::error_code(reTurn::ReadError, asio::misc_ecat); 
   }

   if(dataSize > bufferSize) 
   {
     // Passed in buffer is not large enough
     std::cout << "Passed in buffer not large enough." << std::endl;
     return asio::error_code(reTurn::BufferTooSmall, asio::misc_ecat); 
   }

   // Copy data to return buffer
   memcpy(buffer, data, dataSize);
   bufferSize = dataSize;

   if(sourceAddress != 0)
   {
      *sourceAddress = mActiveDestination.getAddress();
   }
   if(sourcePort != 0)
   {
      *sourcePort = mActiveDestination.getPort();
   }
   return errorCode;
}

asio::error_code 
TurnSocket::handleStunMessage(StunMessage& stunMessage, char* buffer, unsigned int& size, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;
   if(stunMessage.isValid())
   {
      if(stunMessage.mClass == StunMessage::StunClassIndication && stunMessage.mMethod == StunMessage::TurnDataInd)
      {
         if(stunMessage.mHasTurnData)
         {
            if(stunMessage.mTurnData->size() > size)
            {
               // Passed in buffer is not large enough
               std::cout << "Passed in buffer not large enough." << std::endl;
               return asio::error_code(reTurn::BufferTooSmall, asio::misc_ecat);
            }

            memcpy(buffer, stunMessage.mTurnData->data(), stunMessage.mTurnData->size());
            size = (unsigned int)stunMessage.mTurnData->size();

            if(stunMessage.mHasTurnRemoteAddress && sourceAddress != 0)
            {
               if(stunMessage.mTurnRemoteAddress.family == StunMessage::IPv6Family)
               {
                  asio::ip::address_v6::bytes_type bytes;
                  memcpy(bytes.c_array(), &stunMessage.mTurnRemoteAddress.addr.ipv6, bytes.size());
                  *sourceAddress = asio::ip::address_v6(bytes);
               }
               else
               {
                  *sourceAddress = asio::ip::address_v4(stunMessage.mTurnRemoteAddress.addr.ipv4);
               }
            }
            if(stunMessage.mHasTurnRemoteAddress && sourcePort != 0)
            {
               *sourcePort = stunMessage.mTurnRemoteAddress.port;
            }
         }
      }
      else
      {
         // TODO - handle others ????
      }
   }
   else
   {
      std::cout << "Read Invalid StunMsg." << std::endl;
      return asio::error_code(reTurn::ErrorParsingMessage, asio::misc_ecat);
   }
   return errorCode;
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


