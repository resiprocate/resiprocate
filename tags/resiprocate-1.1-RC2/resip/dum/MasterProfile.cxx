
#include "resip/dum/Profile.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/HeaderTypes.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


// Be sure to look at the documentation of the accessors for 
// the members being set by this constructor in the .hxx file 
// for the implications of these default values.

MasterProfile::MasterProfile() : 
   mValidateContentEnabled(true),
   mValidateContentLanguageEnabled(false),
   mValidateAcceptEnabled(false),
   mAllowBadRegistrationEnabled(false),
   mCheckReqUriInMergeDetectionEnabled(false)
{
   // Default settings
   addSupportedMimeType(INVITE, Mime("application", "sdp"));
   addSupportedMimeType(OPTIONS, Mime("application", "sdp"));
   addSupportedMimeType(PRACK, Mime("application", "sdp"));
   addSupportedMimeType(UPDATE, Mime("application", "sdp"));
   addSupportedLanguage(Token("en"));
   addSupportedMethod(INVITE);
   addSupportedMethod(ACK);
   addSupportedMethod(CANCEL);
   addSupportedMethod(OPTIONS);
   addSupportedMethod(BYE);
   addSupportedScheme(Symbols::Sip);  
}

void 
MasterProfile::addSupportedScheme(const Data& scheme)
{
   mSupportedSchemes.insert(scheme);
}

bool 
MasterProfile::isSchemeSupported(const Data& scheme) const
{
   return mSupportedSchemes.count(scheme) != 0;
}

void 
MasterProfile::clearSupportedSchemes()
{
   mSupportedSchemes.clear();
}

void 
MasterProfile::addSupportedMethod(const MethodTypes& method)
{
   mSupportedMethodTypes.insert(method);
   mSupportedMethods.push_back(Token(getMethodName(method)));
}

bool 
MasterProfile::isMethodSupported(MethodTypes method) const
{
   return mSupportedMethodTypes.count(method) != 0;
}

Tokens 
MasterProfile::getAllowedMethods() const
{
   return mSupportedMethods;
}

Data
MasterProfile::getAllowedMethodsData() const
{
   Data result;
   
   for (Tokens::const_iterator i = mSupportedMethods.begin();
        i != mSupportedMethods.end(); ++i)
   {
      if (i != mSupportedMethods.begin())
      {
         result += ',';
      }
      result += i->value();
   }
   
   return result;
}

void 
MasterProfile::clearSupportedMethods()
{
   mSupportedMethodTypes.clear();
   mSupportedMethods.clear();
}

void 
MasterProfile::addSupportedOptionTag(const Token& tag)
{
   mSupportedOptionTags.push_back(tag);
}

Tokens 
MasterProfile::getUnsupportedOptionsTags(const Tokens& requiresOptionTags)
{
   Tokens tokens;
   for (Tokens::const_iterator i=requiresOptionTags.begin(); i != requiresOptionTags.end(); ++i)
   {
      if (!i->isWellFormed())
      {
         tokens.push_back(Token("malformedTag"));
      }
      // if this option is not supported
      else if (!mSupportedOptionTags.find(*i))
      {
         tokens.push_back(*i);
      }
   }
   
   return tokens;
}

Tokens 
MasterProfile::getSupportedOptionTags() const
{
   return mSupportedOptionTags;
}

void 
MasterProfile::clearSupportedOptionTags()
{
   mSupportedOptionTags.clear();
}

void 
MasterProfile::addSupportedMimeType(const MethodTypes& method, const Mime& mimeType)
{
   mSupportedMimeTypes[method].push_back(mimeType);
}

bool 
MasterProfile::isMimeTypeSupported(const MethodTypes& method, const Mime& mimeType)
{
   if(!mimeType.isWellFormed())
   {
      return false;
   }
   
   std::map<MethodTypes, Mimes>::iterator found = mSupportedMimeTypes.find(method); 
   if (found != mSupportedMimeTypes.end()) 
   { 
      return found->second.find(mimeType); 
   } 
   return false; 
}

Mimes 
MasterProfile::getSupportedMimeTypes(const MethodTypes& method)
{
   std::map<MethodTypes, Mimes>::iterator found = mSupportedMimeTypes.find(method); 
   if (found != mSupportedMimeTypes.end()) 
   { 
      return found->second; 
   } 
   return Mimes(); 
}

void 
MasterProfile::clearSupportedMimeTypes(const MethodTypes& method)
{
   std::map<MethodTypes, Mimes>::iterator found = mSupportedMimeTypes.find(method); 
   if (found != mSupportedMimeTypes.end()) 
   { 
      found->second.clear(); 
   } 
}

void 
MasterProfile::clearSupportedMimeTypes()
{
   mSupportedMimeTypes.clear();
}

void 
MasterProfile::addSupportedEncoding(const Token& encoding)
{
   mSupportedEncodings.push_back(encoding);
}

bool 
MasterProfile::isContentEncodingSupported(const Token& encoding) const
{
   return encoding.isWellFormed() && mSupportedEncodings.find(encoding);
}

Tokens 
MasterProfile::getSupportedEncodings() const
{
   return mSupportedEncodings;
}

void 
MasterProfile::clearSupportedEncodings()
{
   mSupportedEncodings.clear();
}

void 
MasterProfile::addSupportedLanguage(const Token& lang)
{
   mSupportedLanguages.push_back(lang);
}

bool 
MasterProfile::isLanguageSupported(const Tokens& langs) const
{
   for (Tokens::const_iterator i=langs.begin(); i != langs.end(); ++i)
   {
      if (!i->isWellFormed() || mSupportedLanguages.find(*i) == false)
      {
         return false;
      }
   }
   return true;
}

Tokens 
MasterProfile::getSupportedLanguages() const
{
   return mSupportedLanguages;
}

void 
MasterProfile::clearSupportedLanguages()
{
   mSupportedLanguages.clear();
}

void 
MasterProfile::addAllowedEvent(const Token& event)
{
   mAllowedEvents.push_back(event);
}

bool 
MasterProfile::isEventAllowed(const Tokens& events) const
{
   for (Tokens::const_iterator i=events.begin(); i != events.end(); ++i)
   {
      if (!i->isWellFormed() || mAllowedEvents.find(*i) == false)
      {
         return false;
      }
   }
   return true;
}

Tokens 
MasterProfile::getAllowedEvents() const
{
   return mAllowedEvents;
}

void 
MasterProfile::clearAllowedEvents()
{
   mAllowedEvents.clear();
}

bool& 
MasterProfile::validateContentEnabled()
{
   return mValidateContentEnabled;   
}

const bool 
MasterProfile::validateContentEnabled() const
{
   return mValidateContentEnabled;   
}

bool& 
MasterProfile::validateContentLanguageEnabled()
{
   return mValidateContentLanguageEnabled;   
}

const bool 
MasterProfile::validateContentLanguageEnabled() const
{
   return mValidateContentLanguageEnabled;   
}

bool& 
MasterProfile::validateAcceptEnabled()
{
   return mValidateAcceptEnabled;   
}

const bool 
MasterProfile::validateAcceptEnabled() const
{
   return mValidateAcceptEnabled;   
}

const bool 
MasterProfile::allowBadRegistrationEnabled() const
{
   return mAllowBadRegistrationEnabled;   
}

bool& 
MasterProfile::allowBadRegistrationEnabled()
{
   return mAllowBadRegistrationEnabled;   
}

const bool 
MasterProfile::checkReqUriInMergeDetectionEnabled() const
{
   return mCheckReqUriInMergeDetectionEnabled;   
}

bool& 
MasterProfile::checkReqUriInMergeDetectionEnabled()
{
   return mCheckReqUriInMergeDetectionEnabled;   
}

UserProfile*
MasterProfile::clone() const
{
   return new MasterProfile(*this);
}

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

