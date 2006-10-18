#ifndef RESIP_MasterProfile_hxx
#define RESIP_MasterProfile_hxx

#include <iosfwd>
#include <set>
#include <map>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/Token.hxx"
#include "resiprocate/dum/UserProfile.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

namespace resip
{

class Data;

class DUM_API MasterProfile : public UserProfile
{
   public:  
      
       // Creates an Indentity/Profile with no BaseProfile - this is the root of all profiles
       MasterProfile();  
      
      // Default is "sip"
      virtual void addSupportedScheme(const Data& scheme);          
      virtual bool isSchemeSupported(const Data& scheme) const;
      virtual void clearSupportedSchemes(void);

      // Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE
      virtual void addSupportedMethod(const MethodTypes& method);   
      virtual bool isMethodSupported(MethodTypes method) const;
      virtual Tokens getAllowedMethods() const;
      virtual void clearSupportedMethods(void);

      // Default is none
      virtual void addSupportedOptionTag(const Token& tag);        
      virtual Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      virtual Tokens getSupportedOptionTags() const;
      virtual void clearSupportedOptionTags(void);

      // Default is application/sdp for INVITE, OPTIONS, PRACK and UPDATE Methods
      virtual void addSupportedMimeType(const MethodTypes& method, const Mime& mimeType);
      virtual void removeSupportedMimeType(const MethodTypes& method, const Mime& mimeType);
      virtual bool isMimeTypeSupported(const MethodTypes& method, const Mime& mimeType);
      virtual Mimes getSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(void);  // Clear for all Methods

      // Default is no encoding
      virtual void addSupportedEncoding(const Token& encoding);     
      virtual bool isContentEncodingSupported(const Token& contentEncoding) const;
      virtual Tokens getSupportedEncodings() const;
      virtual void clearSupportedEncodings(void);

      // Default is all - if nothing is set, then all are allowed
      virtual void addSupportedLanguage(const Token& lang);         
      virtual bool isLanguageSupported(const Tokens& lang) const;
      virtual Tokens getSupportedLanguages() const;
      virtual void clearSupportedLanguages(void);
      
      //enable/disable content validation
      virtual bool& validateContentEnabled();
      virtual const bool validateContentEnabled() const;

      //enable/disable content language validation
      virtual bool& validateContentLanguageEnabled();
      virtual const bool validateContentLanguageEnabled() const;

      //enable/disable Accept header validation
      virtual bool& validateAcceptEnabled();
      virtual const bool validateAcceptEnabled() const;

   private:
      std::set<Data> mSupportedSchemes;
      std::set<MethodTypes> mSupportedMethodTypes;
      Tokens mSupportedMethods;
      Tokens mSupportedOptionTags;
      std::map<MethodTypes, Mimes> mSupportedMimeTypes;
      Tokens mSupportedEncodings;
      Tokens mSupportedLanguages;

      bool mValidateContentEnabled;
      bool mValidateContentLanguageEnabled;
      bool mValidateAcceptEnabled;
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
