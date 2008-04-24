#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Data.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/SipMessage.hxx"

// GPERF generated external routines
#include "resip/stack/HeaderHash.hxx"

#include <iostream>
using namespace std;

//int strcasecmp(const char*, const char*);
//int strncasecmp(const char*, const char*, int len);

using namespace resip;

Data Headers::HeaderNames[MAX_HEADERS+1];
bool Headers::CommaTokenizing[] = {false};
bool Headers::CommaEncoding[] = {false};
bool Headers::Multi[]={false};
HeaderBase* HeaderBase::theHeaderInstances[] = {0};

bool 
Headers::isCommaTokenizing(Type type)
{
   return CommaTokenizing[type+1];
}

bool 
Headers::isCommaEncoding(Type type)
{
   return CommaEncoding[type+1];
}

const Data&
Headers::getHeaderName(int type)
{
   return HeaderNames[type+1];
}

bool
Headers::isMulti(Type type)
{
   return Multi[type+1];
}

// explicitly declare to avoid h_AllowEventss, ugh
H_AllowEventss resip::h_AllowEvents;
// explicitly declare to avoid h_SecurityVerifys, ugh
H_SecurityVerifys resip::h_SecurityVerifies;
// explicitly declare to avoid h_Privacys
H_Privacys resip::h_Privacies;
// Why do we have a typedef in a cxx file?
typedef ParserContainer<Mime> Mimes;
// Why do we have a typedef in a cxx file?
typedef ParserContainer<GenericUri> GenericUris;
// Why do we have a typedef in a cxx file?
typedef ParserContainer<NameAddr> NameAddrs;
// explicitly declare to avoid h_PPrefferedIdentitys
H_PPreferredIdentitys resip::h_PPreferredIdentities;
// explicitly declare to avoid h_PAssertedIdentitys
H_PAssertedIdentitys resip::h_PAssertedIdentities;
// Why do we have a typedef in a cxx file?
typedef ParserContainer<StringCategory> StringCategories;
H_CallId resip::h_CallId; // code convention compatible

#include "resip/stack/Headers.cxx.ixx"

//Enforces string encoding of extension headers
Headers::Type
H_RESIP_DO_NOT_USEs::getTypeNum() const {return Headers::RESIP_DO_NOT_USE;}

H_RESIP_DO_NOT_USEs::H_RESIP_DO_NOT_USEs()
{
   Headers::CommaTokenizing[Headers::RESIP_DO_NOT_USE+1] = bool(Type::value_type::commaHandling & ParserCategory::CommasAllowedOutputMulti);
   Headers::CommaEncoding[Headers::RESIP_DO_NOT_USE+1] = bool(Type::value_type::commaHandling & 2);
   Headers::HeaderNames[Headers::RESIP_DO_NOT_USE+1] = "RESIP_DO_NOT_USE";
}

ParserContainer<StringCategory>&
H_RESIP_DO_NOT_USEs::knownReturn(ParserContainerBase* container)
{
   return *dynamic_cast<ParserContainer<StringCategory>*>(container);
}

H_RESIP_DO_NOT_USEs resip::h_RESIP_DO_NOT_USEs;

void H_RESIP_DO_NOT_USEs::merge(SipMessage& target, const SipMessage& embedded)
{
}

ParserContainerBase*
H_RESIP_DO_NOT_USEs::makeContainer(HeaderFieldValueList* hfvs) const
{
   return new ParserContainer<StringCategory>(hfvs,Headers::RESIP_DO_NOT_USE);
}

RequestLineType resip::h_RequestLine;
StatusLineType resip::h_StatusLine;

Headers::Type
Headers::getType(const char* name, int len)
{
   struct headers* p;
   p = HeaderHash::in_word_set(name, len);
   return p ? Headers::Type(p->type) : Headers::UNKNOWN;
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
