#if !defined(RESIP_PARAMETERTYPEENUMS_HXX)
#define RESIP_PARAMETERTYPEENUMS_HXX 

#include "resiprocate/os/Data.hxx"

#define defineParam(_enum, _name, _type, _rfc) _enum
#define UNUSED_defineParam(_enum, _name, _type, _rfc) SAVE##_enum, _enum = UNKNOWN, RESET##enum = SAVE##_enum-1

namespace resip
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
         defineParam(accessType, "access-type", DataParameter, "RFC 2046"),
         defineParam(algorithm, "algorithm", DataParameter, "RFC ????"),
         defineParam(boundary, "boundary", DataParameter, "RFC 2046"),
         defineParam(branch, "branch", BranchParameter, "RFC ????"),
         defineParam(charset, "charset", DataParameter, "RFC 2045"),
         defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC ????"),
         defineParam(comp, "comp", DataParameter, "RFC ????"),
         defineParam(dAlg, "d-alg", DataParameter, "RFC 3329"),
         defineParam(dQop, "d-qop", DataParameter, "RFC ????"),
         defineParam(dVer, "d-ver", QuotedDataParameter, "RFC ????"),
         defineParam(directory, "directory", DataParameter, "RFC 2046"),
         defineParam(domain, "domain", QuotedDataParameter, "RFC ????"),
         defineParam(duration, "duration", IntegerParameter, "RFC ????"),
         defineParam(expiration, "expiration", IntegerParameter, "RFC 2046"),
         defineParam(expires, "expires", IntegerParameter, "RFC ????"),
         defineParam(filename, "filename", DataParameter, "RFC ????"),
         defineParam(fromTag, "from-tag", DataParameter, "RFC ????"),
         defineParam(handling, "handling", DataParameter, "RFC ????"),
         defineParam(id, "id", DataParameter, "RFC ????"),
         defineParam(lr, "lr", ExistsParameter, "RFC ????"),
         defineParam(maddr, "maddr", DataParameter, "RFC ????"),
         defineParam(method, "method", DataParameter, "RFC ????"),
         defineParam(micalg, "micalg", DataParameter, "RFC 1847"),
         defineParam(mobility, "mobility", DataParameter, "RFC ????"),
         defineParam(mode, "mode", DataParameter, "RFC 2046"),
         defineParam(name, "name", DataParameter, "RFC 2046"),
         defineParam(nc, "nc", DataParameter, "RFC ????"),
         defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????"),
         defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????"),
         defineParam(permission, "permission", DataParameter, "RFC 2046"),
         defineParam(protocol, "protocol", DataParameter, "RFC 1847"),
         defineParam(purpose, "purpose", DataParameter, "RFC ????"),
         defineParam(q, "q", FloatParameter, "RFC 3261"),

         defineParam(qop, "qopFactory", SPECIAL, "RFC 3261"),
         defineParam(qopOptions, "", IGNORE, "RFC 3261"),
         defineParam(qopFactory, "", IGNORE, "RFC 3261"),

         defineParam(realm, "realm", QuotedDataParameter, "RFC ????"),
         defineParam(reason, "reason", DataParameter, "RFC ????"),
         defineParam(received, "received", DataParameter, "RFC ????"),
         defineParam(response, "response", QuotedDataParameter, "RFC ????"),
         defineParam(retryAfter, "retry-after", IntegerParameter, "RFC ????"),
         defineParam(rport, "rport", RportParameter, "RFC ????"),
         defineParam(server, "server", DataParameter, "RFC 2046"),
         defineParam(site, "site", DataParameter, "RFC 2046"),
         defineParam(size, "size", DataParameter, "RFC 2046"),
         defineParam(smimeType, "smime-type", DataParameter, "RFC 2633"),
         defineParam(stale, "stale", DataParameter, "RFC ????"),
         defineParam(tag, "tag", DataParameter, "RFC ????"),
         defineParam(toTag, "to-tag", DataParameter, "RFC ????"),
         defineParam(transport, "transport", DataParameter, "RFC ????"),
         defineParam(ttl, "ttl", IntegerParameter, "RFC ????"),
         defineParam(uri, "uri", QuotedDataParameter, "RFC ????"),
         defineParam(user, "user", DataParameter, "RFC ????"),
         defineParam(username, "username", DataParameter, "RFC ????"),

         MAX_PARAMETER
      };

      // convert to enum from two pointers into the HFV raw buffer
      static Type getType(const char* start, unsigned int length);

      typedef Parameter* (*Factory)(ParameterTypes::Type, ParseBuffer&, const char*);

      static Factory ParameterFactories[MAX_PARAMETER];
      static Data ParameterNames[MAX_PARAMETER];
};
 
}

#undef defineParam
#undef UNUSED_defineParam

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
