#ifndef ParameterTypes_hxx
#define ParameterTypes_hxx

#include "resiprocate/BranchParameter.hxx"
#include "resiprocate/DataParameter.hxx"
#include "resiprocate/QuotedDataParameter.hxx"
#include "resiprocate/QopParameter.hxx"
#include "resiprocate/IntegerParameter.hxx"
#include "resiprocate/FloatParameter.hxx"
#include "resiprocate/ExistsParameter.hxx"
#include "resiprocate/ParameterTypeEnums.hxx"
#include "resiprocate/RportParameter.hxx"
#include "resiprocate/Symbols.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)      \
   class _enum##_Param : public ParamBase                       \
   {                                                            \
      public:                                                   \
         typedef _type Type;                                    \
         typedef _type::Type DType;                             \
         virtual ParameterTypes::Type getTypeNum() const;       \
         _enum##_Param();                                       \
   };                                                           \
   extern _enum##_Param p_##_enum

namespace Vocal2
{

   class ParamBase
   {
      public:
         virtual ParameterTypes::Type getTypeNum() const = 0;
   };

   defineParam(accessType, "access-type", DataParameter, "RFC 2046");
   defineParam(algorithm, "algorithm", DataParameter, "RFC ????");
   defineParam(boundary, "boundary", DataParameter, "RFC 2046");
   defineParam(branch, "branch", BranchParameter, "RFC ????");
   defineParam(charset, "charset", DataParameter, "RFC 2045");
   defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC ????");
   defineParam(comp, "comp", DataParameter, "RFC ????");
   defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
   defineParam(dQop, "d-qop", DataParameter, "RFC ????");
   defineParam(dVer, "d-ver", QuotedDataParameter, "RFC ????");
   defineParam(directory, "directory", DataParameter, "RFC 2046");
   defineParam(domain, "domain", QuotedDataParameter, "RFC ????");
   defineParam(duration, "duration", IntegerParameter, "RFC ????");
   defineParam(expiration, "expiration", IntegerParameter, "RFC 2046");
   defineParam(expires, "expires", IntegerParameter, "RFC ????");
   defineParam(filename, "filename", DataParameter, "RFC ????");
   defineParam(fromTag, "from-tag", DataParameter, "RFC ????");
   defineParam(handling, "handling", DataParameter, "RFC ????");
   defineParam(id, "id", DataParameter, "RFC ????");
   defineParam(lr, "lr", ExistsParameter, "RFC ????");
   defineParam(maddr, "maddr", DataParameter, "RFC ????");
   defineParam(method, "method", DataParameter, "RFC ????");
   defineParam(micalg, "micalg", DataParameter, "RFC 1847");
   defineParam(mobility, "mobility", DataParameter, "RFC ????");
   defineParam(mode, "mode", DataParameter, "RFC 2046");
   defineParam(name, "name", DataParameter, "RFC 2046");
   defineParam(nc, "nc", DataParameter, "RFC ????");
   defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????");
   defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????");
   defineParam(permission, "permission", DataParameter, "RFC 2046");
   defineParam(protocol, "protocol", DataParameter, "RFC 1847");
   defineParam(purpose, "purpose", DataParameter, "RFC ????");
   defineParam(q, "q", FloatParameter, "RFC ????");
   defineParam(realm, "realm", QuotedDataParameter, "RFC ????");
   defineParam(reason, "reason", DataParameter, "RFC ????");
   defineParam(received, "received", DataParameter, "RFC ????");
   defineParam(response, "response", QuotedDataParameter, "RFC ????");
   defineParam(retryAfter, "retry-after", IntegerParameter, "RFC ????");
   defineParam(rport, "rport", RportParameter, "RFC ????");
   defineParam(server, "server", DataParameter, "RFC 2046");
   defineParam(site, "site", DataParameter, "RFC 2046");
   defineParam(size, "size", DataParameter, "RFC 2046");
   defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
   defineParam(stale, "stale", DataParameter, "RFC ????");
   defineParam(tag, "tag", DataParameter, "RFC ????");
   defineParam(toTag, "to-tag", DataParameter, "RFC ????");
   defineParam(transport, "transport", DataParameter, "RFC ????");
   defineParam(ttl, "ttl", IntegerParameter, "RFC ????");
   defineParam(uri, "uri", QuotedDataParameter, "RFC ????");
   defineParam(user, "user", DataParameter, "RFC ????");
   defineParam(username, "username", DataParameter, "RFC ????");

   // SPECIAL-CASE
   class Qop_Options_Param : public ParamBase
   {
      public:
         typedef QuotedDataParameter Type;
         typedef QuotedDataParameter::Type DType;
         virtual ParameterTypes::Type getTypeNum() const;
         Qop_Options_Param();
   };
   extern Qop_Options_Param p_qopOptions;

   class qop_Param : public ParamBase
   {
      public:
         typedef DataParameter Type;
         typedef DataParameter::Type DType;
         virtual ParameterTypes::Type getTypeNum() const;
         qop_Param();
   };
   extern qop_Param p_qop;

   class Qop_Factory_Param
   {
      public:
         typedef QopParameter Type;
         Qop_Factory_Param();
   };
   extern Qop_Factory_Param p_qopFactory;
}

#undef defineParam
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
