
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/HeaderTypes.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


MasterProfile::MasterProfile() : 
   mValidateContentEnabled(true),
   mValidateContentLanguageEnabled(false),
   mValidateAcceptEnabled(true)
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
      // if this option is not supported
      if (!mSupportedOptionTags.find(*i))
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

void 
MasterProfile::removeSupportedMimeType(const MethodTypes& method, const Mime& mimeType)
{
   Mimes& mimes = mSupportedMimeTypes[method];
   for(Mimes::iterator it = mimes.begin(); it != mimes.end(); ++it)
   {
      if(*it == mimeType)
      {
         mimes.erase(it);
      }
   }
}

bool 
MasterProfile::isMimeTypeSupported(const MethodTypes& method, const Mime& mimeType)
{
   return mSupportedMimeTypes[method].find(mimeType);
}

Mimes 
MasterProfile::getSupportedMimeTypes(const MethodTypes& method)
{
   return mSupportedMimeTypes[method];
}

void 
MasterProfile::clearSupportedMimeTypes(const MethodTypes& method)
{
   mSupportedMimeTypes[method].clear();
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
   return mSupportedEncodings.find(encoding);
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
      if (mSupportedLanguages.find(*i) == false)
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
   return mValidateContentEnabled;   
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
