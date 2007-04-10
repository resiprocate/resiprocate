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

      /// Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE
      virtual void addSupportedMethod(const MethodTypes& method);   
      virtual bool isMethodSupported(MethodTypes method) const;
      virtual Tokens getAllowedMethods() const;
      virtual Data getAllowedMethodsData() const;

      virtual void clearSupportedMethods(void);

      /// Default is none
      virtual void addSupportedOptionTag(const Token& tag);        
      virtual Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      virtual Tokens getSupportedOptionTags() const;
      virtual void clearSupportedOptionTags(void);

      // Note only supported as UAC (10/29/2006)
      typedef enum
      {
         Never,
         Required,
         IfRequested
      } ReliableProvisionalMode;
      virtual void setReliableProvisionalMode(ReliableProvisionalMode mode);
      virtual ReliableProvisionalMode getReliableProvisionalMode() const;

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
      virtual const bool validateContentEnabled() const;

      ///enable/disable content language validation
      virtual bool& validateContentLanguageEnabled();
      virtual const bool validateContentLanguageEnabled() const;

      ///enable/disable Accept header validation
      virtual bool& validateAcceptEnabled();
      virtual const bool validateAcceptEnabled() const;

      ///Set this to allow the Registration Server to accept registration requests that contain 
      ///a To Tag.
      virtual bool& allowBadRegistrationEnabled();
      virtual const bool allowBadRegistrationEnabled() const;  

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
      virtual const bool checkReqUriInMergeDetectionEnabled() const;

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
      bool mCheckReqUriInMergeDetectionEnabled;
      ReliableProvisionalMode mReliableProvisionalMode;
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

