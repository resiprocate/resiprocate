#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/ParameterTypes.hxx"
#include "rutil/compat.hxx"
#include <iostream>

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
ParameterTypes::Type                                                            \
_enum##_Param::getTypeNum() const {return ParameterTypes::_enum;}               \
_enum##_Param::_enum##_Param()                                                  \
{                                                                               \
   ParameterTypes::ParameterFactories[ParameterTypes::_enum] = Type::decode;    \
   ParameterTypes::ParameterNames[ParameterTypes::_enum] = _name;               \
}                                                                               \
_enum##_Param resip::p_##_enum

int strncasecmp(char*,char*,int);

using namespace std;

using namespace resip;

ParameterTypes::Factory ParameterTypes::ParameterFactories[ParameterTypes::MAX_PARAMETER] = {0};
Data ParameterTypes::ParameterNames[ParameterTypes::MAX_PARAMETER] = {"PARAMETER?"};

defineParam(data, "data", ExistsParameter, "callee-caps");
defineParam(control, "control", ExistsParameter, "callee-caps");
defineParam(mobility, "mobility", QuotedDataParameter, "callee-caps"); // mobile|fixed
defineParam(description, "description", QuotedDataParameter, "callee-caps"); // <> quoted
defineParam(events, "events", QuotedDataParameter, "callee-caps"); // list
defineParam(priority, "priority", QuotedDataParameter, "callee-caps"); // non-urgent|normal|urgent|emergency
defineParam(methods, "methods", QuotedDataParameter, "callee-caps"); // list
defineParam(schemes, "schemes", QuotedDataParameter, "callee-caps"); // list
defineParam(application, "application", ExistsParameter, "callee-caps");
defineParam(video, "video", ExistsParameter, "callee-caps");
defineParam(language, "language", QuotedDataParameter, "callee-caps"); // list
defineParam(type, "type", QuotedDataParameter, "callee-caps"); // list
defineParam(isFocus, "isfocus", ExistsParameter, "callee-caps");
defineParam(actor, "actor", QuotedDataParameter, "callee-caps"); // principal|msg-taker|attendant|information
defineParam(text, "text", ExistsParameter, "callee-caps");
defineParam(extensions, "extensions", QuotedDataParameter, "callee-caps"); //list
defineParam(Instance, "+sip.instance", QuotedDataParameter, "gruu");  // <> quoted
defineParam(regid, "reg-id", UInt32Parameter, "outbound");
defineParam(ob,"ob",ExistsParameter,"outbound-05");
defineParam(gruu, "gruu", QuotedDataParameter, "gruu");

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
defineParam(duration, "duration", UInt32Parameter, "RFC ????");
defineParam(expiration, "expiration", QuotedDataParameter, "RFC 2046");
defineParam(expires, "expires", UInt32Parameter, "RFC ????");
defineParam(filename, "filename", DataParameter, "RFC ????");
defineParam(fromTag, "from-tag", DataParameter, "RFC ????");
defineParam(handling, "handling", DataParameter, "RFC ????");
defineParam(id, "id", DataParameter, "RFC ????");
defineParam(lr, "lr", ExistsParameter, "RFC ????");
defineParam(maddr, "maddr", DataParameter, "RFC ????");
defineParam(method, "method", DataParameter, "RFC ????");
defineParam(micalg, "micalg", DataParameter, "RFC 1847");
defineParam(mode, "mode", DataParameter, "RFC 2046");
defineParam(name, "name", DataParameter, "RFC 2046");
defineParam(nc, "nc", DataParameter, "RFC ????");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????");
defineParam(permission, "permission", DataParameter, "RFC 2046");
defineParam(protocol, "protocol", QuotedDataParameter, "RFC 1847");
defineParam(purpose, "purpose", DataParameter, "RFC ????");
defineParam(q, "q", QValueParameter, "RFC ????");
defineParam(realm, "realm", QuotedDataParameter, "RFC ????");
defineParam(reason, "reason", DataParameter, "RFC ????");
defineParam(received, "received", DataParameter, "RFC ????");
defineParam(response, "response", QuotedDataParameter, "RFC ????");
defineParam(retryAfter, "retry-after", UInt32Parameter, "RFC ????");
defineParam(rinstance, "rinstance", DataParameter, "");
defineParam(rport, "rport", RportParameter, "RFC ????");
defineParam(server, "server", DataParameter, "RFC 2046");
defineParam(site, "site", DataParameter, "RFC 2046");
defineParam(size, "size", DataParameter, "RFC 2046");
defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
defineParam(stale, "stale", DataParameter, "RFC ????");
defineParam(tag, "tag", DataParameter, "RFC ????");
defineParam(toTag, "to-tag", DataParameter, "RFC ????");
defineParam(transport, "transport", DataParameter, "RFC ????");
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

defineParam(url, "url", QuotedDataParameter, "draft-ietf-sip-content-indirect-mech-05");

defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "draft-ietf-rohc-sigcomp-sip");

defineParam(addTransport, "addTransport", ExistsParameter, "");

// SPECIAL-CASE
ParameterTypes::Type
Qop_Options_Param::getTypeNum() const {return ParameterTypes::qopOptions;}
Qop_Options_Param::Qop_Options_Param()
{
   ParameterTypes::ParameterNames[ParameterTypes::qopOptions] = "qop";
}
Qop_Options_Param resip::p_qopOptions;

ParameterTypes::Type
qop_Param::getTypeNum() const {return ParameterTypes::qop;}
qop_Param:: qop_Param()
{
   ParameterTypes::ParameterNames[ParameterTypes::qop] = "qop";
}
qop_Param resip::p_qop;

Qop_Factory_Param::Qop_Factory_Param()
{
   ParameterTypes::ParameterFactories[ParameterTypes::qopFactory] = Type::decode;
   ParameterTypes::ParameterNames[ParameterTypes::qopFactory] = "qop";
}
Qop_Factory_Param resip::p_qopFactory;

#include "resip/stack/ParameterHash.hxx"

ParameterTypes::Type
ParameterTypes::getType(const char* pname, unsigned int len)
{
   struct params* p;
   p = ParameterHash::in_word_set(pname, len);
   return p ? p->type : ParameterTypes::UNKNOWN;
   
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
