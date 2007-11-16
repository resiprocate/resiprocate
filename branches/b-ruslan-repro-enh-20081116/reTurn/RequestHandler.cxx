#include "RequestHandler.hxx"
#include <fstream>
#include <sstream>
#include <string>

#include "TurnAllocation.hxx"
#include "TurnTransportBase.hxx"
#include "StunAuth.hxx"

using namespace std;
using namespace resip;

namespace reTurn {

// !slg! these need to be made into settings
RequestHandler::AuthenticationMode authenticationMode = RequestHandler::NoAuthentication;
//RequestHandler::AuthenticationMode authenticationMode = RequestHandler::ShortTermPassword;
const char authenticationRealm[] = "test";
const char authenticationUsername[] = "test";
const char authenticationPassword[] = "1234";

#define DEFAULT_LIFETIME 600   // 10 minutes
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
}

RequestHandler::ProcessResult 
RequestHandler::processStunMessage(TurnTransportBase* turnTransport, StunMessage& request, StunMessage& response)
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
            result = processStunBindingRequest(request, response);
            break;
         case StunMessage::SharedSecretMethod:
            result = processStunSharedSecretRequest(request, response);
            break;
         case StunMessage::TurnAllocateMethod:
            result = processTurnAllocateRequest(turnTransport, request, response);
            if(result != NoResponseToSend)
            {
               // Add an XOrMappedAddress to all TurnAllocateResponses
               response.mHasXorMappedAddress = true;
               response.mXorMappedAddress.port = request.mRemoteTuple.getPort();
               if(request.mRemoteTuple.getAddress().is_v6())
               {
                  response.mXorMappedAddress.family = StunMessage::IPv6Family;  
                  memcpy(&response.mXorMappedAddress.addr.ipv6, request.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mXorMappedAddress.addr.ipv6));
               }
               else
               {
                  response.mXorMappedAddress.family = StunMessage::IPv4Family;  
                  response.mXorMappedAddress.addr.ipv4 = request.mRemoteTuple.getAddress().to_v4().to_ulong();   
               }
            }
            break;
         case StunMessage::TurnConnectMethod:
            // TODO
            buildErrorResponse(response, 400, "Invalid Request Method - not implemented yet");  
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
         case StunMessage::TurnConnectStatusMethod:  
         default:
            // Unknown indication - just ignore
            break;
         }
   
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
         response.setServer("reTURN 0.1");
      }

      // Ensure fingerprint is added
      response.mHasFingerprint = true;
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

      // !slg! Need to add a random nonce value that is expirable
      response.setNonce("some random value - TODO");
   }
}

bool 
RequestHandler::handleAuthentication(StunMessage& request, StunMessage& response)
{
   bool verbose = false;

   // !slg! could add here to lookup transactionId - if this is a retransmission of a previous request, then just resend last response
   // for binding requests it's OK to just re-calculate the response - just do that for now

   // Don't authenticate shared secret requests, or Send/Data Indications
   if((request.mClass == StunMessage::StunClassRequest && request.mMethod == StunMessage::SharedSecretMethod) ||
      (request.mClass == StunMessage::StunClassIndication && 
       (request.mMethod == StunMessage::TurnSendMethod ||
        request.mMethod == StunMessage::TurnDataMethod)))
   {
      return true;
   }

   if (!request.mHasMessageIntegrity)
   {
      if (verbose) clog << "Request does not contain MessageIntegrity" << endl;

      if (authenticationMode != NoAuthentication) 
      {
         // !slg! if we want a long term credential to be provided, we need to add a realm to the response, otherwise 
         // we are requesting that short term credentials be provided
         if(verbose) clog << "Received Request with no Message Integrity. Sending 401." << endl;
         buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
         return false;
      }
   }
   else
   {
      if (!request.mHasUsername)
      {
         if (verbose) clog << "No UserName. Send 432." << endl;
         buildErrorResponse(response, 432, "No UserName and contains MessageIntegrity", authenticationMode == LongTermPassword ? authenticationRealm : 0);
         return false;
      }

      if(request.mHasRealm && authenticationMode == LongTermPassword)  
      {
         if(!request.mHasNonce)
         {
            if (verbose) clog << "No Nonce. Send 435." << endl;
            buildErrorResponse(response, 435, "No Nonce and contains Realm", authenticationMode == LongTermPassword ? authenticationRealm : 0);
            return false;
         }
         // !slg! Need to check if nonce expired
         // !slg! we may want to delay this check for Turn Allocations so that expired 
         //       authentications can still be accepted for existing allocation refreshes 
         //       and removals
         if(0)
         {
            if (verbose) clog << "Nonce Expired. Send 438." << endl;
            buildErrorResponse(response, 438, "Stale Nonce" /* Send realm? ,authenticationMode == LongTermPassword ? authenticationRealm : 0 */);
            return false;
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
            if (verbose) clog << "Username Expired. Send 430." << endl;
            buildErrorResponse(response, 430, "Stale Credentials");
            return false;
         }
      }

      if (verbose) clog << "Validating username: " << *request.mUsername << endl;

      // !slg! need to determine whether the USERNAME contains a known entity, and in the case of a long-term
      //       credential, known within the realm of the REALM attribute of the request
      if (authenticationMode == LongTermPassword && strcmp(request.mUsername->c_str(), authenticationUsername) != 0)
      {
         if (verbose) clog << "Invalid username: " << *request.mUsername << "Send 436." << endl; 
         buildErrorResponse(response, 436, "Unknown username.", authenticationMode == LongTermPassword ? authenticationRealm : 0);
         return false;
      }

      if (verbose) clog << "Validating MessageIntegrity" << endl;

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
         if (verbose) clog << "MessageIntegrity is bad. Sending 431." << endl;
         buildErrorResponse(response, 431, "Integrty Check Failure", authenticationMode == LongTermPassword ? authenticationRealm : 0);
         return false;
      }

      // need to compute this later after message is filled in
      response.mHasMessageIntegrity = true;
      response.mHmacKey = hmacKey;  // Used to later calculate Message Integrity during encoding

      // copy username into response
      assert(request.mHasUsername);
      response.setUsername(request.mUsername->c_str()); 

      // !slg! bis06 is missing this, but I think it makes sense
      if (request.mHasRealm && !request.mRealm->empty()) 
      {
         response.setRealm(request.mRealm->c_str());
      }
   }

   return true;
}

RequestHandler::ProcessResult 
RequestHandler::processStunBindingRequest(StunMessage& request, StunMessage& response)
{
   ProcessResult result = RespondFromReceiving;
   //bool verbose = true;

   // TODO should check for unknown attributes here and send 420 listing the
   // unknown attributes. 

   // form the outgoing message
   response.mClass = StunMessage::StunClassSuccessResponse;

   // Add XOrMappedAddress to response - we do this even for 3489 endpoints - since some support it, others should just ignore this attribute
   response.mHasXorMappedAddress = true;
   response.mXorMappedAddress.port = request.mRemoteTuple.getPort();
   if(request.mRemoteTuple.getAddress().is_v6())
   {
      response.mXorMappedAddress.family = StunMessage::IPv6Family;  
      memcpy(&response.mXorMappedAddress.addr.ipv6, request.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mXorMappedAddress.addr.ipv6));
   }
   else
   {
      response.mXorMappedAddress.family = StunMessage::IPv4Family;  
      response.mXorMappedAddress.addr.ipv4 = request.mRemoteTuple.getAddress().to_v4().to_ulong();   
   }

   // the following code is for RFC3489 backward compatibility
   if(mRFC3489SupportEnabled)
   {
      asio::ip::address sendFromAddress;
      unsigned short sendFromPort;
      asio::ip::address changeAddress = request.mLocalTuple.getAddress() == mPrim3489Address ? mAlt3489Address : mPrim3489Address;
      unsigned short changePort = request.mLocalTuple.getPort() == mPrim3489Port ? mAlt3489Port : mPrim3489Port;
      UInt32 changeRequest = request.mHasChangeRequest ? request.mChangeRequest : 0;
      if(changeRequest & StunMessage::ChangeIpFlag && changeRequest & StunMessage::ChangePortFlag)
      {
         result = RespondFromAlternateIpPort;
         sendFromAddress = changeAddress;
         sendFromPort = changePort;
      }
      else if(changeRequest & StunMessage::ChangePortFlag)
      {
         result = RespondFromAlternatePort;
         sendFromAddress = request.mLocalTuple.getAddress();
         sendFromPort = changePort;
      }
      else if(changeRequest & StunMessage::ChangeIpFlag)
      {
         result = RespondFromAlternateIp;
         sendFromAddress = changeAddress;
         sendFromPort = request.mLocalTuple.getPort();
      }
      else
      {
         // default to send from receiving transport
         sendFromAddress = request.mLocalTuple.getAddress();
         sendFromPort = request.mLocalTuple.getPort();
      }

      // Add Mapped address to response
      response.mHasMappedAddress = true;
      response.mMappedAddress.port = request.mRemoteTuple.getPort();
      if(request.mRemoteTuple.getAddress().is_v6())
      {
         response.mMappedAddress.family = StunMessage::IPv6Family;  
         memcpy(&response.mMappedAddress.addr.ipv6, request.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mMappedAddress.addr.ipv6));
      }
      else
      {
         response.mMappedAddress.family = StunMessage::IPv4Family;  
         response.mMappedAddress.addr.ipv4 = request.mRemoteTuple.getAddress().to_v4().to_ulong();   
      }

      // Add source address - for RFC3489 backwards compatibility
      response.mHasSourceAddress = true;
      response.mSourceAddress.port = sendFromPort;
      if(sendFromAddress.is_v6())
      {
         response.mSourceAddress.family = StunMessage::IPv6Family;  
         memcpy(&response.mSourceAddress.addr.ipv6, sendFromAddress.to_v6().to_bytes().c_array(), sizeof(response.mSourceAddress.addr.ipv6));
      }
      else
      {
         response.mSourceAddress.family = StunMessage::IPv4Family;
         response.mSourceAddress.addr.ipv4 = sendFromAddress.to_v4().to_ulong();
      }

      // Add changed address - for RFC3489 backward compatibility
      response.mHasChangedAddress = true;
      response.mChangedAddress.port = changePort;
      if(changeAddress.is_v6())
      {
         response.mChangedAddress.family = StunMessage::IPv6Family; 
         memcpy(&response.mChangedAddress.addr.ipv6, changeAddress.to_v6().to_bytes().c_array(), sizeof(response.mChangedAddress.addr.ipv6));
      }
      else
      {
         response.mChangedAddress.family = StunMessage::IPv4Family; 
         response.mChangedAddress.addr.ipv4 = changeAddress.to_v4().to_ulong();  
      }

      // If Response-Address is present, then response should be sent here instead of source address to be
      // compliant with RFC3489.  In this case we need to add a REFLECTED-FROM attribute
      if(request.mHasResponseAddress)
      {
         response.mRemoteTuple.setPort(request.mResponseAddress.port);
         if(request.mResponseAddress.family == StunMessage::IPv6Family)
         {
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &request.mResponseAddress.addr.ipv6, bytes.size());
            asio::ip::address_v6 addr(bytes);
            response.mRemoteTuple.setAddress(addr);
         }
         else
         {
            asio::ip::address_v4 addr(request.mResponseAddress.addr.ipv4);
            response.mRemoteTuple.setAddress(addr);
         }

         // If the username is present and is greater than or equal to 92 bytes long, then we assume this username
         // was obtained from a shared secret request
         if (request.mHasUsername && (request.mUsername->size() >= 92 ) )
         {
            // Extract source address that sent the shared secret request from the encoded username and use this in response address
            asio::ip::address responseAddress;
            unsigned int responsePort;
            request.getAddressFromUsername(responseAddress, responsePort);

            response.mHasReflectedFrom = true;
            response.mReflectedFrom.port = responsePort;  
            if(responseAddress.is_v6())
            {
               response.mReflectedFrom.family = StunMessage::IPv6Family;  
               memcpy(&response.mReflectedFrom.addr.ipv6, responseAddress.to_v6().to_bytes().c_array(), sizeof(response.mReflectedFrom.addr.ipv6));
            }
            else
            {
               response.mReflectedFrom.family = StunMessage::IPv4Family;  
               response.mReflectedFrom.addr.ipv4 = responseAddress.to_v4().to_ulong();
            }
         }
         else
         {
            response.mHasReflectedFrom = true;
            response.mReflectedFrom.port = request.mRemoteTuple.getPort();
            if(request.mRemoteTuple.getAddress().is_v6())
            {
               response.mReflectedFrom.family = StunMessage::IPv6Family;  
               memcpy(&response.mReflectedFrom.addr.ipv6, request.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mReflectedFrom.addr.ipv6));
            }
            else
            {
               response.mReflectedFrom.family = StunMessage::IPv4Family;  
               response.mReflectedFrom.addr.ipv4 = request.mRemoteTuple.getAddress().to_v4().to_ulong();
            }
         }
      }
   }

   return result;
}

RequestHandler::ProcessResult 
RequestHandler::processStunSharedSecretRequest(StunMessage& request, StunMessage& response)
{
   bool verbose = true;

   // Only allow shared secret requests on TLS transport
   if(request.mLocalTuple.getTransportType() != StunTuple::TLS)
   {
      if (verbose) clog << "Invalid transport for shared secret request.  Send 436." << endl; 
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
RequestHandler::processTurnAllocateRequest(TurnTransportBase* turnTransport, StunMessage& request, StunMessage& response)
{
   bool verbose = true;

   // Turn Allocate requests must be authenticated (note: if this attribute is present 
   // then handleAuthentication would have validation authentication info)
   if(!request.mHasMessageIntegrity)
   {
      if (verbose) clog << "Turn allocate request without authentication.  Send 436." << endl; 
      buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, authenticationPassword);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   // If this is a subsequent request, then ensure that the same shared secret was used
   if(allocation)
   {
      if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
      {
         if (verbose) clog << "Subsequent allocate requested with non-matching username.  Send 436." << endl; 
         buildErrorResponse(response, 436, "Unknown Username", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
         return RespondFromReceiving;
      }
      if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
      {
         if (verbose) clog << "Subsequent allocate requested with non-matching shared secret.  Send 431." << endl; 
         buildErrorResponse(response, 431, "Integrity Check Failure", authenticationMode == LongTermPassword ? authenticationRealm : 0 );   
         return RespondFromReceiving;
      }
   }

   // check if Lifetime is 0, if so then just send success response
   if(request.mHasTurnLifetime && request.mTurnLifetime == 0)
   {
      // form the outgoing success response
      response.mClass = StunMessage::StunClassSuccessResponse;

      response.mHasTurnLifetime = true;
      response.mTurnLifetime = 0;

      // Note:  XorMappedAddress is added to all TurnAllocate responses in processStunMessage
  
      // If allocation exists then delete it
      if(allocation)
      {
         mTurnManager.removeTurnAllocation(allocation->getKey());  // will delete allocation
      }

      return RespondFromReceiving;
   }

   // Check if this is a subsequent allocate request 
   if(allocation)
   {
      // TODO - If the allocated transport address given out previously to the client still matches the 
      //        contraints in the request (in terms of request ports, IP address and transport
      //        protocols), the same allocation previously granted must be returned
      if(1)
      {
         allocation->refresh(request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME);

         // fallthrough and send success response
      }
      else
      {
         // free previous allocation and create new one
         mTurnManager.removeTurnAllocation(allocation->getKey());  // will delete allocation
         allocation = 0;  // will cause new allocation to be created
      }
   }

   if(!allocation) // this is a new allocation request
   {
      bool allocationSubsumed = false;
      // Logic to allow one allocation to subsume an previous one.  For example, my IP address 
      // changes.  I send an AllocateRequest with the requested IP and port that I used before and 
      // the same credentials.  My new allocation takes over the port I had been using previously.  
      // Note that this check should happen before checking if bandwidth is available.
      if(request.mHasTurnRequestedIp && request.mHasTurnRequestedPortProps && request.mTurnRequestedPortProps.port != 0)
      {
         asio::ip::address requestedAddress;
         if(request.mTurnRequestedIp.family == StunMessage::IPv6Family)
         {            
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &request.mTurnRequestedIp.addr.ipv6, bytes.size());
            requestedAddress = asio::ip::address_v6(bytes);
         }
         else
         {
            requestedAddress = asio::ip::address_v4(request.mTurnRequestedIp.addr.ipv4);
         }

         StunTuple allocationTuple(request.mHasTurnRequestedTransport ? 
                                  (request.mTurnRequestedTransport == StunMessage::RequestedTransportTcp ? StunTuple::TCP : StunTuple::UDP) :
                                   request.mLocalTuple.getTransportType(), // use receiving transport if not specified
                                   requestedAddress,  
                                   request.mTurnRequestedPortProps.port); 

         // Try to find an existing allocation with these properties - if we find one and the credentials match, then old
         // allocation is removed and new allocation is created.
         allocation = mTurnManager.findTurnAllocation(allocationTuple);
         if(allocation && allocation->getClientAuth().getClientUsername() == *request.mUsername &&
                          allocation->getClientAuth().getClientSharedSecret() == hmacKey)
         {
            allocationSubsumed = true;

            // ?slg? should we be copying any properties from the old allocation - ie. current active destination or permissions?

            // free previous allocation and create new one
            mTurnManager.removeTurnAllocation(allocation->getKey());  // will delete allocation
            allocation = 0;  // will cause new allocation to be created
         }
      }

      if(!allocationSubsumed)
      {
         // TODO - This is a new allocation - If we delayed the nonce or username exirey check from handleAuthentication it should be checked now

         // TODO - add a check that a per-user quota for number of allowed TurnAllocations has not been exceeded

         // Check if bandwidth is available
         if(request.mHasTurnBandwidth)
         {
            // TODO - bandwidth check
         }
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
            if (verbose) clog << "Invalid transport requested.  Send 442." << endl; 
            buildErrorResponse(response, 442, "Unsupported Transport Protocol", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
            return RespondFromReceiving;
         }
         allocationTuple.setTransportType(request.mTurnRequestedTransport == StunMessage::RequestedTransportTcp ? StunTuple::TCP : StunTuple::UDP);
      }
      if(request.mHasTurnRequestedIp)
      {
         if(request.mTurnRequestedIp.family == StunMessage::IPv6Family)
         {            
            asio::ip::address_v6::bytes_type bytes;
            memcpy(bytes.c_array(), &request.mTurnRequestedIp.addr.ipv6, bytes.size());
            asio::ip::address_v6 addr(bytes);
            allocationTuple.setAddress(addr);
         }
         else
         {
            allocationTuple.setAddress(asio::ip::address_v4(request.mTurnRequestedIp.addr.ipv4));
         }
         // Validate that requested interface is valid
         if(allocationTuple.getAddress() != request.mLocalTuple.getAddress())  // TODO - for now only allow receiving interface
         {
            if (verbose) clog << "Invalid ip address requested.  Send 443." << endl; 
            buildErrorResponse(response, 443, "Invalid IP Address", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
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
         if (verbose) clog << "Can't allocate port.  Send 444." << endl; 
         buildErrorResponse(response, 444, "Invalid Port", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
         return RespondFromReceiving;
      }

      allocationTuple.setPort(port);

      // We now have an internal 5-Tuple and an allocation tuple - create the allocation

      try
      {
         allocation = new TurnAllocation(mTurnManager,
                                         turnTransport, 
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
         if (verbose) clog << "Error allocating socket.  Send 500." << endl; 
         buildErrorResponse(response, 500, "Server Error", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
         return RespondFromReceiving;
      }

      mTurnManager.addTurnAllocation(allocation);
   }

   // form the outgoing success response
   response.mClass = StunMessage::StunClassSuccessResponse;

   response.mHasTurnLifetime = true;
   response.mTurnLifetime = request.mHasTurnLifetime ? request.mTurnLifetime : DEFAULT_LIFETIME;

   response.mHasTurnRelayAddress = true;
   response.mTurnRelayAddress.port = allocation->getRequestedTuple().getPort();
   if(allocation->getRequestedTuple().getAddress().is_v6())
   {
      response.mTurnRelayAddress.family = StunMessage::IPv6Family;  
      memcpy(&response.mTurnRelayAddress.addr.ipv6, allocation->getRequestedTuple().getAddress().to_v6().to_bytes().c_array(), sizeof(response.mTurnRelayAddress.addr.ipv6));
   }
   else
   {
      response.mTurnRelayAddress.family = StunMessage::IPv4Family;  
      response.mTurnRelayAddress.addr.ipv4 = allocation->getRequestedTuple().getAddress().to_v4().to_ulong();   
   }

   response.mHasXorMappedAddress = true;
   response.mXorMappedAddress.port = request.mRemoteTuple.getPort();
   if(request.mRemoteTuple.getAddress().is_v6())
   {
      response.mXorMappedAddress.family = StunMessage::IPv6Family;  
      memcpy(&response.mXorMappedAddress.addr.ipv6, request.mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), sizeof(response.mXorMappedAddress.addr.ipv6));
   }
   else
   {
      response.mXorMappedAddress.family = StunMessage::IPv4Family;  
      response.mXorMappedAddress.addr.ipv4 = request.mRemoteTuple.getAddress().to_v4().to_ulong();   
   }
  
   response.mHasTurnBandwidth = true;
   response.mTurnBandwidth = request.mHasTurnBandwidth ? request.mTurnBandwidth : DEFAULT_BANDWIDTH;

   // Note: Message Integrity added by handleAuthentication, and fingerprint is added by default

   return RespondFromReceiving;
}

RequestHandler::ProcessResult 
RequestHandler::processTurnListenPermissionRequest(StunMessage& request, StunMessage& response)
{
   bool verbose = true;

   // Turn Allocate requests must be authenticated
   if(!request.mHasMessageIntegrity)
   {
      if (verbose) clog << "Turn set active desination request without authentication.  Send 436." << endl; 
      buildErrorResponse(response, 401, "Missing Message Integrity", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   Data hmacKey;
   assert(request.mHasUsername);
   request.calculateHmacKey(hmacKey, authenticationPassword);

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      if (verbose) clog << "Turn set active desination request for non-existing binding.  Send 437." << endl; 
      buildErrorResponse(response, 437, "No Binding", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }

   if(allocation->getClientAuth().getClientUsername() != *request.mUsername)
   {
      if (verbose) clog << "Turn set active desination request requested with non-matching username.  Send 436." << endl; 
      buildErrorResponse(response, 436, "Unknown Username", authenticationMode == LongTermPassword ? authenticationRealm : 0 );  
      return RespondFromReceiving;
   }
   if(allocation->getClientAuth().getClientSharedSecret() != hmacKey)
   {
      if (verbose) clog << "Turn set active desination request requested with non-matching shared secret.  Send 431." << endl; 
      buildErrorResponse(response, 431, "Integrity Check Failure", authenticationMode == LongTermPassword ? authenticationRealm : 0 );   
      return RespondFromReceiving;
   }

   response.mClass = StunMessage::StunClassSuccessResponse;

   if(request.mHasTurnRemoteAddress)
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
   bool verbose = true;

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      if (verbose) clog << "Turn send indication for non existing allocation.  Dropping." << endl; 
      return;
   }

   if(!request.mHasTurnChannelNumber)
   {
      if (verbose) clog << "Turn send indication with no channel number.  Dropping." << endl; 
      return;
   }

   if(!request.mHasTurnRemoteAddress)
   {
      if (verbose) clog << "Turn send indication with no remote address.  Dropping." << endl; 
      return;
   }

   StunTuple remoteAddress;
   remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
   remoteAddress.setPort(request.mTurnRemoteAddress.port);
   if(request.mTurnRemoteAddress.family == StunMessage::IPv6Family)
   {            
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), &request.mTurnRemoteAddress.addr.ipv6, bytes.size());
      asio::ip::address_v6 addr(bytes);
      remoteAddress.setAddress(addr);
   }
   else
   {
      remoteAddress.setAddress(asio::ip::address_v4(request.mTurnRemoteAddress.addr.ipv4));
   }

   if(request.mHasTurnData)  
   {
      allocation->sendDataToPeer(request.mTurnChannelNumber, remoteAddress, *request.mTurnData);
   }
   else
   {
      allocation->sendDataToPeer(request.mTurnChannelNumber, remoteAddress, Data::Empty);
   }
}

void
RequestHandler::processTurnChannelConfirmationIndication(StunMessage& request)
{
   bool verbose = true;

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(request.mLocalTuple, request.mRemoteTuple));

   if(!allocation)
   {
      if (verbose) clog << "Turn channel confirmation indication for non existing allocation.  Dropping." << endl; 
      return;
   }

   if(!request.mHasTurnChannelNumber)
   {
      if (verbose) clog << "Turn channel confirmation indication with no channel number.  Dropping." << endl; 
      return;
   }

   if(!request.mHasTurnRemoteAddress)
   {
      if (verbose) clog << "Turn channel confirmation indication with no remote address.  Dropping." << endl; 
      return;
   }

   StunTuple remoteAddress;
   remoteAddress.setTransportType(allocation->getRequestedTuple().getTransportType());
   remoteAddress.setPort(request.mTurnRemoteAddress.port);
   if(request.mTurnRemoteAddress.family == StunMessage::IPv6Family)
   {            
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), &request.mTurnRemoteAddress.addr.ipv6, bytes.size());
      asio::ip::address_v6 addr(bytes);
      remoteAddress.setAddress(addr);
   }
   else
   {
      remoteAddress.setAddress(asio::ip::address_v4(request.mTurnRemoteAddress.addr.ipv4));
   }

   allocation->serverToClientChannelConfirmed(request.mTurnChannelNumber, remoteAddress);
}

void 
RequestHandler::processTurnData(unsigned char channelNumber, const StunTuple& localTuple, const StunTuple& remoteTuple, const char* data, unsigned int size)
{
   bool verbose = true;

   TurnAllocation* allocation = mTurnManager.findTurnAllocation(TurnAllocationKey(localTuple, remoteTuple));

   if(!allocation)
   {
      if (verbose) clog << "Turn data for non existing allocation.  Dropping." << endl; 
      return;
   }

   allocation->sendDataToPeer(channelNumber, Data(Data::Share, data, size));
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

