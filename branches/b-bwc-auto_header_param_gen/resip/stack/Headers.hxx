#if !defined(RESIP_HEADERS_HXX)
#define RESIP_HEADERS_HXX 

#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Data.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{
class SipMessage;
class HeaderFieldValueList;

class HeaderBase
{
   public:
      virtual ~HeaderBase() {}
      virtual Headers::Type getTypeNum() const = 0;
      virtual void merge(SipMessage&, const SipMessage&)=0;
      
      static HeaderBase* getInstance(Headers::Type typenum)
      {
         return theHeaderInstances[typenum+1];
      }
      
      virtual ParserContainerBase* makeContainer(HeaderFieldValueList* hfvs) const=0;
   protected:
      static HeaderBase* theHeaderInstances[Headers::MAX_HEADERS+1];
};

#include "resip/stack/Headers.hxx.ixx"

typedef ParserContainer<Token> Tokens;
// explicitly declare to avoid h_AllowEventss, ugh
extern H_AllowEventss h_AllowEvents;
// explicitly declare to avoid h_SecurityVerifys, ugh
extern H_SecurityVerifys h_SecurityVerifies;
// explicitly declare to avoid h_Privacys
extern H_Privacys h_Privacies;
typedef ParserContainer<Mime> Mimes;
typedef ParserContainer<GenericUri> GenericUris;
typedef ParserContainer<NameAddr> NameAddrs;
// explicitly declare to avoid h_PAssertedIdentitys
extern H_PPreferredIdentitys h_PPreferredIdentities;
// explicitly declare to avoid h_PAssertedIdentitys
extern H_PAssertedIdentitys h_PAssertedIdentities;
typedef ParserContainer<StringCategory> StringCategories;
typedef ParserContainer<UInt32Category> UInt32Categories;
typedef H_CallID H_CallId; // code convention compatible
extern H_CallId h_CallId; // code convention compatible
typedef ParserContainer<Auth> Auths;
typedef ParserContainer<Via> Vias;


//====================
// special first line accessors
//====================
class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;

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
