#if !defined(RESIP_PARAMETERTYPES_HXX)
#define RESIP_PARAMETERTYPES_HXX 

#include "resip/stack/BranchParameter.hxx"
#include "resip/stack/DataParameter.hxx"
#include "resip/stack/ExistsOrDataParameter.hxx"
#include "resip/stack/QuotedDataParameter.hxx"
#include "resip/stack/IntegerParameter.hxx"
#include "resip/stack/UInt32Parameter.hxx"
#include "resip/stack/QValueParameter.hxx"
#include "resip/stack/ExistsParameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "resip/stack/RportParameter.hxx"
#include "resip/stack/Symbols.hxx"


#define defineParam(_enum, _name, _type, _headertype, _RFC_ref_ignored)  \
   class _enum##_Param : public Param<_headertype>                   \
   {                                                        \
     public:                                                \
      typedef _type Type;                                   \
      typedef _type::Type DType;                            \
      virtual ParameterTypes::Type getTypeNum() const;      \
      virtual const char* name() const { return _name; }    \
      _enum##_Param();                                      \
   };                                                       \
   extern _enum##_Param p_##_enum

#define defineParam2(_enum, _name, _type, _headertype, _headertype2, _RFC_ref_ignored)  \
   class _enum##_Param : public Param<_headertype>, public Param<_headertype2> \
   {                                                        \
     public:                                                \
      typedef _type Type;                                   \
      typedef _type::Type DType;                            \
      virtual ParameterTypes::Type getTypeNum() const;      \
      virtual const char* name() const { return _name; }    \
      _enum##_Param();                                      \
   };                                                       \
   extern _enum##_Param p_##_enum

// .bwc. So far, I have not discovered any parameter types that are defined for
// more than three ParserCategories. This may change. If so, we just create
// another macro with one more _headertype field.
#define defineParam3(_enum, _name, _type, _headertype, _headertype2, _headertype3, _RFC_ref_ignored)  \
   class _enum##_Param : public Param<_headertype>, public Param<_headertype2>, public Param<_headertype3> \
   {                                                        \
     public:                                                \
      typedef _type Type;                                   \
      typedef _type::Type DType;                            \
      virtual ParameterTypes::Type getTypeNum() const;      \
      virtual const char* name() const { return _name; }    \
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
      virtual const char* name() const = 0;
};

template<class T>
class Param : virtual public ParamBase
{};

class Auth;
class CSeqCategory;
class CallID;
class DateCategory;
class ExpiresCategory;
class GenericUri;
class IntegerCategory;
class UInt32Category;
class Mime;
class NameAddr;
class PrivacyCategory;
class RAckCategory;
class RequestLine;
class StatusLine;
class StringCategory;
class Token;
class Uri;
class Via;
class WarningCategory;
class TokenOrQuotedStringCategory;

defineParam(data, "data", ExistsParameter, NameAddr, "RFC 3840");
defineParam(control, "control", ExistsParameter, NameAddr, "RFC 3840");
defineParam(mobility, "mobility", QuotedDataParameter, NameAddr, "RFC 3840"); // mobile|fixed
defineParam(description, "description", QuotedDataParameter, NameAddr, "RFC 3840"); // <> quoted
defineParam(events, "events", QuotedDataParameter, NameAddr, "RFC 3840"); // list
defineParam(priority, "priority", QuotedDataParameter, NameAddr, "RFC 3840"); // non-urgent|normal|urgent|emergency
defineParam(methods, "methods", QuotedDataParameter, NameAddr, "RFC 3840"); // list
defineParam(schemes, "schemes", QuotedDataParameter, NameAddr, "RFC 3840"); // list
defineParam(application, "application", ExistsParameter, NameAddr, "RFC 3840");
defineParam(video, "video", ExistsParameter, NameAddr, "RFC 3840");
defineParam(language, "language", QuotedDataParameter, NameAddr, "RFC 3840"); // list
defineParam(type, "type", QuotedDataParameter, NameAddr, "RFC 3840"); // list
defineParam(isFocus, "isfocus", ExistsParameter, NameAddr, "RFC 3840");
defineParam(actor, "actor", QuotedDataParameter, NameAddr, "RFC 3840"); // principal|msg-taker|attendant|information
defineParam2(text, "text", ExistsOrDataParameter, NameAddr, Token, "RFC 3326/3840");
defineParam(cause, "cause", UInt32Parameter, Token, "RFC3326");
defineParam(extensions, "extensions", QuotedDataParameter, NameAddr, "RFC 3840"); //list
defineParam(Instance, "+sip.instance", QuotedDataParameter, NameAddr, "RFC 5626");  // <> quoted
defineParam(regid, "reg-id", UInt32Parameter, NameAddr, "RFC 5626");
defineParam(ob,"ob",ExistsParameter, Uri, "RFC 5626");
defineParam(pubGruu, "pub-gruu", QuotedDataParameter, NameAddr, "RFC 5627");
defineParam(tempGruu, "temp-gruu", QuotedDataParameter, NameAddr, "RFC 5627");
defineParam(gr, "gr", ExistsOrDataParameter, Uri, "RFC 5627");

defineParam(accessType, "access-type", DataParameter, Mime, "RFC 2046");
defineParam(algorithm, "algorithm", DataParameter, Auth, "RFC 2617");
defineParam(boundary, "boundary", DataParameter, Mime, "RFC 2046");
defineParam(branch, "branch", BranchParameter, Via, "RFC 3261");
defineParam(charset, "charset", DataParameter, Mime, "RFC 2045");
defineParam(cnonce, "cnonce", QuotedDataParameter, Auth, "RFC 2617");
defineParam2(comp, "comp", DataParameter, Uri, Via, "RFC 3486");
defineParam(dAlg, "d-alg", DataParameter, Token, "RFC 3329");
defineParam(dQop, "d-qop", DataParameter, Token, "RFC 3329");
defineParam(dVer, "d-ver", QuotedDataParameter, Token, "RFC 3329");
defineParam(directory, "directory", DataParameter, Mime, "RFC 2046");
defineParam(domain, "domain", QuotedDataParameter, Auth, "RFC 3261");
defineParam2(duration, "duration", UInt32Parameter, Uri, UInt32Category, "RFC 4240");
defineParam(expiration, "expiration", QuotedDataParameter, Mime, "RFC 2046");
defineParam2(expires, "expires", UInt32Parameter, NameAddr, Token, "RFC 3261");
defineParam(filename, "filename", DataParameter, Token, "RFC 2183");
defineParam2(fromTag, "from-tag", DataParameter, Token, CallID, "RFC 4235");
defineParam(handling, "handling", DataParameter, Token, "RFC 3261");
defineParam(id, "id", DataParameter, Token, "RFC 3265");
defineParam(lr, "lr", ExistsParameter, Uri, "RFC 3261");
defineParam2(maddr, "maddr", DataParameter, Uri, Via, "RFC 3261");
defineParam(method, "method", DataParameter, Uri, "RFC 3261");
defineParam(micalg, "micalg", DataParameter, Mime, "RFC 1847");
defineParam(mode, "mode", DataParameter, Mime, "RFC 2046");
defineParam(name, "name", DataParameter, Mime, "RFC 2046");
defineParam(nc, "nc", DataParameter, Auth, "RFC 2617");
defineParam(nonce, "nonce", QuotedDataParameter, Auth, "RFC 2617");
defineParam(opaque, "opaque", QuotedDataParameter, Auth, "RFC 2617");
defineParam(permission, "permission", DataParameter, Mime, "RFC 2046");
defineParam(protocol, "protocol", QuotedDataParameter, Mime, "RFC 1847");
defineParam2(purpose, "purpose", DataParameter, GenericUri, TokenOrQuotedStringCategory, "RFC 3261, draft-ietf-cuss-sip-uui-17");
defineParam3(q, "q", QValueParameter, NameAddr, Token, Mime, "RFC 3261");
defineParam(realm, "realm", QuotedDataParameter, Auth, "RFC 2617");
defineParam(reason, "reason", DataParameter, Token, "RFC 3265");
defineParam(received, "received", DataParameter, Via, "RFC 3261");
defineParam(require, "require", DataParameter, Token, "RFC 5373");
defineParam(response, "response", QuotedDataParameter, Auth, "RFC 3261");
defineParam(retryAfter, "retry-after", UInt32Parameter, Token, "RFC 3265");
defineParam(rinstance, "rinstance", DataParameter, Uri, "proprietary (resip)");
defineParam(rport, "rport", RportParameter, Via, "RFC 3581");
defineParam(server, "server", DataParameter, Mime, "RFC 2046");
defineParam(site, "site", DataParameter, Mime, "RFC 2046");
defineParam(size, "size", DataParameter, Mime, "RFC 2046");
defineParam(smimeType, "smime-type", DataParameter, Mime, "RFC 2633");
defineParam(stale, "stale", DataParameter, Auth, "RFC 2617");
defineParam(tag, "tag", DataParameter, NameAddr, "RFC 3261");
defineParam2(toTag, "to-tag", DataParameter, Token, CallID, "RFC 4235");
defineParam(transport, "transport", DataParameter, Uri, "RFC 3261");
defineParam2(ttl, "ttl", UInt32Parameter, Uri, Via, "RFC 3261");
defineParam(uri, "uri", QuotedDataParameter, Auth, "RFC 3261");
defineParam(user, "user", DataParameter, Uri, "RFC 3261, 4967");
defineParam2(extension, "ext", DataParameter, Uri, Token, "RFC 3966"); // Token is used when ext is a user-parameter
defineParam(username, "username", QuotedDataParameter, Auth, "RFC 3261");
defineParam(earlyOnly, "early-only", ExistsParameter, CallID, "RFC 3891");
defineParam(refresher, "refresher", DataParameter, ExpiresCategory, "RFC 4028");

defineParam(profileType, "profile-type", DataParameter, Token, "RFC 6080");
defineParam(vendor, "vendor", QuotedDataParameter, Token, "RFC 6080");
defineParam(model, "model", QuotedDataParameter, Token, "RFC 6080");
defineParam(version, "version", QuotedDataParameter, Token, "RFC 6080");
defineParam(effectiveBy, "effective-by", UInt32Parameter, Token, "RFC 6080");
defineParam(document, "document", DataParameter, Token, "draft-ietf-sipping-config-framework-07 (removed in 08)");
defineParam(appId, "app-id", DataParameter, Token, "draft-ietf-sipping-config-framework-05 (renamed to auid in 06, which was then removed in 08)");
defineParam(networkUser, "network-user", DataParameter, Token, "draft-ietf-sipping-config-framework-11 (removed in 12)");

defineParam(url, "url", QuotedDataParameter, Mime, "RFC 4483");

defineParam2(sigcompId, "sigcomp-id", QuotedDataParameter, Uri, Via, "RFC 5049");
defineParam(qop,"qop",DataParameter, Auth, "RFC 3261");

defineParam(index, "index", DataParameter, NameAddr, "RFC 4244");
defineParam(rc, "rc", DataParameter, NameAddr, "RFC 4244-bis");
defineParam(mp, "mp", DataParameter, NameAddr, "RFC 4244-bis");
defineParam(np, "np", DataParameter, NameAddr, "RFC 4244-bis");

defineParam(utranCellId3gpp, "utran-cell-id-3gpp", DataParameter, Token, "RFC 3455"); // P-Access-Network-Info
defineParam(cgi3gpp, "cgi-3gpp", DataParameter, Token, "RFC 3455"); // P-Access-Network-Info
defineParam(ccf, "ccf", DataParameter, Token, "RFC 3455"); // P-Charging-Function-Addresses
defineParam(ecf, "ecf", DataParameter, Token, "RFC 3455"); // P-Charging-Function-Addresses
defineParam(icidValue, "icid-value", DataParameter, Token, "RFC 3455"); // P-Charging-Vector
defineParam(icidGeneratedAt, "icid-generated-at", DataParameter, Token, "RFC 3455"); // P-Charging-Vector
defineParam(origIoi, "orig-ioi", DataParameter, Token, "RFC 3455"); // P-Charging-Vector
defineParam(termIoi, "term-ioi", DataParameter, Token, "RFC 3455"); // P-Charging-Vector

defineParam(content, "content", DataParameter, TokenOrQuotedStringCategory, "draft-ietf-cuss-sip-uui-17"); // User-to-User
defineParam(encoding, "encoding", DataParameter, TokenOrQuotedStringCategory, "draft-ietf-cuss-sip-uui-17"); // User-to-User

// Internal use only
defineParam(qopOptions,"qop",DataParameter, Auth, "RFC 3261");
defineParam(addTransport, "addTransport", ExistsParameter, Uri, "RESIP INTERNAL");
defineParam(wsSrcIp, "ws-src-ip", DataParameter, Uri, "RESIP INTERNAL (WebSocket)");
defineParam(wsSrcPort, "ws-src-port", UInt32Parameter, Uri, "RESIP INTERNAL (WebSocket)");

}

#undef defineParam
#undef defineParam2
#undef defineParam3
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
