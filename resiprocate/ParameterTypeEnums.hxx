#ifndef ParameterTypeEnums_hxx
#define ParameterTypeEnums_hxx

#include "resiprocate/util/Data.hxx"

#undef UNUSED
#define UNUSED(_enum, _type) _enum = UNKNOWN
#define used(_enum, _type) _enum

namespace Vocal2
{

class Parameter;
class ParseBuffer;

class ParameterTypes
{
  
   public:
      // !dlb! until automated, must ensure that this set is consistent with
      // gperf in ParameterTypes.cxx and ParameterTypes.hxx
      enum Type
      {
         UNKNOWN = -1,
         transport,
         user,
         method,
         ttl,
         maddr,
         lr,
         q,
         purpose,
         expires,
         handling,
         tag,
         toTag,
         fromTag,
         duration,
         branch,
         received,
         mobility,

         comp,
         rport,

         algorithm,
         cnonce,
         domain,
         id,
         nonce,
         nc,
         opaque,
         realm,
         response,
         stale,
         username,
         qop,
         qopOptions,
         qopFactory,
         uri,
         retryAfter,
         reason,

         dAlg,
         dQop,
         dVer,

         smimeType,
         name,
         filename,
         protocol,
         micalg,
         boundary,
         expiration,
         size,
         permission,
         site,
         directory,
         mode,
         server,
         charset,
         accessType,

         MAX_PARAMETER
      };

      // convert to enum from two pointers into the HFV raw buffer
      static Type getType(const char* start, unsigned int length);

      typedef Parameter* (*Factory)(ParameterTypes::Type, ParseBuffer&, const char*);

      static Factory ParameterFactories[MAX_PARAMETER];
      static Data ParameterNames[MAX_PARAMETER];
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
