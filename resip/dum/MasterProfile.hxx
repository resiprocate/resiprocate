#ifndef RESIP_MasterProfile_hxx
#define RESIP_MasterProfile_hxx

#include <iosfwd>
#include <set>
#include <map>
#include "resip/stack/Headers.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Token.hxx"
#include "resip/dum/UserProfile.hxx"

namespace resip
{

class Data;

class MasterProfile : public UserProfile
{
   public:  
      
      /// Creates an Indentity/Profile with no BaseProfile - this is the root of all profiles
      MasterProfile();  
      
      /// Default is "sip"
      virtual void addSupportedScheme(const Data& scheme);          
      virtual bool isSchemeSupported(const Data& scheme) const;
      virtual void clearSupportedSchemes(void);

      /// Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE, UPDATE
      virtual void addSupportedMethod(const MethodTypes& method);   
      virtual bool isMethodSupported(MethodTypes method) const;
      virtual Tokens getAllowedMethods() const;
      virtual Data getAllowedMethodsData() const;

      virtual void clearSupportedMethods(void);

      /// Default is none. Do not use to enable PRACK(100rel) support. 
      virtual void addSupportedOptionTag(const Token& tag);        
      virtual Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      virtual Tokens getSupportedOptionTags() const;
      virtual void clearSupportedOptionTags(void);

      typedef enum
      {
         Never,     
         SupportedEssential,  // If UAS - Only use reliable provisionals if sending a body and far end supports
         Supported,           // If UAS - Always use reliable provisionals if far end supports
         Required             // If UAS - Always use reliable provisionals
      } ReliableProvisionalMode;

      // UAC PRACK support.  UPDATE must be enabled(currently defaults to on, do
      // not disable w/out disabling UAC PRACK support).
      //
      // Flows where an an answer is received in a 180rel and subsequent O/A
      // exchanges using UPDATE occur in the early dialog
      // have been tested.
      //
      // A subsequent O/A exchange using 180rel/PRACK is also supported. This is
      // a really bad idea, as an answer must be generated; the offer cannot be
      // rejected. UPDATE should always be used for O/A exchanges once the
      // dialog is established.
      // 
      // Invite/18x(offer)/PRACK(ans) also works
      // 
      // Invite(offer)/18x(ans)/PRACK(offer)/200P(ans) issupported, but not recommended.  
      // The UAC MUST call provideOffer from the onAnswer callback in order to generate 
      // the offer in the PRACK.
      //
      // Explicit limitations are:
      // - Overlapping reliable provisional responses that contain a body are not
      //   handled.
      //
      // Note:  Using SupportedEssential is exactly the same as using Supported, 
      //        SupportedEssential only effects UAS Prack implementation
      virtual void setUacReliableProvisionalMode(ReliableProvisionalMode mode);
      virtual ReliableProvisionalMode getUacReliableProvisionalMode() const;

      // UAS PRACK support.  UPDATE must be enabled(currently defaults to on, do
      // not disable w/out disabling UAS PRACK support).
      //
      // All flows and limitations mentioned in UAC Prack comments apply
      //
      // Modes work as follows:
      // SupportedEssential - Only send reliable provisionals if sending a body and far end supports
      // Supported - Always send reliable provisionals if far end supports
      // Required - Always send reliable provisionals
      virtual void setUasReliableProvisionalMode(ReliableProvisionalMode mode);
      virtual ReliableProvisionalMode getUasReliableProvisionalMode() const;

      /// Default is application/sdp for INVITE, OPTIONS, PRACK and UPDATE Methods
      virtual void addSupportedMimeType(const MethodTypes& method, const Mime& mimeType);      
      virtual bool removeSupportedMimeType(const MethodTypes& method, const Mime& mimeType);      
      virtual bool isMimeTypeSupported(const MethodTypes& method, const Mime& mimeType);
      virtual Mimes getSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(void);  // Clear for all Methods

      /// Default is no encoding
      virtual void addSupportedEncoding(const Token& encoding);     
      virtual bool isContentEncodingSupported(const Token& contentEncoding) const;
      virtual Tokens getSupportedEncodings() const;
      virtual void clearSupportedEncodings(void);

      /// Default is all - if nothing is set, then all are allowed
      virtual void addSupportedLanguage(const Token& lang);         
      virtual bool isLanguageSupported(const Tokens& lang) const;
      virtual Tokens getSupportedLanguages() const;
      virtual void clearSupportedLanguages(void);
      
      /// Default is to not send an Allow-Events header.
      virtual void addAllowedEvent(const Token& event);         
      virtual bool isEventAllowed(const Tokens& event) const;
      virtual Tokens getAllowedEvents() const;
      virtual void clearAllowedEvents(void);
      
      ///enable/disable content validation
      virtual bool& validateContentEnabled();
      virtual bool validateContentEnabled() const;

      ///enable/disable content language validation
      virtual bool& validateContentLanguageEnabled();
      virtual bool validateContentLanguageEnabled() const;

      ///enable/disable Accept header validation
      virtual bool& validateAcceptEnabled();
      virtual bool validateAcceptEnabled() const;

      ///Set this to allow the Registration Server to accept registration requests that contain 
      ///a To Tag.
      virtual bool& allowBadRegistrationEnabled();
      virtual bool allowBadRegistrationEnabled() const;  

           
      ///
      /// Used when receiveing a REGISTER request, if the expires value in the request
      /// is less than this time, then dum will reject the message with a 423 and set the
      /// min-expires header to the value specified here.
      ///
      virtual UInt32& serverRegistrationMinExpiresTime(void);
      virtual const UInt32 serverRegistrationMinExpiresTime(void) const;

      ///
      /// If an inbound REGISTER has an Expires header or any individual contact bindings with expires greater
      /// than this value, use this Max expires instead of the one given by the client.
      virtual UInt32& serverRegistrationMaxExpiresTime(void);
      virtual const UInt32 serverRegistrationMaxExpiresTime(void) const;

      /// If no Expires header or individual contact bindings specify an expiration value, use this value.
      virtual UInt32& serverRegistrationDefaultExpiresTime(void);
      virtual const UInt32 serverRegistrationDefaultExpiresTime(void) const;

      ///Set this to include the RequestURI in merge request detection.
      ///*!*!*!*!*!*! RED FLASHING LIGHT *!*!*!*!*!*! 
      ///When false, DUM implements the policy that all RURIs that arrive are equivalent,
      ///so if a request forks and arives here with different RURIs, we reject all but one
      ///of them as merged requests. This makes sense for single-line endpoints.  Nodes
      ///responsible for multiple simultaneous resources (like gateways, media-servers,
      ///B2BUAs, etc) need to set this to true. Applications like multi-line business
      ///phones will want to carefully consider the edge case of a request that forks
      ///to more than one line - if you want only one line to ring, leave this false.
      ///If you want them all to ring, set it to true.
      
      virtual bool& checkReqUriInMergeDetectionEnabled();
      virtual bool checkReqUriInMergeDetectionEnabled() const;

      /// Enabling this setting will allow the application layer to provide additional SIP responses, from class 4xx, 5xx, 6xx, 
      /// that will lead to transaction termination instead of other failure effects like dialog termination, as defined by
      /// method Helper::determineFailureMessageEffect. (See header Helper.hxx for all transaction failure effects).
      /// A scenarui when this is useful is when, for a server subscription, a NOTIFY is responded with an error response due to a timeout
      /// condition, but the application needs the subscription to be continued. Even though RFC 3265 prescribes the
      /// notifier should remove the subscription in such cases, timeouts may occur for transient conditions like a
      /// overloaded proxy, a slow network connection, eventually nobody would benefit if the subscription is terminated by the server.
      /// With this setting enabled the subscription will be continued, but only until the subscription expires.
      /// Currently, it is only used by ServerSubscriptions.
      /// Default is to not allow additional transaction terminating responses
      /// To enable it:
      /// 1. Set additionalTransactionTerminatingResponsesEnabled() = true on master profile
      /// 2. Call method addAdditionalTransactionTerminatingResponses(code) to provide SIP responses for which the application
      /// need the transaction to be terminated
      virtual bool& additionalTransactionTerminatingResponsesEnabled();
      virtual bool additionalTransactionTerminatingResponsesEnabled() const;

      virtual void addAdditionalTransactionTerminatingResponses(int code);
      virtual bool isAdditionalTransactionTerminatingResponse(int code) const;

      virtual const std::set<int>& getAdditionalTransactionTerminatingResponses() const;
      virtual void clearAdditionalTransactionTerminatingResponses(void);

   private:
      virtual UserProfile* clone() const;
      std::set<Data> mSupportedSchemes;
      std::set<MethodTypes> mSupportedMethodTypes;
      Tokens mSupportedMethods;
      Tokens mSupportedOptionTags;
      std::map<MethodTypes, Mimes> mSupportedMimeTypes;
      Tokens mSupportedEncodings;
      Tokens mSupportedLanguages;
      Tokens mAllowedEvents;

      bool mValidateContentEnabled;
      bool mValidateContentLanguageEnabled;
      bool mValidateAcceptEnabled;
      bool mAllowBadRegistrationEnabled;    
      bool mHasServerRegistrationMinExpires;      
      bool mCheckReqUriInMergeDetectionEnabled;
      ReliableProvisionalMode mUacReliableProvisionalMode;
      ReliableProvisionalMode mUasReliableProvisionalMode;
      UInt32 mServerRegistrationMinExpires;
      UInt32 mServerRegistrationMaxExpires;
      UInt32 mServerRegistrationDefaultExpires;

      bool mAdditionalTransactionTerminatingResponsesEnabled;
      std::set<int> mAdditionalTransactionTerminatingResponsess;
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

