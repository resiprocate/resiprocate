#if !defined(RESIP_PARAMETERTYPES_HXX)
#define RESIP_PARAMETERTYPES_HXX 

#include "resip/stack/BranchParameter.hxx"
#include "resip/stack/DataParameter.hxx"
#include "resip/stack/QuotedDataParameter.hxx"
#include "resip/stack/QopParameter.hxx"
#include "resip/stack/IntegerParameter.hxx"
#include "resip/stack/UInt32Parameter.hxx"
#include "resip/stack/QValueParameter.hxx"
#include "resip/stack/ExistsParameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "resip/stack/RportParameter.hxx"
#include "resip/stack/Symbols.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
   class _enum##_Param : public ParamBase                   \
   {                                                        \
     public:                                                \
      typedef _type Type;                                   \
      typedef _type::Type DType;                            \
      virtual ParameterTypes::Type getTypeNum() const;      \
      _enum##_Param();                                      \
   };                                                       \
   extern _enum##_Param p_##_enum

namespace resip
{

class ParamBase
{
   public:
      virtual ~ParamBase() {}
      virtual ParameterTypes::Type getTypeNum() const = 0;
};

defineParam(data, "data", ExistsParameter, "RFC 3840");
defineParam(control, "control", ExistsParameter, "RFC 3840");
defineParam(mobility, "mobility", QuotedDataParameter, "RFC 3840"); // mobile|fixed
defineParam(description, "description", QuotedDataParameter, "RFC 3840"); // <> quoted
defineParam(events, "events", QuotedDataParameter, "RFC 3840"); // list
defineParam(priority, "priority", QuotedDataParameter, "RFC 3840"); // non-urgent|normal|urgent|emergency
defineParam(methods, "methods", QuotedDataParameter, "RFC 3840"); // list
defineParam(schemes, "schemes", QuotedDataParameter, "RFC 3840"); // list
defineParam(application, "application", ExistsParameter, "RFC 3840");
defineParam(video, "video", ExistsParameter, "RFC 3840");
defineParam(language, "language", QuotedDataParameter, "RFC 3840"); // list
defineParam(type, "type", QuotedDataParameter, "RFC 3840"); // list
defineParam(isFocus, "isfocus", ExistsParameter, "RFC 3840");
defineParam(actor, "actor", QuotedDataParameter, "RFC 3840"); // principal|msg-taker|attendant|information
defineParam(text, "text", ExistsParameter, "RFC 3840");
defineParam(extensions, "extensions", QuotedDataParameter, "RFC 3840"); //list
defineParam(Instance, "+sip.instance", QuotedDataParameter, "gruu");  // <> quoted
defineParam(FlowId, "+sip.flowId", UInt32Parameter, "outbound");
defineParam(gruu, "gruu", QuotedDataParameter, "gruu");

defineParam(accessType, "access-type", DataParameter, "RFC 2046");
defineParam(algorithm, "algorithm", DataParameter, "RFC 2617");
defineParam(boundary, "boundary", DataParameter, "RFC 2046");
defineParam(branch, "branch", BranchParameter, "RFC 3261");
defineParam(charset, "charset", DataParameter, "RFC 2045");
defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC 2617");
defineParam(comp, "comp", DataParameter, "RFC 3486");
defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
defineParam(dQop, "d-qop", DataParameter, "RFC ????");
defineParam(dVer, "d-ver", QuotedDataParameter, "RFC ????");
defineParam(directory, "directory", DataParameter, "RFC 2046");
defineParam(domain, "domain", QuotedDataParameter, "RFC ????");
defineParam(duration, "duration", UInt32Parameter, "RFC ????");
defineParam(expiration, "expiration", QuotedDataParameter, "RFC 2046");
defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
defineParam(filename, "filename", DataParameter, "RFC ????");
defineParam(fromTag, "from-tag", DataParameter, "RFC ????");
defineParam(handling, "handling", DataParameter, "RFC ????");
defineParam(id, "id", DataParameter, "RFC ????");
defineParam(lr, "lr", ExistsParameter, "RFC 3261");
defineParam(maddr, "maddr", DataParameter, "RFC 3261");
defineParam(method, "method", DataParameter, "RFC ????");
defineParam(micalg, "micalg", DataParameter, "RFC 1847");
defineParam(mode, "mode", DataParameter, "RFC 2046");
defineParam(name, "name", DataParameter, "RFC 2046");
defineParam(nc, "nc", DataParameter, "RFC 2617");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC 2617");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC 2617");
defineParam(permission, "permission", DataParameter, "RFC 2046");
defineParam(protocol, "protocol", QuotedDataParameter, "RFC 1847");
defineParam(purpose, "purpose", DataParameter, "RFC ????");
defineParam(q, "q", QValueParameter, "RFC 3261");
defineParam(realm, "realm", QuotedDataParameter, "RFC 2617");
defineParam(reason, "reason", DataParameter, "RFC ????");
defineParam(received, "received", DataParameter, "RFC 3261");
defineParam(response, "response", QuotedDataParameter, "RFC ????");
defineParam(retryAfter, "retry-after", UInt32Parameter, "RFC 3261");
defineParam(rinstance, "rinstance", DataParameter, "");
defineParam(rport, "rport", RportParameter, "RFC 3261");
defineParam(server, "server", DataParameter, "RFC 2046");
defineParam(site, "site", DataParameter, "RFC 2046");
defineParam(size, "size", DataParameter, "RFC 2046");
defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
defineParam(stale, "stale", DataParameter, "RFC 2617");
defineParam(tag, "tag", DataParameter, "RFC 3261");
defineParam(toTag, "to-tag", DataParameter, "RFC ????");
defineParam(transport, "transport", DataParameter, "RFC 3261");
defineParam(ttl, "ttl", UInt32Parameter, "RFC ????");
defineParam(uri, "uri", QuotedDataParameter, "RFC ????");
defineParam(user, "user", DataParameter, "RFC ????");
defineParam(username, "username", QuotedDataParameter, "RFC 3261");
defineParam(earlyOnly, "early-only", ExistsParameter, "RFC 3891");
defineParam(refresher, "refresher", DataParameter, "RFC 4028");

defineParam(profileType, "profile-type", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(vendor, "vendor", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(model, "model", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(version, "version", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(effectiveBy, "effective-by", UInt32Parameter, "draft-ietf-sipping-config-framework");
defineParam(document, "document", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(appId, "app-id", DataParameter, "draft-ietf-sipping-config-framework");
defineParam(networkUser, "network-user", DataParameter, "draft-ietf-sipping-config-framework");

defineParam(url, "url", QuotedDataParameter, "RFC 4483");

defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "draft-ietf-rohc-sigcomp-sip");
defineParam(qop,"qop",DataParameter,"RFC3261");
defineParam(qopOptions,"qop",DataParameter,"RFC3261");

// Internal use only
defineParam(addTransport, "addTransport", ExistsParameter, "RESIP INTERNAL");

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
