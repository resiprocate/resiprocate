/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_PARSERCATEGORY_HXX)
#define RESIP_PARSERCATEGORY_HXX 

#include <iosfwd>
#include <vector>
#include <set>
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/LazyParser.hxx"
#include "resip/stack/ParameterTypes.hxx"
#include "rutil/Data.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/PoolAllocator.hxx"
#include "rutil/PoolBase.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType)

namespace resip
{
class UnknownParameter;
class ExtensionParameter;
class Parameter;
class ParseBuffer;

/**
   @brief Base class for all SIP grammar elements that can have parameters.

   The pattern for accessing the parameters in a ParserCategory is very similar
   to accessing headers in a SipMessage. Each parameter type has an access token
   class (a subclass of ParamBase), and a corresponding ParserCategory::param() 
   function that takes an instance of that subclass as an argument, and returns
   the correct type for that parameter. Common examples of access-token include 
   p_tag, p_q, p_lr, p_expires, p_branch, etc.

   @code
      NameAddr& contact = sip.header(h_Contacts).front();
      if(contact.exists(p_q))
      {
         QValueParameter& q = contact.param(p_q);
         // do stuff with q
      }

      NameAddr& to = sip.header(h_To);
      if(to.exists(p_tag))
      {
         DataParameter& toTag = to.param(p_tag);
         // do stuff with toTag
      }

      Via& topVia = sip.header(h_Vias).front();
      if(topVia.exists(p_branch))
      {
         BranchParameter& branch = topVia.param(p_branch);
         // do stuff with branch
      }
   @endcode

   Note the calls to ParserCategory::exists() in the code above; calling 
   ParserCategory::param() when the relevant parameter doesn't exist will either
   cause the parameter to be created, or an exception to be thrown (if you're
   working with a const reference).

   In some cases, you will need to access parameter-types that are not natively 
   supported by the stack (ie, don't have an access-token). ExtensionParameter 
   will allow you to construct an access-token at runtime that will retrieve the
   parameter as a raw Data. Here's an example:

   @code
      // We need to access the foo parameter on the Request-Uri
      RequestLine& rLine = sip.header(h_RequestLine);
      static ExtensionParameter p_foo("foo");
      if(rLine.uri().exists(p_foo))
      {
         Data& foo = rLine.uri().param(p_foo);
      }
   @endcode

   @todo Maybe a better name? IHaveParams? ElemWithParams?
   @ingroup resip_crit
*/
class ParserCategory : public LazyParser
{
    public:
      enum {UnknownParserCategory = -1};

      // NoCommaTokenizing:        commas do not indicate a new header value
      // CommasAllowedOutputMulti: multi headers can be received with commas but
      //                           output them on separate lines.
      // CommasAllowedOutputCommas: multi headers can be received with commas
      //                            and will always output with commas when
      //                            parsed.  
      enum {NoCommaTokenizing = 0, CommasAllowedOutputMulti = 1, CommasAllowedOutputCommas = 3};

      /**
         @internal
         @brief Constructor used by SipMessage. Unless you _really_ know what
            you're doing, don't touch this.
      */
      ParserCategory(const HeaderFieldValue& headerFieldValue, 
                     Headers::Type type,
                     PoolBase* pool=0);

      /**
         @internal
         @brief Constructor used by SipMessage. Unless you _really_ know what
            you're doing, don't touch this.
      */
      ParserCategory(const char* buf, 
                     int length, 
                     Headers::Type type,
                     PoolBase* pool=0);

      /**
         @internal
         @brief Copy c'tor.
      */
      ParserCategory(const ParserCategory& rhs,
                     PoolBase* pool=0);

      /**
         @internal
         @brief Assignment operator.
      */
      ParserCategory& operator=(const ParserCategory& rhs);

      virtual ~ParserCategory();

      virtual ParserCategory* clone() const = 0;

      // Do a placement new
      virtual ParserCategory* clone(void* location) const = 0;

      // Do a pool allocated new
      virtual ParserCategory* clone(PoolBase* pool) const = 0;

      /**
         @brief Checks for the existence of a natively supported parameter.
         @param paramType The accessor token for the parameter.
         @return true iff the parameter is present.
      */
      inline bool exists(const ParamBase& paramType) const
      {
          checkParsed();
          return (getParameterByEnum(paramType.getTypeNum()) != NULL);
      }

      /**
         @brief Removes a natively supported parameter, if the parameter is 
            present.
         @param paramType The accessor token for the parameter.
      */
      void remove(const ParamBase& paramType);

      // !dlb! causes compiler error in windows -- change template to const T*
      /**
         @brief Const accessor for non-natively-supported parameter types.
         @throw ParserCategory::Exception if this parameter doesn't exist.
         @param param The runtime constructed parameter accessor.
         @return The parameter, as a raw Data.
      */
      const Data& param(const ExtensionParameter& param) const;
      
      /**
         @brief Accessor for non-natively-supported parameter types.
            Will create the parameter if it does not exist.
         @param param The runtime constructed parameter accessor.
         @return The parameter, as a raw Data.
      */
      Data& param(const ExtensionParameter& param);

      /**
         @brief Removes a non-natively-supported parameter, if the parameter is 
            present.
         @param param The accessor token for the parameter.
      */
      void remove(const ExtensionParameter& param); 

      /**
         @brief Checks for the existence of a non-natively-supported parameter.
         @param param The runtime constructed accessor token.
         @return true iff the parameter is present.
      */
      bool exists(const ExtensionParameter& param) const;

      typedef std::set<ParameterTypes::Type> ParameterTypeSet;      
      
      static const ParameterTypeSet EmptyParameterTypeSet;      

      /**
         @brief Removes all known parameters except those that are specified in 
            set.
         @param set The set of parameters to retain, as an enum.
         @note This does not remove unknown parameters.
      */
      void removeParametersExcept(const ParameterTypeSet& set = EmptyParameterTypeSet);

      /**
         @brief Exception class used by ParserCategory.
      */
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "ParserCategory::Exception"; }
      };

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
      defineParam(text, "text", ExistsOrDataParameter, "callee-caps"); // using ExistsOrDataParameter so this parameter is compatible with both RFC3840 and RFC3326
      defineParam(extensions, "extensions", QuotedDataParameter, "callee-caps"); //list
      defineParam(Instance, "+sip.instance", QuotedDataParameter, "draft-ietf-sip-gruu");  // <> quoted
      defineParam(regid, "reg-id", UInt32Parameter, "outbound");
      defineParam(ob,"ob",ExistsParameter,"outbound-05");

      defineParam(pubGruu, "pub-gruu", QuotedDataParameter, "gruu");
      defineParam(tempGruu, "temp-gruu", QuotedDataParameter, "gruu");
      defineParam(gr, "gr", ExistsOrDataParameter, "gruu");

      defineParam(accessType, "access-type", DataParameter, "RFC 2046");
      defineParam(algorithm, "algorithm", DataParameter, "RFC 2617");
      defineParam(boundary, "boundary", DataParameter, "RFC 2046");
      defineParam(branch, "branch", BranchParameter, "RFC 3261");
      defineParam(charset, "charset", DataParameter, "RFC 2045");
      defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC 2617");
      defineParam(comp, "comp", DataParameter, "RFC 3486");
      defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
      defineParam(dQop, "d-qop", DataParameter, "RFC 3329");
      defineParam(dVer, "d-ver", QuotedDataParameter, "RFC 3329");
      defineParam(directory, "directory", DataParameter, "RFC 2046");
      defineParam(domain, "domain", QuotedDataParameter, "RFC 3261");
      defineParam(duration, "duration", UInt32Parameter, "RFC 4240");
      defineParam(expiration, "expiration", QuotedDataParameter, "RFC 2046");
      defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
      defineParam(filename, "filename", DataParameter, "proprietary");
      defineParam(fromTag, "from-tag", DataParameter, "RFC 4235");
      defineParam(handling, "handling", DataParameter, "RFC 3261");
      defineParam(id, "id", DataParameter, "RFC 3265");
      defineParam(lr, "lr", ExistsParameter, "RFC 3261");
      defineParam(maddr, "maddr", DataParameter, "RFC 3261");
      defineParam(max_interval, "max-interval", UInt32Parameter, "draft-ietf-sipcore-event-rate-control");
      defineParam(method, "method", DataParameter, "RFC 3261");
      defineParam(micalg, "micalg", DataParameter, "RFC 1847");
      defineParam(min_interval, "min-interval", UInt32Parameter, "draft-ietf-sipcore-event-rate-control");
      defineParam(mode, "mode", DataParameter, "RFC 2046");
      defineParam(name, "name", DataParameter, "RFC 2046");
      defineParam(nc, "nc", DataParameter, "RFC 2617");
      defineParam(nonce, "nonce", QuotedDataParameter, "RFC 2617");
      defineParam(opaque, "opaque", QuotedDataParameter, "RFC 2617");
      defineParam(permission, "permission", DataParameter, "RFC 2046");
      defineParam(protocol, "protocol", QuotedDataParameter, "RFC 1847");
      defineParam(purpose, "purpose", DataParameter, "RFC 3261");
      defineParam(q, "q", QValueParameter, "RFC 3261");

      defineParam(realm, "realm", QuotedDataParameter, "RFC 2617");
      defineParam(reason, "reason", DataParameter, "RFC 3265");
      defineParam(received, "received", DataParameter, "RFC 3261");
      defineParam(response, "response", QuotedDataParameter, "RFC 3262");
      defineParam(retryAfter, "retry-after", UInt32Parameter, "RFC 3261");
      defineParam(rinstance, "rinstance", DataParameter, "proprietary");
      defineParam(rport, "rport", RportParameter, "RFC 3581");
      defineParam(server, "server", DataParameter, "RFC 2046");
      defineParam(site, "site", DataParameter, "RFC 2046");
      defineParam(size, "size", DataParameter, "RFC 2046");
      defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
      defineParam(stale, "stale", DataParameter, "RFC 2617");
      defineParam(tag, "tag", DataParameter, "RFC 3261");
      defineParam(toTag, "to-tag", DataParameter, "RFC 4235");
      defineParam(transport, "transport", DataParameter, "RFC 3261");
      defineParam(ttl, "ttl", UInt32Parameter, "RFC 3261");
      defineParam(uri, "uri", QuotedDataParameter, "RFC 3261");
      defineParam(user, "user", DataParameter, "RFC 3261, 4967");
      defineParam(extension, "ext", DataParameter, "RFC 3966");
      defineParam(username, "username", DataParameter, "RFC 3261");
      defineParam(earlyOnly, "early-only", ExistsParameter, "RFC 3891");
      defineParam(refresher, "refresher", DataParameter, "RFC 4028");

      defineParam(profileType, "profile-type", DataParameter, "draft-ietf-sipping-config-framework");
      defineParam(vendor, "vendor", QuotedDataParameter, "draft-ietf-sipping-config-framework");
      defineParam(model, "model", QuotedDataParameter, "draft-ietf-sipping-config-framework");
      defineParam(version, "version", QuotedDataParameter, "draft-ietf-sipping-config-framework");
      defineParam(effectiveBy, "effective-by", UInt32Parameter, "draft-ietf-sipping-config-framework");
      defineParam(document, "document", DataParameter, "draft-ietf-sipping-config-framework");
      defineParam(appId, "app-id", DataParameter, "draft-ietf-sipping-config-framework");
      defineParam(networkUser, "network-user", DataParameter, "draft-ietf-sipping-config-framework");

      defineParam(url, "url", QuotedDataParameter, "draft-ietf-sip-content-indirect-mech-05");

      defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "draft-ietf-rohc-sigcomp-sip");
      defineParam(addTransport, "addTransport", ExistsParameter, "internal, proprietary");

      /**
         @internal
         @brief Causes this ParserCategory to parse parameters out of pb.
         @param pb The ParseBuffer to parse params from.
      */
      void parseParameters(ParseBuffer& pb);

      /**
         @brief Encodes parameters as they should appear on the wire.
         @param str The ostream to encode to.
         @return str
      */
      std::ostream& encodeParameters(std::ostream& str) const;
      
      // used to compare 2 parameter lists for equality in an order independent way
      /**
         @internal
         @brief An order-sensitive hash over the set of parameters in this 
            ParserCategory.
         @return The hash value as a Data.
      */
      Data commutativeParameterHash() const;
      
      /**
         @internal
         @brief Typeless parameter get interface.
      */
      Parameter* getParameterByEnum(ParameterTypes::Type type) const;

      /**
         @internal
         @brief Removes a parameter.
         @param type The parameter type.
      */
      void removeParameterByEnum(ParameterTypes::Type type);

      /**
         @internal
         @brief Typeless parameter put interface.
      */
      void setParameter(const Parameter* parameter);

      /**
         @brief Returns the number of known (natively supported) parameters.
      */
      int numKnownParams() const {return (int)mParameters.size();};

      /**
         @brief Returns the number of unknown parameters.
      */
      int numUnknownParams() const {return (int)mUnknownParameters.size();};

   protected:
      ParserCategory(PoolBase* pool=0);

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);
      inline PoolBase* getPool()
      {
         return mParameters.get_allocator().mPool;
      }

      inline void freeParameter(Parameter* p)
      {
         if(p)
         {
            p->~Parameter();
            mParameters.get_allocator().deallocate_raw(p);
         }
      }

      virtual const Data& errorContext() const;

      typedef std::vector<Parameter*, PoolAllocator<Parameter*, PoolBase> > ParameterList; 
      ParameterList mParameters;
      ParameterList mUnknownParameters;
      Headers::Type mHeaderType;
   private:
      void clear();
      void copyParametersFrom(const ParserCategory& other);
      friend std::ostream& operator<<(std::ostream&, const ParserCategory&);
      friend class NameAddr;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

}

#undef defineParam

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
