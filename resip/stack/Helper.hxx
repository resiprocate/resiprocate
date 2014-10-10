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

      /// bytes in to-tag& from-tag, should prob. live somewhere else
      const static int tagSize;  

      /** 
          Used by Registration, Publication and Subscription refreshes, to
          calculate the time at which a refresh should be performed (which
          is some time, that is a bit smaller than the Expiration interval).
          The recommended calculation from the RFC's is the minimnum of the 
          Exipiration interval less 5 seconds and nine tenths of the exipiration 
          interval.
      */
      template<typename T>
      static T aBitSmallerThan(T secs)
      {
         return resipMax(T(0), resipMin(T(secs-5), T(9*secs/10)));
      }

      /** 
          Converts an interger in a character string containing the
          hexidecimal representation of the integer.  Note:  The 
          string buffer provided should be at least 8 characters long.
          This function will NOT NULL terminate the string.

          @param _d     A pointer to the character buffer to write
                        the hex string

          @param _s     The integer value to convert.

          @param _l     Boolean flag to include leading 0 zeros or 
                        not.
      */
      static void integer2hex(char* _d, unsigned int _s, bool _l = true);

      /** 
          Converts a character string containing a hexidecimal value
          into an unsigned int.  Note:  Parsing stops after the first
          non-hex character, or after 8 characters have been processed.

          @param _s     A pointer to the character buffer to convert.

          @returns      The integer value of the coverted hex string.
      */
      static unsigned int hex2integer(const char* _s);

      /**
           Used to jitter the expires in a SUBSCRIBE or REGISTER expires header

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
          Make an invite request - Empty Contact and Via is added and will be populated 
          by the stack when sent.
            
          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header
      */
      static SipMessage* makeInvite(const NameAddr& target, const NameAddr& from);
      
      /**
          Make an invite request using a overridden contact header - Empty Via is added 
          and will be populated by the stack when sent.
            
          @param target Ends up in the RequestURI and To header

          @param from   Ends up in the From header

          @param contact Ends up in the Contact header.  Stack will not change this
                         when sent.
      */
      static SipMessage* makeInvite(const NameAddr& target, const NameAddr& from, const NameAddr& contact);
      
      /**
          Make a response to a provided request.  Adds a To tag, Contact and Record-Route
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
          Make a response to a provided request with an overridden Contact.  
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
          Make a new response to a provided request.  Adds a To tag, Contact and 
          Record-Route headers appropriately.  Caller owns the returned pointer and
          is responsible for deleting it.
            
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
          Make a new response to a provided request with an overridden Contact.  
          Adds a To tag, Contact and Record-Route headers appropriately.
          Caller owns the returned pointer and is responsible for deleting it.

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

      static void makeRawResponse(Data& rawBuffer,
                                    const SipMessage& request, 
                                    int responseCode,
                                    const Data& additionalHeaders=Data::Empty,
                                    const Data& body=Data::Empty);

      /**
          Make a 405 response to a provided request.  Allows header is added
          with specified methods, or with all methods the stack knows about.
          Caller owns the returned pointer and is responsible for deleting it.

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
          Returns the default reason string for a particular response code.

          @param responseCode  Response code to get reason for

          @param reason Data where the reason string associated with the 
                   responseCode will be set.
      */
      static void getResponseCodeReason(int responseCode, Data& reason);

      /**
          Make a new request with a overridden Contact.  To, maxforward=70, requestline 
          created, cseq method set, cseq sequence is 1, from and from tag set, contact 
          set, CallId created.  Caller owns the returned pointer and is responsible for 
          deleting it.

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
          Make a new request.  To, maxforward=70, requestline created, cseq method set, 
          cseq sequence is 1, from and from tag set, CallId created.  Caller owns the 
          returned pointer and is responsible for deleting it.

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
          Make a new Cancel request for the specified request.  Caller owns the 
          returned pointer and is responsible for deleting it.

          @param request Request for which the Cancel will apply. ie. Invite request.

          @returns Created Cancel request.  Caller must deallocate.         
      */
      static SipMessage* makeCancel(const SipMessage& request);
      
      /// Create a Register request with an overriden Contact.  See makeRequest.
      static SipMessage* makeRegister(const NameAddr& to, const NameAddr& from, const NameAddr& contact);

      /// Create a Register request with an empty Contact.  See makeRequest.
      static SipMessage* makeRegister(const NameAddr& to, const NameAddr& from);

      /// Create a Register request with an overriden Contact, transport is added to Request URI.  See makeRequest.
      static SipMessage* makeRegister(const NameAddr& to, const Data& transport, const NameAddr& contact);

      /// Create a Register request with an empty Contact, transport is added to Request URI.  See makeRequest.
      static SipMessage* makeRegister(const NameAddr& to, const Data& transport);

      /// Create a Subscribe request with an overriden Contact.  See makeRequest.
      static SipMessage* makeSubscribe(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /// Create a Subscribe request with an empty Contact.  See makeRequest.
      static SipMessage* makeSubscribe(const NameAddr& target, const NameAddr& from);

      /// Create a Message request with an overriden Contact.  See makeRequest.
      static SipMessage* makeMessage(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /// Create a Message request with an empty Contact.  See makeRequest.
      static SipMessage* makeMessage(const NameAddr& target, const NameAddr& from);

      /// Create a Publish request with an overriden Contact.  See makeRequest.
      static SipMessage* makePublish(const NameAddr& target, const NameAddr& from, const NameAddr& contact);

      /// Create a Publish request with an empty Contact.  See makeRequest.
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
          Creates and returns a unique branch parameter.  Generated branch will contain
          the RFC3261 magic cookie + 4 randome hex characters + "C1" + 2 random hex characters.

          @deprecated Not used by stack.
      */
      static Data computeUniqueBranch();
      static Data computeProxyBranch(const SipMessage& request);

      static Data computeCallId();
      static Data computeTag(int numBytes);

      enum AuthResult {Failed = 1, Authenticated, Expired, BadlyFormed};

      static AuthResult authenticateRequest(const SipMessage& request, 
                                            const Data& realm,
                                            const Data& password,
                                            int expiresDelta = 0);

      static AuthResult authenticateRequestWithA1(const SipMessage& request, 
                                                  const Data& realm,
                                                  const Data& hA1,
                                                  int expiresDelta = 0);
      
      static std::pair<AuthResult,Data> 
                advancedAuthenticateRequest(const SipMessage& request, 
                                            const Data& realm,
                                            const Data& a1,
                                            int expiresDelta = 0,
                                            bool proxyAuthorization = true);
      
      // create a 407 response with Proxy-Authenticate header filled in
      static SipMessage* makeProxyChallenge(const SipMessage& request, 
                                            const Data& realm,
                                            bool useAuth = true,
                                            bool stale = false);

      //create a 401 response with WWW-Authenticate header filled in
      static SipMessage* makeWWWChallenge(const SipMessage& request, 
                                            const Data& realm,
                                            bool useAuth = true,
                                            bool stale = false);

      // create a 401 or 407 response with Proxy-Authenticate or Authenticate header 
      // filled in
      static SipMessage* makeChallenge(const SipMessage& request, 
                                       const Data& realm,
                                       bool useAuth = true,
                                       bool stale = false,
                                       bool proxy = false);

      static Data qopOption(const Auth& challenge);
      static void updateNonceCount(unsigned int& nonceCount, Data& nonceCountString);
      static bool algorithmAndQopSupported(const Auth& challenge);
      

      // adds authorization headers in reponse to the 401 or 407--currently
      // only supports md5.
      static SipMessage& addAuthorization(SipMessage& request,
                                          const SipMessage& challenge,
                                          const Data& username,
                                          const Data& password,
                                          const Data& cnonce,
                                          unsigned int& nonceCount);

      static Auth makeChallengeResponseAuth(const SipMessage& request,
                                            const Data& username,
                                            const Data& password,
                                            const Auth& challenge,
                                            const Data& cnonce,
                                            unsigned int& nonceCount,
                                            Data& nonceCountString);      

      static void makeChallengeResponseAuth(const SipMessage& request,
                                            const Data& username,
                                            const Data& password,
                                            const Auth& challenge,
                                            const Data& cnonce,
                                            const Data& authQop,
                                            const Data& nonceCountString,
                                            Auth& auth);

      static Auth makeChallengeResponseAuthWithA1(const SipMessage& request,
                                                  const Data& username,
                                                  const Data& passwordHashA1,
                                                  const Auth& challenge,
                                                  const Data& cnonce,
                                                  unsigned int& nonceCount,
                                                  Data& nonceCountString);      

      static void makeChallengeResponseAuthWithA1(const SipMessage& request,
                                                  const Data& username,
                                                  const Data& passwordHashA1,
                                                  const Auth& challenge,
                                                  const Data& cnonce,
                                                  const Data& authQop,
                                                  const Data& nonceCountString,
                                                  Auth& auth);

      static Data makeResponseMD5WithA1(const Data& a1,
                                        const Data& method, const Data& digestUri, const Data& nonce,
                                        const Data& qop = Data::Empty, const Data& cnonce = Data::Empty, 
                                        const Data& cnonceCount = Data::Empty, const Contents *entityBody = 0);

      static Data makeResponseMD5(const Data& username, const Data& password, const Data& realm, 
                                  const Data& method, const Data& digestUri, const Data& nonce,
                                  const Data& qop = Data::Empty, const Data& cnonce = Data::Empty, 
                                  const Data& cnonceCount = Data::Empty, const Contents *entityBody = 0);
      
      /// Note: Helper assumes control of NonceHelper object and will delete when global scope is cleaned up      
      static void setNonceHelper(NonceHelper *nonceHelper);
      static NonceHelper* getNonceHelper();
      static Data makeNonce(const SipMessage& request, const Data& timestamp);

      static Uri makeUri(const Data& aor, const Data& scheme=Symbols::DefaultSipScheme);

      static void processStrictRoute(SipMessage& request);

      // return the port that the response should be sent to using rules from 
      // RFC 3261 - 18.2.2
      static int getPortForReply(SipMessage& request);

      static void massageRoute(const SipMessage& request, NameAddr& route);

      static Uri fromAor(const Data& aor, const Data& scheme=Symbols::DefaultSipScheme);

      // Do basic checks to validate a received message off the wire
      // If the basic check fails, and reason is non-null, reason will be set
      // to the reason the check failed. This function does not take ownership
      // of reason.
      static bool validateMessage(const SipMessage& message,resip::Data* reason=0);

      // GRUU support -- reversibly and opaquely combine instance id and aor
      static Data gruuUserPart(const Data& instanceId,
                               const Data& aor,
                               const Data& key);

      // GRUU support -- extract instance id and aor from user portion
      static std::pair<Data,Data> fromGruuUserPart(const Data& gruuUserPart,
                                                   const Data& key);

      struct ContentsSecAttrs
      {
            ContentsSecAttrs();
            ContentsSecAttrs(std::auto_ptr<Contents> contents,
                             std::auto_ptr<SecurityAttributes> attributes);
            ContentsSecAttrs(const ContentsSecAttrs& rhs);
            ContentsSecAttrs& operator=(const ContentsSecAttrs& rhs);
            mutable std::auto_ptr<Contents> mContents;
            mutable std::auto_ptr<SecurityAttributes> mAttributes;
      };

      static ContentsSecAttrs extractFromPkcs7(const SipMessage& message, Security& security);

      
      enum FailureMessageEffect{ DialogTermination, TransactionTermination, UsageTermination, 
                                 RetryAfter, OptionalRetryAfter, ApplicationDependant };
      
      static FailureMessageEffect determineFailureMessageEffect(const SipMessage& response,
          const std::set<int>* additionalTransactionTerminatingResponses = NULL);

      // Just simply walk the contents tree and return the first SdpContents in
      // the tree.
      static std::auto_ptr<SdpContents> getSdp(Contents* tree);

      /** Looks at SIP headers and message source for a mismatch to make an
          assumption that the sender is behind a NAT device.

          @param request Request message that we use for checking.

          @privateToPublicOnly If enabled then we ensure that the via is private
                               address and that the source was a public address.
                               This allows us to ignore cases of private-private NAT'ing
                               or false detections, when a server behind a load balancer
                               is sending us requests and using the load balancer address
                               in the Via, instead of the real of the adapter.
      */
      static bool isClientBehindNAT(const SipMessage& request, bool privateToPublicOnly=true);

      /** Look at Via headers, and find the first public IP address closest to the sending
          client.

          @param request Request message that we use for checking.

          @note If no public IP's are found, then an empty Tuple is returned.  This
                can be tested by checking if Tuple::getType() returns UNKNOWN_TRANSPORT.
      */
      static Tuple getClientPublicAddress(const SipMessage& request);

   private:
      class NonceHelperPtr
      {
         public:
            NonceHelperPtr() : mNonceHelper(0) {}
            ~NonceHelperPtr() { delete mNonceHelper; }
            NonceHelper *mNonceHelper;
      };
      static NonceHelperPtr mNonceHelperPtr;
};

}

#endif

/* ====================================================================
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
