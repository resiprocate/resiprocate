#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include "resip/stack/UriParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/GenericUri.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

UriParameter::UriParameter(ParameterTypes::Type type,
                             ParseBuffer& pb,
                             const std::bitset<256>& terminators)
   : Parameter(type),
     mUri(new GenericUri)
{
   pb.skipWhitespace();
   pb.skipChar(Symbols::EQUALS[0]);
   pb.skipWhitespace();
   if(terminators[(unsigned char)(*pb.position())]) // handle cases such as ;info=
   {
      throw ParseException("Empty value in uri parameter.",
                           "UriParameter",
                           __FILE__,__LINE__);
   }
   mUri->parse(pb);
}

UriParameter::UriParameter(ParameterTypes::Type type)
   : Parameter(type),
     mUri(new GenericUri)
{
}

UriParameter::UriParameter(const UriParameter& other)
   : Parameter(other),
     mUri(new GenericUri(*other.mUri))
{
}

UriParameter::~UriParameter()
{
   delete mUri;
}

Parameter* 
UriParameter::clone() const
{
   return new UriParameter(*this);
}

EncodeStream& 
UriParameter::encode(EncodeStream& stream) const
{
   return stream << getName() << Symbols::EQUALS << *mUri;
}

UriParameter::Type&
UriParameter::value() { return *mUri; }


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
