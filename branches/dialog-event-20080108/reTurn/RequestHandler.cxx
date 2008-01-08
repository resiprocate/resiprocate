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

// !slg! these need to be made into settings
//RequestHandler::AuthenticationMode authenticationMode = RequestHandler::NoAuthentication;
//RequestHandler::AuthenticationMode authenticationMode = RequestHandler::ShortTermPassword;
RequestHandler::AuthenticationMode authenticationMode = RequestHandler::LongTermPassword;
const char authenticationRealm[] = "test";
const char authenticationUsername[] = "test";
const char authenticationPassword[] = "1234";

#define NONCE_LIFETIME 3600    // 1 hour - ?slg? what do we want as a default here? 
//#define NONCE_LIFETIME 10    // for TESTING
#define SERVER_STRING "reTURN 0.1"
#define DEFAULT_LIFETIME 600   // 10 minutes
//#define DEFAULT_LIFETIME 30  // for TESTING
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

         case StunMessage::TurnChannelConfirmationMethod:
            processTurnChannelConfirmationIndication(request);
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

   if(result != NoResponseToSend)
   {
      // Fill in common response properties
      response.mMethod = request.mMethod;

      // Copy over TransactionId
      response.mHeader.magicCookieAndTid = request.mHeader.magicCookieAndTid;

      if (1) // add ServerName - could be a setting in the future
      {
         response.setServer(SERVER_STRING);
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
   if((now-creationTime) <= NONCE_LIFETIME)
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
   // Don't authenticate shared secret requests, or Indications
   if((request.mClass == StunMessage::StunClassRequest && request.mMethod == StunMessage::SharedSecretMethod) ||
      (request.mClass == StunMessage::StunClassIndication))
   {
      return true;
   }

   if (!request.mHasMessageIntegrity)
   {
      if (authenticationMode == ShortTermPassword) 
      {
         InfoLog(<< "Received Request with no Message Integrity. Sending 400.");
         buildErrorResponse(response, 400, "Bad Request (no MessageIntegrity)");  
         return false;
      }
      else if(authenticationMode == LongTermPassword)
      {
         InfoLog(<< "Received Request with no Message Integrity. Sending 401.");
         buildErrorResponse(response, 401, "Unauthorized (no MessageIntegrity)", authenticationRealm);  
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

      if(authenticationMode == LongTermPassword)  
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
            buildErrorResponse(response, 438, "Stale Nonce", authenticationRealm);
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

      if(authenticationMode == ShortTermPassword)
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
      if (authenticationMode == LongTermPassword && strcmp(request.mUsername->c_str(), authenticationUsername) != 0)
      {
         WarningLog(<< "Invalid username: " << *request.mUsername << ". Sending 401.");
         buildErrorResponse(response, 401, "Unathorized", authenticationMode == LongTermPassword ? authenticationRealm : 0);
         return false;
      }

      DebugLog(<< "Validating MessageIntegrity");

      // Need to calculate HMAC across entire message - for ShortTermAuthentication we use the password
      // as the key - for LongTermAuthentication we use username:realm:password string as the key
      Data hmacKey;
      assert(request.mHasUsername);

      if(authenticationMode != NoAuthentication)
      {
         request.calculateHmacKey(hmacKey, authenticationPassword);
      }
      
      if(!request.checkMessageIntegrity(hmacKey))
      {
         WarningLog(<< "MessageIntegrity is bad. Sending 401.");
         buildErrorResponse(response, 401, "Unauthorized", authenticationMode == LongTermPassword ? authenticationRealm : 0);
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

   // TODO should check for unknown attributes here and send 420 listing the
   // unknown attributes. 

   // form the outgoing message
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Add XOrMappedAddress to response - we do this even for 3489 endpoints - since some support it, others should just ignore this attribute
   response.mHasXorMappedAddress = true;
   StunMessage::setStunAtrAddressFromTuple(response.mXorMappedAddress, request.mRemoteTuple);

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

      // Add Mapped address to response
      response.mHasMappedAddress = true;
      StunMessage::setStunAtrAddressFromTuple(response.mMappedAddress, request.mRemoteTuple);

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
      buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, authenticationPassword);

   DebugLog(<< "Allocation request received: localTuple=" << request.mLocalTuple << ", remoteTuple=" << request.mRemoteTuple);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   // If this is a subsequent allocation request, return error
   if(allocation)
   {
      WarningLog(<< "Allocation requested but already exists.  Sending 437.");
      buildErrorResponse(response, 437, "Allocation Mismatch");  
      return RespondFromReceiving;
   }

   // TODO - add a check that a per-user quota for number of allowed TurnAllocations has not been exceeded

   // Check if bandwidth is available
   if(request.mHasTurnBandwidth)
   {
      // TODO - bandwidth check
   }

   // Build the Allocation Tuple
   StunTuple allocationTuple(request.mLocalTuple.getTransportType(), // Default to receiving transport
                             request.mLocalTuple.getAddress(),   // default ip address is at the discretion of the server
                             0);  // port to be populated later
                                
   // Check for requested properties
   if(request.mHasTurnRequestedTransport)
   {
      if(request.mTurnRequestedTransport == StunMessage::RequestedTransportTcp && 
         request.mLocalTuple.getTransportType() == StunTuple::UDP)  // Don't allow UDP traffic to request TCP transport
      {
         WarningLog(<< "Invalid transport requested.  Sending 442.");
         buildErrorResponse(response, 442, "Unsupported Transport Protocol");  
         return RespondFromReceiving;
      }
      allocationTuple.setTransportType(request.mTurnRequestedTransport == StunMessage::RequestedTransportTcp ? StunTuple::TCP : StunTuple::UDP);
   }
   if(request.mHasTurnRequestedIp)
   {
      StunMessage::setTupleFromStunAtrAddress(allocationTuple, request.mTurnRequestedIp);

      // Validate that requested interface is valid
      if(allocationTuple.getAddress() != request.mLocalTuple.getAddress())  // TODO - for now only allow receiving interface
      {
         WarningLog(<< "Invalid ip address requested.  Sending 443.");
         buildErrorResponse(response, 443, "Invalid IP Address");  
         return RespondFromReceiving;
      }
   }
   unsigned short port = 0;  // Default to Any free port
   UInt8 portprops = StunMessage::PortPropsNone;
   if(request.mHasTurnRequestedPortProps)
   {
      port = request.mTurnRequestedPortProps.port;
      portprops = request.mTurnRequestedPortProps.props;
   }

   // If there is a specific requested port - then check if it is allocated
   if(port != 0)
   {
      // Note:  Port could be reserved temporarily as an adjacent pair of ports
      if(!mTurnManager.allocatePort(allocationTuple.getTransportType(), port))
      {
         // If the specific port is not available deny request
         port = 0;
      }
   }
   else
   {
      if(portprops == StunMessage::PortPropsOdd)
      {
         // Attempt to allocate an odd port
         port = mTurnManager.allocateOddPort(allocationTuple.getTransportType());
      }
      else if(portprops == StunMessage::PortPropsEven)
      {
         // Attempt to allocate an even port
         port = mTurnManager.allocateEvenPort(allocationTuple.getTransportType());
      }
      else if(portprops == StunMessage::PortPropsEvenPair)
      {
         // Attempt to allocate an even port, with a free adjacent odd port
         port = mTurnManager.allocateEvenPortPair(allocationTuple.getTransportType());
      }
      else
      {
         // Allocate any available port
         port = mTurnManager.allocateAnyPort(allocationTuple.getTransportType());
      }
   }

   // If finding a port failed - send error
   if(port < 1024)
   {
      WarningLog(<< "Can't allocate port.  Sending 444.");
      buildErrorResponse(response, 444, "Invalid Port");  
      return RespondFromReceiving;
   }

   allocationTuple.setPort(port);

   // We now have an internal 5-Tuple and an allocation tuple - create the allocation

   try
   {
      allocation = new TurnAllocation(mTurnManager,
                                      turnSocket, 
                                      request.mLocalTuple, 
                                      request.mRemoteTuple, 
                                      StunAuth(*request.mUsername, hmacKey), 
                                      allocationTuple, 
                                      request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME,  
                                      request.mHasTurnRequestedPortProps ? request.mTurnRequestedPortProps.port : 0,  // should we pass in actual port requested too?
                                      request.mHasTurnRequestedTransport ? request.mTurnRequestedTransport : StunTuple::None,
                                      request.mHasTurnRequestedIp ? (asio::ip::address*)&allocationTuple.getAddress() : 0);
   }
   catch(asio::system_error e)
   {
      ErrLog(<< "Error allocating socket for allocation.  Sending 500.");
      buildErrorResponse(response, 500, "Server Error");  
      return RespondFromReceiving;
   }

   // Add the new allocation to be managed
   mTurnManager.addTurnAllocation(allocation);

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   response.mHasTurnLifetime = true;
   response.mTurnLifetime = request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME;

   response.mHasTurnRelayAddress = true;
   StunMessage::setStunAtrAddressFromTuple(response.mTurnRelayAddress, allocation->getRequestedTuple());

   // Note:  XorMappedAddress is added to all TurnAllocate responses in processStunMessage
  
   response.mHasTurnBandwidth = true;
   response.mTurnBandwidth = request.mHasTurnBandwidth ? request.mTurnBandwidth : DEFAULT_BANDWIDTH;

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
      buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, authenticationPassword);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Refresh requested with non-matching allocation.  Sending 437."); 
      buildErrorResponse(response, 437, "Allocation Mismatch");  
      return RespondFromReceiving;
   }

   // If allocation was found, then ensure that the same shared secret was used
   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      WarningLog(<< "Refresh requested with username not matching allocation.  Sending 436.");
      buildErrorResponse(response, 436, "Unknown Username");  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      WarningLog(<< "Refresh requested with shared secret not matching allocation.  Sending 431.");
      buildErrorResponse(response, 431, "Integrity Check Failure");   
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

   // Check if this is a subsequent allocate request 
   allocation->refresh(request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME);

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   response.mHasTurnLifetime = true;
   response.mTurnLifetime = request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME;
  
   response.mHasTurnBandwidth = true;
   response.mTurnBandwidth = request.mHasTurnBandwidth ? request.mTurnBandwidth : DEFAULT_BANDWIDTH;

   // Note: Message Integrity added by handleAuthentication

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnListenPermissionRequest(StunMessage& request, StunMessage& response)
{
   // Turn Allocate requests must be authenticated
   if(!request.mHasMessageIntegrity)
   {
      WarningLog(<< "Turn listen permission request without authentication.  Send 401.");
      buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, authenticationPassword);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn listen permission request for non-existing allocation.  Send 437."); 
      buildErrorResponse(response, 437, "No Allocation");  
      return RespondFromReceiving;
   }

   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      WarningLog(<< "Listen permission requested with username not matching allocation.  Sending 436.");
      buildErrorResponse(response, 436, "Unknown Username");  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      WarningLog(<< "Listen permission requested with shared secret not matching allocation.  Sending 431.");
      buildErrorResponse(response, 431, "Integrity Check Failure");   
      return RespondFromReceiving;
   }

   response.mClass = StunMessage::StunClassSuccessResponse;

   if(request.mHasTurnPeerAddress)
   {
      // !SLG! TODO
   }
   else
   {
      // !SLG! send error - no remote address
   }

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

   if(!request.mHasTurnChannelNumber)
   {
      WarningLog(<< "Turn send indication with no channel number.  Dropping.");
      return;
   }

   if(!request.mHasTurnPeerAddress)
   {
      WarningLog(<< "Turn send indication with no remote address.  Dropping.");
      return;
   }

   StunTuple remoteAddress;
   remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
   StunMessage::setTupleFromStunAtrAddress(remoteAddress, request.mTurnPeerAddress);

   if(request.mHasTurnData)  
   {
      boost::shared_ptr<DataBuffer> data(new DataBuffer(request.mTurnData->data(), request.mTurnData->size()));
      allocation->sendDataToPeer(request.mTurnChannelNumber, remoteAddress, data, false /* framed? */);
   }
   else
   {
      boost::shared_ptr<DataBuffer> empty(new DataBuffer((const char*)0, 0));
      allocation->sendDataToPeer(request.mTurnChannelNumber, remoteAddress, empty, false /* framed? */);
   }
}

void
RequestHandler::processTurnChannelConfirmationIndication(StunMessage& request)
{
   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn channel confirmation for non-existing allocation.  Dropping.");
      return;
   }

   if(!request.mHasTurnChannelNumber)
   {
      WarningLog(<< "Turn channel confirmation indication with no channel number.  Dropping.");
      return;
   }

   if(!request.mHasTurnPeerAddress)
   {
      WarningLog(<< "Turn channel confirmation indication with no remote address.  Dropping.");
      return;
   }

   StunTuple remoteAddress;
   remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
   StunMessage::setTupleFromStunAtrAddress(remoteAddress, request.mTurnPeerAddress);

   allocation->serverToClientChannelConfirmed(request.mTurnChannelNumber, remoteAddress);
}

void 
RequestHandler::processTurnData(unsigned short channelNumber, const StunTuple& localTuple, const StunTuple& remoteTuple, boost::shared_ptr<DataBuffer> data)
{
   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(localTuple, remoteTuple));

   if(!allocation)
   {
      WarningLog(<< "Turn data for non existing allocation.  Dropping.");
      return;
   }

   allocation->sendDataToPeer(channelNumber, data, true /* framed? */);
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

