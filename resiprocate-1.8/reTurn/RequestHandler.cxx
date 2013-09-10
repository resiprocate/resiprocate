#include "RequestHandler.hxx"
#include <fstream>
#include <sstream>
#include <string>

#include "TurnAllocation.hxx"
#include "AsyncSocketBase.hxx"
#include "StunAuth.hxx"
#include <rutil/Random.hxx>
#include <rutil/Timer.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;
using namespace resip;

namespace reTurn {

// !slg! TODO these need to be made into settings
#define SOFTWARE_STRING "reTURNServer 0.4 - RFC5389/turn-12  "  // Note padding size to a multiple of 4, to help compatibility with older clients
#define DEFAULT_BANDWIDTH 100  // 100 kbit/s - enough for G711 RTP ?slg? what do we want this to be?

RequestHandler::RequestHandler(TurnManager& turnManager,
                               const asio::ip::address* prim3489Address, unsigned short* prim3489Port,
                               const asio::ip::address* alt3489Address, unsigned short* alt3489Port) 
 : mTurnManager(turnManager)
{
   if(prim3489Address && prim3489Port && alt3489Address && alt3489Port)
   {
      mRFC3489SupportEnabled = true;
      mPrim3489Address = *prim3489Address;
      mPrim3489Port = *prim3489Port;
      mAlt3489Address = *alt3489Address;
      mAlt3489Port = *alt3489Port;
   }
   else
   {
      mRFC3489SupportEnabled = false;
   }
   mPrivateNonceKey = Random::getRandomHex(24);
}

RequestHandler::ProcessResult 
RequestHandler::processStunMessage(AsyncSocketBase* turnSocket, StunMessage& request, StunMessage& response, bool isRFC3489BackwardsCompatServer)
{
   ProcessResult result =  RespondFromReceiving;

   response.mRemoteTuple = request.mRemoteTuple; // Default to send response back to sender

   if(handleAuthentication(request, response))  
   {
      // Check if there were unknown require attributes
      if(request.mUnknownRequiredAttributes.numAttributes > 0)
      {
         InfoLog(<< "Received Request with unknown comprehension-required attributes. Sending 420.");
         buildErrorResponse(response, 420, "Unknown attribute", getConfig().mAuthenticationRealm.c_str());  
         response.mHasUnknownAttributes = true;
         response.mUnknownAttributes = request.mUnknownRequiredAttributes;
      }
      else
      {
         // Request is authenticated, process it
         switch(request.mClass)
         { 
         case StunMessage::StunClassRequest:
            switch (request.mMethod) 
            {
            case StunMessage::BindMethod:
               result = processStunBindingRequest(request, response, isRFC3489BackwardsCompatServer);
               break;

            case StunMessage::SharedSecretMethod:
               result = processStunSharedSecretRequest(request, response);
               break;

            case StunMessage::TurnAllocateMethod:
               result = processTurnAllocateRequest(turnSocket, request, response);
               if(result != NoResponseToSend)
               {
                  // Add an XOrMappedAddress to all TurnAllocateResponses
                  response.mHasXorMappedAddress = true;
                  StunMessage::setStunAtrAddressFromTuple(response.mXorMappedAddress, request.mRemoteTuple);
               }
               break;

            case StunMessage::TurnRefreshMethod:
               result = processTurnRefreshRequest(request, response);
               break;

            case StunMessage::TurnCreatePermissionMethod:
               result = processTurnCreatePermissionRequest(request, response);
               break;

            case StunMessage::TurnChannelBindMethod:
               result = processTurnChannelBindRequest(request, response);
               break;

            default:
               buildErrorResponse(response, 400, "Invalid Request Method");  
               break;
            }
            break;

         case StunMessage::StunClassIndication:
            result = NoResponseToSend;  // Indications don't have responses
            switch (request.mMethod) 
            {
            case StunMessage::TurnSendMethod:
               processTurnSendIndication(request);
               break;
   
            case StunMessage::BindMethod:
               // A Bind indication is simply a keepalive with no response required
               break;
   
            case StunMessage::TurnDataMethod: // Don't need to handle these - only sent by server, never received
            default:
               // Unknown indication - just ignore
               break;
            }
            break;
   
         case StunMessage::StunClassSuccessResponse:
         case StunMessage::StunClassErrorResponse:
         default:
            // A server should never receive a response
            result = NoResponseToSend;
            break;
         }
      }
   }

   if(result != NoResponseToSend)
   {
      // Fill in common response properties
      response.mMethod = request.mMethod;

      // Copy over TransactionId
      response.mHeader.magicCookieAndTid = request.mHeader.magicCookieAndTid;

      if (1) // add Software name - could be a setting in the future
      {
         response.setSoftware(SOFTWARE_STRING);
      }

      // If fingerprint is used in request, then use fingerprint in response
      if(request.mHasFingerprint)
      {
         response.mHasFingerprint = true;
      }
   }

   return result;
}

void
RequestHandler::buildErrorResponse(StunMessage& response, unsigned short errorCode, const char* msg, const char* realm)
{
   response.mClass = StunMessage::StunClassErrorResponse;
   response.setErrorCode(errorCode, msg);
   if(realm)
   {
      response.setRealm(realm);

      // Add a random nonce value that is expirable
      Data nonce(100, Data::Preallocate);
      Data timestamp(Timer::getTimeMs()/1000);
      generateNonce(timestamp, nonce);
      response.setNonce(nonce.c_str());
   }
}

void 
RequestHandler::generateNonce(const Data& timestamp, Data& nonce)
{
   nonce += timestamp;
   nonce += ":";
   Data noncePrivate(100, Data::Preallocate);
   noncePrivate += timestamp;
   noncePrivate += ":";
   //noncePrivate += user;  // ?slg? What could we put here
   noncePrivate += mPrivateNonceKey;
   nonce += noncePrivate.md5();
}

RequestHandler::CheckNonceResult
RequestHandler::checkNonce(const Data& nonce)
{
   ParseBuffer pb(nonce.data(), nonce.size());
   if (!pb.eof() && !isdigit(*pb.position()))
   {
      DebugLog(<< "Invalid nonce.  Expected timestamp.");
      return NotValid;
   }
   const char* anchor = pb.position();
   pb.skipToChar(':');
   if (pb.eof())
   {
      DebugLog(<< "Invalid nonce. Expected timestamp terminator.");
      return NotValid;
   }
   UInt64 now = Timer::getTimeMs()/1000;
   Data creationTimeData;
   UInt64 creationTime;
   pb.data(creationTimeData, anchor);
   creationTime = creationTimeData.convertUInt64();
   if((now-creationTime) <= getConfig().mNonceLifetime)
   {
      // If nonce hasn't expired yet - ensure this is a nonce we generated
      Data nonceToMatch(100, Data::Preallocate);
      generateNonce(creationTimeData, nonceToMatch);
      if(nonceToMatch == nonce)
      {
         return Valid;
      }
      else
      {
         DebugLog(<< "Invalid nonce.  Not generated by this server.");
         return NotValid;
      }
   }
   else
   {
      DebugLog(<< "Invalid nonce.  Expired.");
      return Expired;
   }
}

bool 
RequestHandler::handleAuthentication(StunMessage& request, StunMessage& response)
{
   // Don't authenticate shared secret requests, Binding Requests or Indications (if LongTermCredentials are used)
   if((request.mClass == StunMessage::StunClassRequest && request.mMethod == StunMessage::SharedSecretMethod) ||
      (request.mClass == StunMessage::StunClassRequest && request.mMethod == StunMessage::BindMethod) ||
      (getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword && request.mClass == StunMessage::StunClassIndication))
   {
      return true;
   }

   if (!request.mHasMessageIntegrity)
   {
      if (getConfig().mAuthenticationMode == ReTurnConfig::ShortTermPassword) 
      {
         InfoLog(<< "Received Request with no Message Integrity. Sending 400.");
         buildErrorResponse(response, 400, "Bad Request (no MessageIntegrity)");  
         return false;
      }
      else if(getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword)
      {
         InfoLog(<< "Received Request with no Message Integrity. Sending 401.");
         buildErrorResponse(response, 401, "Unauthorized (no MessageIntegrity)", getConfig().mAuthenticationRealm.c_str());  
         return false;
      }
   }
   else
   {
      if (!request.mHasUsername)
      {
         WarningLog(<< "No Username and contains MessageIntegrity. Sending 400.");
         buildErrorResponse(response, 400, "Bad Request (no Username and contains MessageIntegrity)");
         return false;
      }

      if(getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword)  
      {
         if(!request.mHasRealm)
         {
            WarningLog(<< "No Realm.  Sending 400.");
            buildErrorResponse(response, 400, "Bad Request (No Realm)");
            return false;
         }
         if(!request.mHasNonce)
         {
            WarningLog(<< "No Nonce and contains realm.  Sending 400.");
            buildErrorResponse(response, 400, "Bad Request (No Nonce and contains Realm)");
            return false;
         }
         switch(checkNonce(*request.mNonce))
         {
         case Valid:
            // Do nothing
            break;
         case Expired:
            WarningLog(<< "Nonce expired. Sending 438.");
            buildErrorResponse(response, 438, "Stale Nonce", getConfig().mAuthenticationRealm.c_str());
            return false;
            break;
         case NotValid:
         default:
            WarningLog(<< "Invalid Nonce. Sending 400.");
            buildErrorResponse(response, 400, "BadRequest (Invalid Nonce)");
            return false;
            break;
         }
      }

      if(getConfig().mAuthenticationMode == ReTurnConfig::ShortTermPassword)
      {
         // !slg! check if username field has expired
         // !slg! we may want to delay this check for Turn Allocations so that expired 
         //       authentications can still be accepted for existing allocation refreshes 
         //       and removals
         if(0)
         {
            WarningLog(<< "Username expired. Sending 430.");
            buildErrorResponse(response, 430, "Stale Credentials");
            return false;
         }
      }

      DebugLog(<< "Validating username: " << *request.mUsername);

      // !slg! need to determine whether the USERNAME contains a known entity, and in the case of a long-term
      //       credential, known within the realm of the REALM attribute of the request
      if (getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword && !getConfig().isUserNameValid(*request.mUsername))
      {
         WarningLog(<< "Invalid username: " << *request.mUsername << ". Sending 401.");
         buildErrorResponse(response, 401, "Unathorized", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0);
         return false;
      }

      DebugLog(<< "Validating MessageIntegrity");

      // Need to calculate HMAC across entire message - for ShortTermAuthentication we use the password
      // as the key - for LongTermAuthentication we use username:realm:password string as the key
      Data hmacKey;
      assert(request.mHasUsername);

      if(getConfig().mAuthenticationMode != ReTurnConfig::NoAuthentication)
      {
         request.calculateHmacKey(hmacKey, getConfig().getPasswordForUsername(*request.mUsername));
      }
      
      if(!request.checkMessageIntegrity(hmacKey))
      {
         WarningLog(<< "MessageIntegrity is bad. Sending 401.");
         buildErrorResponse(response, 401, "Unauthorized", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0);
         return false;
      }

      // need to compute this later after message is filled in
      response.mHasMessageIntegrity = true;
      response.mHmacKey = hmacKey;  // Used to later calculate Message Integrity during encoding
   }

   return true;
}

RequestHandler::ProcessResult 
RequestHandler::processStunBindingRequest(StunMessage& request, StunMessage& response, bool isRFC3489BackwardsCompatServer)
{
   ProcessResult result = RespondFromReceiving;

   // form the outgoing message
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Add XOrMappedAddress to response if RFC5389 sender 
   if(request.hasMagicCookie())
   {
      response.mHasXorMappedAddress = true;
      StunMessage::setStunAtrAddressFromTuple(response.mXorMappedAddress, request.mRemoteTuple);
   }
   else
   {
      // Add Mapped address to response
      response.mHasMappedAddress = true;
      StunMessage::setStunAtrAddressFromTuple(response.mMappedAddress, request.mRemoteTuple);

      if(0) // TODO - setting to add XOR address even if older client
      {
         response.mHasXorMappedAddress = true;
         StunMessage::setStunAtrAddressFromTuple(response.mXorMappedAddress, request.mRemoteTuple);
      }
   }

   // the following code is for RFC3489 backward compatibility
   if(mRFC3489SupportEnabled && isRFC3489BackwardsCompatServer)
   {
      StunTuple sendFromTuple;
      StunTuple changeTuple;

      sendFromTuple.setTransportType(request.mLocalTuple.getTransportType());
      changeTuple.setTransportType(request.mLocalTuple.getTransportType());
      changeTuple.setAddress(request.mLocalTuple.getAddress() == mPrim3489Address ? mAlt3489Address : mPrim3489Address);
      changeTuple.setPort(request.mLocalTuple.getPort() == mPrim3489Port ? mAlt3489Port : mPrim3489Port);
      UInt32 changeRequest = request.mHasChangeRequest ? request.mChangeRequest : 0;

      if(changeRequest & StunMessage::ChangeIpFlag && changeRequest & StunMessage::ChangePortFlag)
      {
         result = RespondFromAlternateIpPort;
         sendFromTuple.setAddress(changeTuple.getAddress());
         sendFromTuple.setPort(changeTuple.getPort());
      }
      else if(changeRequest & StunMessage::ChangePortFlag)
      {
         result = RespondFromAlternatePort;
         sendFromTuple.setAddress(request.mLocalTuple.getAddress());
         sendFromTuple.setPort(changeTuple.getPort());
      }
      else if(changeRequest & StunMessage::ChangeIpFlag)
      {
         result = RespondFromAlternateIp;
         sendFromTuple.setAddress(changeTuple.getAddress());
         sendFromTuple.setPort(request.mLocalTuple.getPort());
      }
      else
      {
         // default to send from receiving transport
         sendFromTuple.setAddress(request.mLocalTuple.getAddress());
         sendFromTuple.setPort(request.mLocalTuple.getPort());
      }

      // Add source address - for RFC3489 backwards compatibility
      response.mHasSourceAddress = true;
      StunMessage::setStunAtrAddressFromTuple(response.mSourceAddress, sendFromTuple);

      // Add changed address - for RFC3489 backward compatibility
      response.mHasChangedAddress = true;
      StunMessage::setStunAtrAddressFromTuple(response.mChangedAddress, changeTuple);

      // If Response-Address is present, then response should be sent here instead of source address to be
      // compliant with RFC3489.  In this case we need to add a REFLECTED-FROM attribute
      if(request.mHasResponseAddress)
      {
         StunMessage::setTupleFromStunAtrAddress(response.mRemoteTuple, request.mResponseAddress);

         // If the username is present and is greater than or equal to 92 bytes long, then we assume this username
         // was obtained from a shared secret request
         if (request.mHasUsername && (request.mUsername->size() >= 92 ) )
         {
            // Extract source address that sent the shared secret request from the encoded username and use this in response address
            StunTuple responseTuple;
            request.getTupleFromUsername(responseTuple);

            response.mHasReflectedFrom = true;
            StunMessage::setStunAtrAddressFromTuple(response.mReflectedFrom, responseTuple);
         }
         else
         {
            response.mHasReflectedFrom = true;
            StunMessage::setStunAtrAddressFromTuple(response.mReflectedFrom, request.mRemoteTuple);
         }
      }
   }

   return result;
}

RequestHandler::ProcessResult 
RequestHandler::processStunSharedSecretRequest(StunMessage& request, StunMessage& response)
{
   // Only allow shared secret requests on TLS transport
   if(request.mLocalTuple.getTransportType() != StunTuple::TLS)
   {
      WarningLog(<< "Invalid transport for shared secret request.  TLS required.  Sending 433."); 
      buildErrorResponse(response, 433, "Invalid transport.  TLS required.");
      return RespondFromReceiving;
   }

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Set the username and password
   response.createUsernameAndPassword();

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnAllocateRequest(AsyncSocketBase* turnSocket, StunMessage& request, StunMessage& response)
{
   // Turn Allocate requests must be authenticated (note: if this attribute is present 
   // then handleAuthentication would have validated authentication info)
   if(!request.mHasMessageIntegrity)
   {
      WarningLog(<< "Turn allocate request without authentication.  Sending 401.");
      buildErrorResponse(response, 401, "Missing Message Integrity", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, getConfig().getPasswordForUsername(*request.mUsername));

   DebugLog(<< "Allocation request received: localTuple=" << request.mLocalTuple << ", remoteTuple=" << request.mRemoteTuple);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   // If this is a subsequent allocation request, return error
   if(allocation)
   {
      WarningLog(<< "Allocation requested but already exists.  Sending 437.");
      buildErrorResponse(response, 437, "Allocation Mismatch");  
      return RespondFromReceiving;
   }

   // TODO - add a check that a per-user quota for number of allowed TurnAllocations 
   // has not been exceeded - if so send 486 (Allocation Quota Reached)

   // Build the Allocation Tuple
   StunTuple allocationTuple(request.mLocalTuple.getTransportType(), // Default to receiving transport
                             request.mLocalTuple.getAddress(),   // default ip address is at the discretion of the server
                             0);  // port to be populated later

   // Check for requested properties
   if(request.mHasTurnRequestedTransport)
   {
      if(request.mTurnRequestedTransport != StunMessage::RequestedTransportUdp)  // We only support UDP allocations right now
      {
         WarningLog(<< "Invalid transport requested.  Sending 442.");
         buildErrorResponse(response, 442, "Unsupported Transport Protocol");  
         return RespondFromReceiving;
      }
      allocationTuple.setTransportType(StunTuple::UDP);
   }
   else
   {
      WarningLog(<< "Missing requested transport header.  Sending 400.");
      buildErrorResponse(response, 400, "Bad Request - Missing requested transport");  
      return RespondFromReceiving;
   }

   // Check if Don't Fragment attribute is present - if so return an error - TODO implement DF bit, then remove this check
   if(request.mHasTurnDontFragment)
   {
      WarningLog(<< "Turn allocate request with Don't Fragment requested, not yet implemented.  Sending 420.");
      buildErrorResponse(response, 420, "Don't Fragment not yet implemented", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0 );  
      return RespondFromReceiving;
   }

   // Check if bandwidth is available
   if(request.mHasTurnBandwidth)
   {
      // TODO - bandwidth check - if insufficient send 507 (Insufficient Bandwidth Capacity)
   }
                                
   unsigned short port = 0;  // Default to Any free port
   if(request.mHasTurnEvenPort)
   {
      if(request.mHasTurnReservationToken)
      {
         WarningLog(<< "Both Even Port and Reservation Token attributes are present.  Sending 400.");
         buildErrorResponse(response, 400, "Bad request - both Even Port and Reservation Token present");  
         return RespondFromReceiving;
      }
      if(request.mTurnEvenPort.propType == StunMessage::PropsPortEven)
      {
         // Attempt to allocate an even port
         port = mTurnManager.allocateEvenPort(allocationTuple.getTransportType());
      }
      else if(request.mTurnEvenPort.propType == StunMessage::PropsPortPair)
      {
         // Attempt to allocate an even port, with a free adjacent odd port
         port = mTurnManager.allocateEvenPortPair(allocationTuple.getTransportType());

         // Add Reservation Token to response - for now just use reserved port number as token
         response.mHasTurnReservationToken = true;
         response.mTurnReservationToken = port + 1;
      }
      if(port == 0)
      {
         WarningLog(<< "Unable to allocate requested port.  Sending 508.");
         buildErrorResponse(response, 508, "Insufficient Port Capacity");  
         return RespondFromReceiving;
      }
   }

   if(request.mHasTurnReservationToken)
   {
      // Try to allocate reserved port - for now reservation token is reserved port number
      port = (unsigned short)request.mTurnReservationToken;
      if(!mTurnManager.allocatePort(allocationTuple.getTransportType(), port, true /* allocate reserved */))
      {
         WarningLog(<< "Unable to allocate requested port - bad reservation token.  Sending 508.");
         buildErrorResponse(response, 508, "Insufficient Port Capacity");  
         return RespondFromReceiving;
      }      
   }

   if(port == 0)
   {
      // Allocate any available port
      port = mTurnManager.allocateAnyPort(allocationTuple.getTransportType());      
      if(port == 0)
      {
         WarningLog(<< "Unable to allocate port.  Sending 508.");
         buildErrorResponse(response, 508, "Insufficient Port Capacity");  
         return RespondFromReceiving;
      }      
   }

   allocationTuple.setPort(port);

   UInt32 lifetime = getConfig().mDefaultAllocationLifetime;
   if(request.mHasTurnLifetime)
   {
      // Check if the requested value is greater than the server max
      if(request.mTurnLifetime > getConfig().mMaxAllocationLifetime)
      {
         lifetime = getConfig().mMaxAllocationLifetime;
      }
      // The server should ignore requests for a lifetime less than it's default
      else if(request.mTurnLifetime > getConfig().mDefaultAllocationLifetime)
      {
         lifetime = request.mTurnLifetime;
      }
   }

   // We now have an internal 5-Tuple and an allocation tuple - create the allocation

   try
   {
      allocation = new TurnAllocation(mTurnManager,
                                      turnSocket, 
                                      request.mLocalTuple, 
                                      request.mRemoteTuple, 
                                      StunAuth(*request.mUsername, hmacKey), 
                                      allocationTuple, 
                                      lifetime);
   }
   catch(asio::system_error e)
   {
      // TODO - handle port in use error better - try to allocate a new port or something?
      ErrLog(<< "Error allocating socket for allocation.  Sending 500.");
      buildErrorResponse(response, 500, "Server Error");  
      return RespondFromReceiving;
   }

   // Add the new allocation to be managed
   mTurnManager.addTurnAllocation(allocation);

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   response.mHasTurnLifetime = true;
   response.mTurnLifetime = lifetime;

   response.mHasTurnXorRelayedAddress = true;
   StunMessage::setStunAtrAddressFromTuple(response.mTurnXorRelayedAddress, allocation->getRequestedTuple());

   // Note:  XorMappedAddress is added to all TurnAllocate responses in processStunMessage
  
   //Reserved for future draft
   //response.mHasTurnBandwidth = true;
   //response.mTurnBandwidth = request.mHasTurnBandwidth ? request.mTurnBandwidth : DEFAULT_BANDWIDTH;

   // Note: Message Integrity added by handleAuthentication

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnRefreshRequest(StunMessage& request, StunMessage& response)
{
   // Turn Allocate requests must be authenticated (note: if this attribute is present 
   // then handleAuthentication would have validation authentication info)
   if(!request.mHasMessageIntegrity)
   {
      WarningLog(<< "Turn refresh request without authentication.  Sending 401.");
      buildErrorResponse(response, 401, "Missing Message Integrity", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, getConfig().getPasswordForUsername(*request.mUsername));

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Refresh requested with non-matching allocation.  Sending 437."); 
      buildErrorResponse(response, 437, "Allocation Mismatch");  
      return RespondFromReceiving;
   }

   // If allocation was found, then ensure that the same username and shared secret was used
   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      WarningLog(<< "Refresh requested with username not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      WarningLog(<< "Refresh requested with shared secret not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");   
      return RespondFromReceiving;
   }

   // check if Lifetime is 0, if so then just send success response
   if(request.mHasTurnLifetime && request.mTurnLifetime == 0)
   {
      // form the outgoing success response
      response.mClass = StunMessage::StunClassSuccessResponse;

      response.mHasTurnLifetime = true;
      response.mTurnLifetime = 0;

      // If allocation exists then delete it
      if(allocation)
      {
         mTurnManager.removeTurnAllocation(allocation->getKey());  // will delete allocation
      }

      return RespondFromReceiving;
   }

   UInt32 lifetime = getConfig().mDefaultAllocationLifetime;
   if(request.mHasTurnLifetime)
   {
      // Check if the requested value is greater than the server max
      if(request.mTurnLifetime > getConfig().mMaxAllocationLifetime)
      {
         lifetime = getConfig().mMaxAllocationLifetime;
      }
      // The server should ignore requests for a lifetime less than it's default
      else if(request.mTurnLifetime > getConfig().mDefaultAllocationLifetime)
      {
         lifetime = request.mTurnLifetime;
      }
   }

   // Check if this is a subsequent allocate request 
   allocation->refresh(lifetime);

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   response.mHasTurnLifetime = true;
   response.mTurnLifetime = lifetime;
  
   //Reserved for future draft
   //response.mHasTurnBandwidth = true;
   //response.mTurnBandwidth = request.mHasTurnBandwidth ? request.mTurnBandwidth : DEFAULT_BANDWIDTH;

   // Note: Message Integrity added by handleAuthentication

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnCreatePermissionRequest(StunMessage& request, StunMessage& response)
{
   // TurnCreatePermission requests must be authenticated
   if(!request.mHasMessageIntegrity)
   {
      WarningLog(<< "Turn create permission request without authentication.  Send 401.");
      buildErrorResponse(response, 401, "Missing Message Integrity", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, getConfig().getPasswordForUsername(*request.mUsername));

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn create permission request for non-existing allocation.  Send 437."); 
      buildErrorResponse(response, 437, "No Allocation");  
      return RespondFromReceiving;
   }

   // If allocation was found, then ensure that the same username and shared secret was used
   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      WarningLog(<< "Create permission requested with username not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      WarningLog(<< "Create permission requested with shared secret not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");   
      return RespondFromReceiving;
   }

   if(request.mCntTurnXorPeerAddress > 0)
   {
      StunTuple remoteAddress;
      remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
      for (int i = 0; i < request.mCntTurnXorPeerAddress; i++)
      {
         StunMessage::setTupleFromStunAtrAddress(remoteAddress, request.mTurnXorPeerAddress[i]);
         allocation->refreshPermission(remoteAddress.getAddress());
      }
   }
   else
   {
      WarningLog(<< "Create permission request missing peer address.  Sending 400.");
      buildErrorResponse(response, 400, "Bad Request - missing attribute");   
      return RespondFromReceiving;
   }

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Note: Message Integrity added by handleAuthentication

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnChannelBindRequest(StunMessage& request, StunMessage& response)
{
   // TurnChannelBind requests must be authenticated
   if(!request.mHasMessageIntegrity)
   {
      WarningLog(<< "Turn channel bind request without authentication.  Send 401.");
      buildErrorResponse(response, 401, "Missing Message Integrity", getConfig().mAuthenticationMode == ReTurnConfig::LongTermPassword ? getConfig().mAuthenticationRealm.c_str() : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, getConfig().getPasswordForUsername(*request.mUsername));

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn channel bind request for non-existing allocation.  Send 437."); 
      buildErrorResponse(response, 437, "No Allocation");  
      return RespondFromReceiving;
   }

   // If allocation was found, then ensure that the same username and shared secret was used
   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      WarningLog(<< "Channel bind requested with username not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      WarningLog(<< "Channel bind requested with shared secret not matching allocation.  Sending 441.");
      buildErrorResponse(response, 441, "Wrong Credentials");   
      return RespondFromReceiving;
   }

   if(request.mCntTurnXorPeerAddress > 0 && request.mHasTurnChannelNumber)
   {
      // Ensure channel number is in valid range
      if(request.mTurnChannelNumber < MIN_CHANNEL_NUM || request.mTurnChannelNumber > MAX_CHANNEL_NUM)
      {
         WarningLog(<< "Channel bind requested with an out of range channel number=" << request.mTurnChannelNumber << ".  Sending 400.");
         buildErrorResponse(response, 400, "Bad Request - channel number out of range");   
         return RespondFromReceiving;
      }

      StunTuple remoteAddress;
      remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
      // Shouldn't have more than one xor-peer-address attribute in this request
      StunMessage::setTupleFromStunAtrAddress(remoteAddress, request.mTurnXorPeerAddress[0]);

      if(!allocation->addChannelBinding(remoteAddress, request.mTurnChannelNumber))
      {
         WarningLog(<< "Channel bind request invalid.  Sending 400.");
         buildErrorResponse(response, 400, "Bad Request");   
         return RespondFromReceiving;
      }
   }
   else
   {
      WarningLog(<< "Channel bind request missing peer address and/or channel number.  Sending 400.");
      buildErrorResponse(response, 400, "Bad Request - missing attribute");   
      return RespondFromReceiving;
   }

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Add the channel number to make the clients job easier
   //response.mHasTurnChannelNumber = true;
   //response.mTurnChannelNumber = request.mTurnChannelNumber;

   // Note: Message Integrity added by handleAuthentication

   return RespondFromReceiving;
}

void
RequestHandler::processTurnSendIndication(StunMessage& request)
{
   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn send indication for non-existing allocation.  Dropping.");
      return;
   }

   if(request.mCntTurnXorPeerAddress == 0 || !request.mHasTurnData)
   {
      WarningLog(<< "Turn send indication with no peer address or data.  Dropping.");
      return;
   }

   // Check if Don't Fragment attribute is present - if so drop - TODO implement DF bit, then remove this check
   if(request.mHasTurnDontFragment)
   {
      WarningLog(<< "Turn send indication with Don't Fragment requested, not yet implemented.  Dropping.");
      return;
   }

   StunTuple remoteAddress;
   remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
   // Shouldn't have more than one xor-peer-address attribute in this request
   StunMessage::setTupleFromStunAtrAddress(remoteAddress, request.mTurnXorPeerAddress[0]);

   // Check if permission exists, if not then drop
   if(!allocation->existsPermission(remoteAddress.getAddress()))
   {
      WarningLog(<< "Turn send indication for destination=" << remoteAddress << ", but no permission installed.  Dropping.");
      return;
   }

   boost::shared_ptr<DataBuffer> data(new DataBuffer(request.mTurnData->data(), request.mTurnData->size()));
   allocation->sendDataToPeer(remoteAddress, data, false /* isFramed? */);
}

void 
RequestHandler::processTurnData(unsigned short channelNumber, const StunTuple& localTuple, const StunTuple& remoteTuple, boost::shared_ptr<DataBuffer>& data)
{
   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(localTuple, remoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn data for non existing allocation.  Dropping.");
      return;
   }

   allocation->sendDataToPeer(channelNumber, data, true /* isFramed? */);
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
