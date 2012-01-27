/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_HELPER_HXX)
#define RESIP_HELPER_HXX 

#include <time.h>

#include "resip/stack/NonceHelper.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/SdpContents.hxx"

namespace resip
{

class SipMessage;
class NameAddr;
class SecurityAttributes;
class Security;

/**
@brief An exception class that is thrown when an authentication
    scheme that is not implemented on the current platform is
    requested.
@deprecated Unused.    
*/
class UnsupportedAuthenticationScheme : public BaseException
{
   public:
      UnsupportedAuthenticationScheme(const Data& msg, const Data& file, const int line)
         : BaseException(msg, file, line) {}
      
      const char* name() const { return "UnsupportedAuthenticationScheme"; }
};


/**
   @ingroup resip_crit
   @brief An aggregation of useful static functions.

   These are mostly involved with
      - The manufacture of SIP messages (requests and responses). This is of 
      particular importance to app-writers.
      - Digest auth related functions.
*/
class Helper
{
   public:
      /**
      @internal
      @todo bytes in to-tag& from-tag, should prob. live somewhere else
      */
      const static int tagSize;  

      /** 
          @brief Used by Registration, Publication and Subscription refreshes, to
          calculate the time at which a refresh should be performed.
          
          @note Value returned is a time that is a bit smaller than
          the Expiration interval.
          
          @note The recommended calculation from the RFC's is the minimnum of the 
          Exipiration interval less 5 seconds and nine tenths of the exipiration 
          interval.
          
          @note The templated time type must support the following operators (-,*,/,>)
      */
      template<typename T>
      static T aBitSmallerThan(T secs)
      {
         return resipMax(T(0), resipMin(T(secs-5), T(9*secs/10)));
      }

      /** 
          @brief Converts an interger in a character string containing the
          hexidecimal representation of the integer.
          @note The string buffer provided should be at least 8 characters long.

          @note This function will NOT NULL terminate the string.

          @param _d     A pointer to the character buffer of size >= 8 to write
                        the hex string

          @param _s     The integer value to convert.

          @param _l     Boolean flag to include leading 0 zeros or 
                        not.
      */
      static void integer2hex(char* _d, unsigned int _s, bool _l = true);

      /** 
          @brief Converts a character string containing a hexidecimal value
          into an unsigned int.
          @note Parsing stops after the first non-hex character, or after 8
          characters have been processed.

          @param _s     A pointer to the character buffer to convert.

          @returns      The integer value of the coverted hex string.
      */
      static unsigned int hex2integer(const char* _s);

      /**
           @brief Used to jitter the expires in a SUBSCRIBE or 
           REGISTER expires header

           @param input            Value to jitter

           @param lowerPercentage  The lower range of the random percentage by which 
                                   to jitter the value by.

           @param upperPercentage  The upper range of the random percentage by which
                                   to jitter the value by.  Must be greater than or equal 
                                   to lowerPercentage

           @param minimum          Only jitter the input if greater than minimum
       */
      static int jitterValue(int input, int lowerPercentage, int upperPercentage, int minimum=0);

      /**
          @brief Make an invite request.
          
          @details Make an invite request. An empty Contact and Via is added and will be
          populated by the stack when sent.
            
          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header
      */
      static SipMessage* makeInvite(const NameAddr& target, const NameAddr& from);
      
      /**
          @brief Make an invite request using a overridden contact header.
          
          @details Make an invite request using a overridden contact header. 
          An empty Via is added and will be populated by the stack when sent.
            
          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header

          @param contact Ends up in the Contact header.  Stack will not change this
                         when sent.
      */
      static SipMessage* makeInvite(const NameAddr& target, const NameAddr& from, const NameAddr& contact);
      
      /**
          @brief Make a response to the provided information representing a request.
          
          @details Make a response to the provided information representing a request. 
          Adds a To tag, Contact and Record-Route headers appropriately.
          
          @note equivalent to Helper::makeResponse(const SipMessage &,int, const Data &, const Data &, const Data &)
            
          @param response SipMessage populated with the appropriate response

          @param to request's To NameAddr

          @param from request's From NameAddr

          @param callId request's CallID

          @param cseq request's CSeq

          @param vias request's Vias

          @param recordRoutes request's record routes

          @param rfc2543TransactionId request's 2543 transaction ID

          @param isExternal request's isExternal flag

          @param responseCode Response code to use on status line.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.
      */
      static void makeResponse(SipMessage& response,
                               const NameAddr& to,
                               const NameAddr& from,
                               const CallID& callId,
                               const CSeqCategory& cseq,
                               const Vias& vias,
                               const NameAddrs& recordRoutes,
                               const Data& rfc2543TransactionId,
                               bool isExternal,
                               int responseCode, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      /**
          @brief Make a response to the provided information representing a request
          with an overridden Contact.
          
          @details Make a response to the provided information representing a request
            with an overridden Contact. Adds a To tag, Contact and Record-Route
            headers appropriately.
            
          @note equivalent to Helper::makeResponse(const SipMessage &,int, const Data &, const Data &, const Data &)
          
          @param response SipMessage populated with the appropriate response

          @param to request's To NameAddr

          @param from request's From NameAddr

          @param callId request's CallID

          @param cseq request's CSeq

          @param vias request's Vias

          @param recordRoutes request's record routes

          @param rfc2543TransactionId request's 2543 transaction ID

          @param isExternal request's isExternal flag

          @param responseCode Response code to use on status line.

          @param myContact Contact header to add to response.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.
      */
      static void makeResponse(SipMessage& response,
                               const NameAddr& to,
                               const NameAddr& from,
                               const CallID& callId,
                               const CSeqCategory& cseq,
                               const Vias& vias,
                               const NameAddrs& recordRoutes,
                               const Data& rfc2543TransactionId,
                               bool isExternal,
                               int responseCode, 
                               const NameAddr& myContact, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      /**
          @brief Make a response to a provided request.  Adds a To tag, Contact and Record-Route
          headers appropriately.
            
          @param response SipMessage populated with the appropriate response

          @param request  SipMessage request from which to generate the response

          @param responseCode Response code to use on status line.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.
      */
      static void makeResponse(SipMessage& response, 
                               const SipMessage& request, 
                               int responseCode, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      /**
          @brief Transform a provided request into an appropriate response.  
            Adds a To tag, Contact and Record-Route headers appropriately.
            This is considerably faster if you don't need the request any 
            longer.

          @param request  SipMessage request to transform into a response.

          @param responseCode Response code to use on status line.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.
      */
      static void makeInPlaceResponse(SipMessage& request, 
                               int responseCode, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      static void applyAdditionalResponseStuff(SipMessage& response, 
                               int responseCode, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      /**
          @brief Make a response to a provided request with an overridden Contact.  
          Adds a To tag, Contact and Record-Route headers appropriately.
            
          @param response SipMessage populated with the appropriate response

          @param request  SipMessage request from which to generate the response

          @param responseCode Response code to use on status line.

          @param myContact Contact header to add to response.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.
      */
      static void makeResponse(SipMessage& response, 
                               const SipMessage& request, 
                               int responseCode, 
                               const NameAddr& myContact, 
                               const Data& reason = Data::Empty,
                               const Data& hostname = Data::Empty,
                               const Data& warning=Data::Empty);

      /**
          @brief Make a new response to a provided request.  Adds a To tag, Contact and 
          Record-Route headers appropriately.
          
          @note Caller owns the returned pointer and is responsible for deleting it.
            
          @param request  SipMessage request from which to generate the response

          @param responseCode Response code to use on status line.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header

          @returns SipMessage populated with the appropriate response.
                   Caller must deallocate.
      */
      static SipMessage* makeResponse(const SipMessage& request,
                                      int responseCode,
                                      const Data& reason = Data::Empty, 
                                      const Data& hostname = Data::Empty,
                                      const Data& warning=Data::Empty);

      /**
          @brief Make a new response to a provided request with an overridden Contact.  
          Adds a To tag, Contact and Record-Route headers appropriately.
          
          @note Caller owns the returned pointer and is responsible for deleting it.

          @param request  SipMessage request from which to generate the response

          @param responseCode Response code to use on status line.

          @param myContact Contact header to add to response.

          @param reason   Optional reason string to use on status line.  If not provided
                          then a default reason string will be added for well defined
                          response codes.

          @param hostname Optional hostname to use in Warning header.  Only used if
                          warning is also provided.

          @param warning  Optional warning text.  If present a Warning header is added
                          and hostname is used in warning header.

          @returns SipMessage populated with the appropriate response.
                   Caller must deallocate.
      */
      static SipMessage* makeResponse(const SipMessage& request, 
                                      int responseCode, 
                                      const NameAddr& myContact, 
                                      const Data& reason = Data::Empty,
                                      const Data& hostname = Data::Empty,
                                      const Data& warning=Data::Empty);


      /**
          @brief Make a 405 response to a provided request.  Allows header is added
          with specified methods, or with all methods the stack knows about.
          
          @note Caller owns the returned pointer and is responsible for deleting it.

          @param request  SipMessage request from which to generate the response

          @param allowedMethods Array of integers representing a list of Method 
                                Types to add to the generated Allows header. 
                                See MethodTypes.hxx.

          @param nMethods Number of methods specified in the allowedMethods 
                          integer array.  Specify -1 to have this method fill
                          in the Allows header automatically with each method
                          supported by the stack.

          @returns SipMessage populated with the appropriate response.    
                   Caller must deallocate.
      */
      static SipMessage* make405(const SipMessage& request,
                                 const int* allowedMethods = 0,
                                 int nMethods = -1);

      /**
          @brief Returns the default reason string for a particular response code.

          @param responseCode  Response code to get reason for

          @param reason Data where the reason string associated with the 
                   responseCode will be set.
      */
      static void getResponseCodeReason(int responseCode, Data& reason);

      /**
          @brief Make a new request with a overridden Contact.  To, maxforward=70, requestline 
          created, cseq method set, cseq sequence is 1, from and from tag set, contact 
          set, CallId created.
          
          @note Caller owns the returned pointer and is responsible for deleting it.

          @note While contact is only necessary for requests that establish a dialog,
                those are the requests most likely created by this method, others will
                be generated by the dialog.

          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header

          @param contact Ends up in the Contact header.  Stack will not change this
                         when sent.

          @param method Type of request to create.  Methos is used in the Request Line
                        and the CSeq.

          @returns SipMessage request created.  Caller must deallocate.         
      */
      static SipMessage* makeRequest(const NameAddr& target, const NameAddr& from, const NameAddr& contact, MethodTypes method);

      /**
          @brief Make a new request.  To, maxforward=70, requestline created, cseq method set, 
          cseq sequence is 1, from and from tag set, CallId created.
          
          @note Caller owns the returned pointer and is responsible for deleting it.

          @note An empty contact header is added.  This signals to the stack that it
                should be populated by the transports when sent.

          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header

          @param method Type of request to create.  Methos is used in the Request Line
                        and the CSeq.

          @returns SipMessage request created.  Caller must deallocate.         
      */
      static SipMessage* makeRequest(const NameAddr& target, const NameAddr& from, MethodTypes method);

      /**
          @brief Make a new Cancel request for the specified request.
          
          @note Caller owns the returned pointer and is responsible for deleting it.

          @param request Request for which the Cancel will apply. ie. Invite request.

          @returns Created Cancel request.  Caller must deallocate.         
      */
      static SipMessage* makeCancel(const SipMessage& request);
      
      /** 
        @brief Create a Register request with an overriden Contact.
        @param to included in the request line and the To header
        @param from included in the From header
		@param contact included in the Contact header
        @sa makeRequest()
        @returns a regristration request message which the caller must deallocate
      */
      static SipMessage* makeRegister(const NameAddr& to, const NameAddr& from, const NameAddr& contact);
      
      /**
        @brief Create a Register request with an empty Contact
        @param to to field
        @param from from field
        @sa makeRequest
        @returns a regristration request message which the caller must deallocate
      */
      static SipMessage* makeRegister(const NameAddr& to, const NameAddr& from);

      /**
        @brief Create a Register request with an overriden Contact
        @param to to field
        @param transport transport to add to the Request URI
        @param contact the contact field
        @sa makeRequest
        @returns a regristration request message which the caller must deallocate
      */
      static SipMessage* makeRegister(const NameAddr& to, const Data& transport, const NameAddr& contact);

      /**
        Create a Register request with an empty Contact
        @param to to field
        @param transport transport to add to the Request URI        
        @sa makeRequest
        @returns a regristration request message which the caller must deallocate
      */
      static SipMessage* makeRegister(const NameAddr& to, const Data& transport);

      /**
        Create a Subscribe request with an overriden Contact.
        @param target the subscribe target
        @param from the from field
        @param contact to override
        @sa makeRequest
        @returns a subscription request message which the caller must deallocate
      */
      static SipMessage* makeSubscribe(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /**
        Create a Subscribe request with an empty Contact.
        @param target the subscribe target
        @param from the from field
        @sa makeRequest
        @returns a subscription request message which the caller must deallocate        
      */
      static SipMessage* makeSubscribe(const NameAddr& target, const NameAddr& from);

      /**
        Create a Message request with an overriden Contact.
        @param target the message target
        @param from the from field
        @param contact the contact to override
        @sa makeRequest
        @returns a message request message which the caller must deallocate
      */
      static SipMessage* makeMessage(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /**
        Create a Message request with an empty Contact.  See makeRequest.
        @param target the message target
        @param from the from field
        @sa makeRequest
        @returns a message request message which the caller must deallocate
      */
      static SipMessage* makeMessage(const NameAddr& target, const NameAddr& from);

      /** Create a Publish request with an overriden Contact.
        @param target the publish target
        @param from the from field
        @param contact the contact to override
        @sa makeRequest
        @returns a publication request message which the caller must deallocate
      */
      static SipMessage* makePublish(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /**
        Create a Publish request with an empty Contact.  See makeRequest.
        @param target the publish target
        @param from the from field
        @sa makeRequest.
        @returns a publication request message which the caller must deallocate
      */
      static SipMessage* makePublish(const NameAddr& target, const NameAddr& from);

      /**
          This interface should be used by the stack (TransactionState) to create an
          AckMsg to a failure response.  See RFC3261 section 17.1.1.3.  Caller owns the 
          returned pointer and is responsible for deleting it.
          
          @note The branch in this ACK needs to be the one from the request. 
                For TU generated ACK, see Dialog::makeAck(...)

          @param request Request that this ACK applies to.

          @param response Response that we are ACKing - required so that we can get the To tag
                          into the generated ACK.

          @returns Created Ack request.  Caller must deallocate.         
       */
      static SipMessage* makeFailureAck(const SipMessage& request, const SipMessage& response);

      /** 
          @brief Creates and returns a unique branch parameter
          
          Creates and returns a unique branch parameter that will contain
          the RFC3261 magic cookie + 4 randome hex characters + "C1" + 2 random hex characters.

          @deprecated Not used by stack.
      */
      static Data computeUniqueBranch();
      static Data computeProxyBranch(const SipMessage& request);

      
      /**
      @brief creates a call id
      @return a random call id
      */
      static Data computeCallId();
      
      /**
      @brief Genrates a tag parameter
      @param numBytes the number of bytes you wish to generate
      @return a tag parameter consisting of random hex characters
      */
      static Data computeTag(int numBytes);

      /**
      @brief enumeration over the possible results of an authentication attempt
      */
      enum AuthResult {Failed = 1, Authenticated, Expired, BadlyFormed};

      /**
        @brief Validates an authentication request
        @param request the request to authenticate
        @param realm the realm
        @param password the user's password
        @param expiresDelta delta from creation to nonce expiration
        @return the results of the authentication
      */
      static AuthResult authenticateRequest(const SipMessage& request, 
                                            const Data& realm,
                                            const Data& password,
                                            int expiresDelta = 0);
      /**
        @brief Validates an authentication request
        @param request the request to authenticate
        @param realm the realm
        @param hA1 the user's A1 hash
        @param expiresDelta delta from creation to nonce expiration
        @return the results of the authentication
      */
      static AuthResult authenticateRequestWithA1(const SipMessage& request, 
                                                  const Data& realm,
                                                  const Data& hA1,
                                                  int expiresDelta = 0);
      /**
         @brief Validates an authentication request
         @param request the request to authenticate
         @param realm the realm
         @param a1 the user's A1 hash
         @param expiresDelta delta from creation to nonce expiration
         @param proxyAuthorization preform proxy authorization
         @return the results of the authentication, a pair containing an AuthResult and a string containing the user's name
      */
      static std::pair<AuthResult,Data> 
                advancedAuthenticateRequest(const SipMessage& request, 
                                            const Data& realm,
                                            const Data& a1,
                                            int expiresDelta = 0,
                                            bool proxyAuthorization = true);
      
      /**
        @brief create a 407 response with Proxy-Authenticate header filled in
        @param request request to respond to
		@param realm the realm
        @param useAuth sets the auth and auth-int qop options when true
        @param stale sets the stale auth parameter
        @return a 407 response message
      */
      static SipMessage* makeProxyChallenge(const SipMessage& request, 
                                            const Data& realm,
                                            bool useAuth = true,
                                            bool stale = false);

      /**
        @brief create a 401 response with WWW-Authenticate header filled in
        @param request request to respond to
		@param realm the realm
        @param useAuth sets the auth and auth-int qop options when true
        @param stale sets the stale auth parameter
        @return a 401 response message
      */
      static SipMessage* makeWWWChallenge(const SipMessage& request, 
                                            const Data& realm,
                                            bool useAuth = true,
                                            bool stale = false);

      /**
        @brief create a 401 or 407 response with Proxy-Authenticate or Authenticate header filled in
        @param request request to respond to
		@param realm the realm
        @param useAuth sets the auth and auth-int qop options when true
        @param stale sets the stale auth parameter
        @param proxy when true use proxy challenge (407), otherwise use WWW challenge (401)
        @return a 401 or 407 response message
      */
      static SipMessage* makeChallenge(const SipMessage& request, 
                                       const Data& realm,
                                       bool useAuth = true,
                                       bool stale = false,
                                       bool proxy = false);

      static Data qopOption(const Auth& challenge);
      static void updateNonceCount(unsigned int& nonceCount, Data& nonceCountString);

      /**
        @brief checks an Auth challenge uses supported qop options and supported algorithms
        @param challenge the challenge to verify
        @sa Security
        @return true if the challenge is supported and false if it is not 
      */
      static bool algorithmAndQopSupported(const Auth& challenge);
      

      /**
        @brief adds authorization headers in reponse to the 401 or 407
        @note only supports md5
        @param request the request to add the auth headers to (edited in place)
        @param challenge the recieved challenge
        @param username the user
        @param password the user's password
        @param cnonce the cnonce
        @param nonceCount the nonceCount
        @return the request parameter
      */
      static SipMessage& addAuthorization(SipMessage& request,
                                          const SipMessage& challenge,
                                          const Data& username,
                                          const Data& password,
                                          const Data& cnonce,
                                          unsigned int& nonceCount);
      /**
        @brief make an Auth for a challenge response
        @param request the request
        @param username the user
        @param password the user's password
        @param challenge the challenge to answer
        @param cnonce the cnonce
        @param nonceCount the nonce count
        @param nonceCountString nonce count string to be updated
        @return the Auth info for the response
      */
      static Auth makeChallengeResponseAuth(SipMessage& request,
                                            const Data& username,
                                            const Data& password,
                                            const Auth& challenge,
                                            const Data& cnonce,
                                            unsigned int& nonceCount,
                                            Data& nonceCountString);      

      static void makeChallengeResponseAuth(SipMessage& request,
                                           const Data& username,
                                           const Data& password,
                                           const Auth& challenge,
                                           const Data& cnonce,
                                           const Data& authQop,
                                           const Data& nonceCountString,
                                           Auth& auth);

      /**
        @brief make an Auth for a challenge response
        @param request the request
        @param username the user
        @param passwordHashA1 the user's password
        @param challenge the challenge to answer
        @param cnonce the cnonce
        @param nonceCount the nonce count
        @param nonceCountString nonce count string to be updated
        @return the Auth info for the response
      */
      static Auth makeChallengeResponseAuthWithA1(const SipMessage& request,
                                                  const Data& username,
                                                  const Data& passwordHashA1,
                                                  const Auth& challenge,
                                                  const Data& cnonce,
                                                  unsigned int& nonceCount,
                                                  Data& nonceCountString);      
      /**
        @brief Generate an HTTP AUTH request digest using a predefined A1
        @param a1 the A1 hash for the user, incorporated into the digest
        @param method request method, incoroporated into uri
        @param digestUri uri of what is to have the digest taken of
        @param nonce the nonce to use
        @param qop incorporated into the digest
		@param cnonce the cnonce
        @param cnonceCount incorporated into the digest
        @param entityBody incorporataed into the digest
      */
      static Data makeResponseMD5WithA1(const Data& a1,
                                        const Data& method, const Data& digestUri, const Data& nonce,
                                        const Data& qop = Data::Empty, const Data& cnonce = Data::Empty, 
                                        const Data& cnonceCount = Data::Empty, const Contents *entityBody = 0);
      /**
        @brief Generate an HTTP AUTH request digest
        @param username the username, used to generate the A1 hash
        @param password the user's password, used to generate the A1 hash
        @param realm the user's realm, used to generate the A1 hash
        @param method request method, incoroporated into uri
        @param digestUri uri of what is to have the digest taken of
        @param nonce the nonce to use
        @param qop incorporated into the digest
		@param cnonce the cnonce
        @param cnonceCount incorporated into the digest
        @param entityBody incorporataed into the digest        
      */
      static Data makeResponseMD5(const Data& username, const Data& password, const Data& realm, 
                                  const Data& method, const Data& digestUri, const Data& nonce,
                                  const Data& qop = Data::Empty, const Data& cnonce = Data::Empty, 
                                  const Data& cnonceCount = Data::Empty, const Contents *entityBody = 0);
      
      /**
      @brief register a NonceHelper with Helper
      @param nonceHelper a NonceHelper to register, 
      @note Helper assumes control of NonceHelper object and will delete when global scope is cleaned up
      @note DO NOT call setNonceHelper after startup
      */
      static void setNonceHelper(NonceHelper * nonceHelper);
      
      /**
      @brief get the registered NonceHelper
      @note if no NonceHelper is registered a BasicNonceHelper is registered and returned
      @return registered NonceHelper
      @note Helper retains ownership of the returned NonceHelper
      */
      static NonceHelper* getNonceHelper();
      
      /**
      @brief creates a nonce given a request and a timestamp using the registered NonceHelper
      @param request the request, exact behavior is dependent on registered NonceHelper
      @param timestamp the timestamp, exact behavior is dependent on registered NonceHelper @sa BasicNonceHelper::makeNonce
      @note Equivalent to Helper::getNonceHelper()->makeNonce(request,timestamp);
      */
      static Data makeNonce(const SipMessage& request, const Data& timestamp);
      
      /**
      @brief creates a uri from an aor and a scheme
      @param aor the aor to create the uri from
      @param scheme the scheme of the new uri
      @note scheme must be sip or sips
      @sa fromAor(const Data&, const Data&)
      @return a new uri
      */
      static Uri makeUri(const Data& aor, const Data& scheme=Symbols::DefaultSipScheme);
      
      /**
      @brief Alters the request line and route header of a request SipMessage 
         for its next hop, according to the strict route fixup rules in RFC 3261 
         Sec 12.2.1.1, page 73.
      @param request request the SipMessage to alter
      */
      static void processStrictRoute(SipMessage& request);

      /**
      @brief return the port that the response to request should be sent to
      @note port is determined using rules from RFC 3261 - 18.2.2
      @param request the request to determine the response port of
      @return the computed response port
      */
      static int getPortForReply(SipMessage& request);

      static void massageRoute(const SipMessage& request, NameAddr& route);

      /**
      @brief creates a URI ( Uri )
      @note unlike makeUri(const Data&,const Data&) it is not required that scheme is a sip or sips scheme
      @param aor the AOR of the created URI
      @param scheme the scheme of the new URI
      @return a constructed Uri
      */
      static Uri fromAor(const Data& aor, const Data& scheme=Symbols::DefaultSipScheme);

      /** 
          @brief Cursory validation of a message recieved off the wire
          @details Does basic checks to validate a received message off the wire
          If the basic check fails, and reason is non-null, reason will be set
          to the reason the check failed.
          @note This function does not take ownership of reason.
          @param message the message to validate
          @param reasons an optional pointer to a data that will be filled with the a string explaining
                 the validation failure.
          @return returns true only if the message passes cursory validation and false otherwise.
      */
      static bool validateMessage(const SipMessage& message,resip::Data* reasons=0);

      /**
      @brief creates a reversable, opaque GRUU user part
      @param instanceId the instanceId to create the GRUU user part with
      @param aor the aor to create the GRUU user part with
      @param key the key to create the GRUU user part with
      @return the user portion of a GRUU
      */
      static Data gruuUserPart(const Data& instanceId,
                               const Data& aor,
                               const Data& key);

      /**
        @brief extracts an the instance id and aor from the user portion of a Gruu
        @param gruuUserPart the user portion of the GRUU
        @param key the key
        @return a pair of Data instances, the first containing the instance id and the second containing the aor
      */
      static std::pair<Data,Data> fromGruuUserPart(const Data& gruuUserPart,
                                                   const Data& key);
      /**
        @brief A container for storing message contents along with the related security attributes
      */
      struct ContentsSecAttrs
      {
            ContentsSecAttrs();
            ContentsSecAttrs(std::auto_ptr<Contents> contents,
                             std::auto_ptr<SecurityAttributes> attributes);
            ContentsSecAttrs(const ContentsSecAttrs& rhs);
            ContentsSecAttrs& operator=(const ContentsSecAttrs& rhs);

            /**
            @brief auto pointer to a message contents instance
            @sa Contents
            */
            mutable std::auto_ptr<Contents> mContents;

            /**
            @brief auto pointer to the security attributes of the message contents
            @sa SecurityAttributes
            */
            mutable std::auto_ptr<SecurityAttributes> mAttributes;
      };
      
      /**
        @brief extracts message contents and security attributes from a Pkcs7 envelope
        @param message the sip message to extract information from
        @param security the cryptographic security manager to use
        @return a structure containing the contents and security attributes
      */
      static ContentsSecAttrs extractFromPkcs7(const SipMessage& message, Security& security);

      /**
        @brief enumeration over the effects a failed message can have
      */
      enum FailureMessageEffect{ DialogTermination, TransactionTermination, UsageTermination, 
                                 RetryAfter, OptionalRetryAfter, ApplicationDependant };
      
      /**
        @brief determines the effect of a failed message
        @param response the failed message
        @return the effect
      */
      static FailureMessageEffect determineFailureMessageEffect(const SipMessage& response);      

      /**
        @brief determines the effect of a failed message
        @param isResponse is the message a response?
        @param code the response code
        @param retryAfterExists is retry after present
        @return the effect
      */
      static FailureMessageEffect determineFailureMessageEffect(bool isResponse,
                                                                int code,
                                                                bool retryAfterExists);      

      /**
        @brief returns the first SdpContents found the Contents tree
        @param tree the Contents tree
        @return the first SdpContents found
      */
      static std::auto_ptr<SdpContents> getSdp(Contents* tree);

   private:

      /**
         @internal
         @todo replace with auto_ptr
      */
      class NonceHelperPtr
      {
         public:
            NonceHelperPtr() : mNonceHelper(0) {}
            ~NonceHelperPtr() { delete mNonceHelper; }
            NonceHelper *mNonceHelper;
      };
      static NonceHelperPtr mNonceHelperPtr;
};

/*
    static Data Helper::computeProxyBranch(const SipMessage& request); is not implemented anywhere and has been removed from
    our local header.
*/

}

#endif

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
