#if !defined(RESIP_URI_HXX)
#define RESIP_URI_HXX 

#include <bitset>
#include "rutil/ResipAssert.h"

#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/Token.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{
class SipMessage;

/**
   @ingroup sip_grammar
   @brief Represents the "SIP-URI" and "SIPS-URI" elements in the RFC 3261 
      grammar. Also can be made to represent other URI types (like tel URIs)
*/
class Uri : public ParserCategory
{
   public:
      RESIP_HeapCount(Uri);
      
      static const size_t uriEncodingTableSize = 256;

      Uri(PoolBase* pool=0);
      Uri(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool=0);
      Uri(const Uri& orig,
         PoolBase* pool=0);
      explicit Uri(const Data& data);

      ~Uri();
      
      // convert from a tel scheme to sip scheme, adds user=phone param
      //static Uri fromTel(const Uri&, const Data& host);  // deprecate...
      static Uri fromTel(const Uri&, const Uri& hostUri);

      Data& host() {checkParsed(); mHostCanonicalized=false; return mHost;}
      const Data& host() const {checkParsed(); return mHost;}
      Data& user() {checkParsed(); return mUser;}
      const Data& user() const {checkParsed(); return mUser;}
      Data& userParameters() {checkParsed(); return mUserParameters;}
      const Data& userParameters() const {checkParsed(); return mUserParameters;}
      Data& opaque() {checkParsed(); return mHost;}
      const Data& opaque() const {checkParsed(); return mHost;}
      Data& path() {checkParsed(); return mPath;}
      const Data& path() const {checkParsed(); return mPath;}

      // Returns user@host[:port] (no scheme)
      Data getAor() const;
      // Returns user@host (no scheme or port)
      Data getAorNoPort() const;

      // Actually returns the AOR; <scheme>:<user>@<host>
      Data getAorNoReally() const
      {
         return getAOR(false);
      }

      // Returns the AOR, optionally adding the port
      Data getAOR(bool addPort) const;

      //strips all paramters - if transport type is specified (ie. not UNKNOWN_TRANSPORT),
      //and the default port for the transport is on the Aor, then it is removed
      Uri getAorAsUri(TransportType transportTypeToRemoveDefaultPort = UNKNOWN_TRANSPORT) const;
      

      /**
         Returns true if the user appears to fit the BNF for the 
         'telephone-subscriber' element in the RFC 3261 (and by extension, RFC 
         3966) grammar. This is important because 'telephone-subscriber' can 
         have parameters, which you could then access easily through the
         getUserAsTelephoneSubscriber() and setUserAsTelephoneSubscriber() 
         calls.
      */
      bool userIsTelephoneSubscriber() const;

      /**
         Returns the user-part as a 'telephone-subscriber' grammar element (in 
         other words, this parses the user-part into a dial string and 
         parameters, with the dial-string accessible with Token::value(), and 
         the parameters accessible with the various Token::param() and 
         Token::exists() interfaces). 
         
         For example, suppose the following is in the Request-URI:
         
         sip:5555551234;phone-context=+86\@example.com;user=dialstring
         
         The user-part of this SIP URI is "5555551234;phone-context=+86", and it
         fits the BNF for the 'telephone-subscriber' grammar element. To access 
         the 'phone-context' parameter, do something like the following:

         @code
            Uri& reqUri(sip.header(h_RequestLine).uri());

            // !bwc! May add native support for this param later
            static ExtensionParameter p_phoneContext("phone-context");
            Data phoneContextValue;

            if(reqUri.isWellFormed())
            {
               if(reqUri.exists(p_phoneContext))
               {
                  // Phone context as URI param
                  phoneContextValue=reqUri.param(p_phoneContext);
               }
               else if(reqUri.scheme()=="sip" || reqUri.scheme()=="sips")
               {
                  // Might have phone-context as a user param (only happens 
                  // in a sip or sips URI)
                  // Technically, this userIsTelephoneSubscriber() check is 
                  // required: 
                  // sip:bob;phone-context=+86@example.com doesn't have a 
                  // phone-context param according to the BNF in 3261. But, 
                  // interop may require you to parse this as if it did have 
                  // such a param.
                  if(reqUri.userIsTelephoneSubscriber())
                  {
                     Token telSub(reqUri.getUserAsTelephoneSubscriber());
                     if(telSub.isWellFormed() && telSub.exists(p_phoneContext))
                     {
                        // Phone context as user param
                        phoneContextValue=telSub.param(p_phoneContext);
                     }
                  }
               }
            }
         @endcode
      */
      Token getUserAsTelephoneSubscriber() const;

      /**
         Sets the user-part of this URI using the dial-string and parameters 
         stored in telephoneSubscriber.
         @param telephoneSubscriber The user-part, as a 'telephone-subscriber'
            grammar element.
      */
      void setUserAsTelephoneSubscriber(const Token& telephoneSubscriber);


      Data& scheme() {checkParsed(); return mScheme;}
      const Data& scheme() const {checkParsed(); return mScheme;}
      int& port() {checkParsed(); return mPort;}
      int port() const {checkParsed(); return mPort;}
      Data& password() {checkParsed(); return mPassword;}
      const Data& password() const {checkParsed(); return mPassword;}

      Data& netNs() {return(mNetNs);}
      const Data& netNs() const {return(mNetNs);}

      Data toString() const;

      /** Returns true if the uri can be converted into a string that can be
          used as an enum lookup */
      bool isEnumSearchable() const;

      /** Return a vector of domains to do a NAPTR lookup for enum */
      std::vector<Data> getEnumLookups(const std::vector<Data>& suffixes) const;

      /** Modifies the default URI encoding character sets */
      static void setUriUserEncoding(unsigned char c, bool encode);
      static void setUriPasswordEncoding(unsigned char c, bool encode);
      
      bool hasEmbedded() const;
      SipMessage& embedded();
      const SipMessage& embedded() const;

      void removeEmbedded();

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      
      // parse the headers into this as SipMessage
      void parseEmbeddedHeaders(ParseBuffer& pb);
      EncodeStream& encodeEmbeddedHeaders(EncodeStream& str) const;

      Uri& operator=(const Uri& rhs);
      bool operator==(const Uri& other) const;
      bool operator!=(const Uri& other) const;
      bool operator<(const Uri& other) const;
      
      bool aorEqual(const Uri& rhs) const;

      typedef std::bitset<Uri::uriEncodingTableSize> EncodingTable;

      static EncodingTable& getUserEncodingTable()
      {
         static EncodingTable userEncodingTable(
               Data::toBitset("abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "0123456789"
                              "-_.!~*\\()&=+$,;?/").flip());
         return userEncodingTable;
      }

      static EncodingTable& getPasswordEncodingTable()
      {
         static EncodingTable passwordEncodingTable(
               Data::toBitset("abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "0123456789"
                              "-_.!~*\\()&=+$").flip());
         return passwordEncodingTable;
      }

      static EncodingTable& getLocalNumberTable()
      {
         // ?bwc? 'p' and 'w' are allowed in 2806, but have been removed in 
         // 3966. Should we support these or not?
         static EncodingTable localNumberTable(
               Data::toBitset("*#-.()0123456789ABCDEFpw"));
         return localNumberTable;
      }

      static EncodingTable& getGlobalNumberTable()
      {
         static EncodingTable globalNumberTable(
               Data::toBitset("-.()0123456789"));
         return globalNumberTable;
      }

      // Inform the compiler that overloads of these may be found in
      // ParserCategory, too.
      using ParserCategory::exists;
      using ParserCategory::remove;
      using ParserCategory::param;

      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool);
      bool exists(const Param<Uri>& paramType) const;
      void remove(const Param<Uri>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      friend class _enum##_Param

      defineParam(ob,"ob",ExistsParameter,"RFC 5626");
      defineParam(gr, "gr", ExistsOrDataParameter, "RFC 5627");
      defineParam(comp, "comp", DataParameter, "RFC 3486");
      defineParam(duration, "duration", UInt32Parameter, "RFC 4240");
      defineParam(lr, "lr", ExistsParameter, "RFC 3261");
      defineParam(maddr, "maddr", DataParameter, "RFC 3261");
      defineParam(method, "method", DataParameter, "RFC 3261");
      defineParam(transport, "transport", DataParameter, "RFC 3261");
      defineParam(ttl, "ttl", UInt32Parameter, "RFC 3261");
      defineParam(user, "user", DataParameter, "RFC 3261, 4967");
      defineParam(extension, "ext", DataParameter, "RFC 3966"); // Token is used when ext is a user-parameter
      defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "RFC 5049");
      defineParam(rinstance, "rinstance", DataParameter, "proprietary (resip)");
      defineParam(addTransport, "addTransport", ExistsParameter, "RESIP INTERNAL");
      defineParam(wsSrcIp, "ws-src-ip", DataParameter, "RESIP INTERNAL (WebSocket)");
      defineParam(wsSrcPort, "ws-src-port", UInt32Parameter, "RESIP INTERNAL (WebSocket)");

#undef defineParam

   protected:
      Data mScheme;
      Data mHost;
      Data mUser;
      Data mUserParameters;
      int mPort;
      Data mPassword;
      Data mNetNs;  ///< Net namespace name scoping host and port
      Data mPath;

      void getAorInternal(bool dropScheme, bool addPort, Data& aor) const;
      mutable bool mHostCanonicalized;
      mutable Data mCanonicalHost;  ///< cache for IPv6 host comparison

   private:
      std::auto_ptr<Data> mEmbeddedHeadersText;
      std::auto_ptr<SipMessage> mEmbeddedHeaders;

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];

      /** 
         Dummy static initialization variable, for ensuring that the encoding 
         tables are initialized sometime during static initialization, 
         preventing the scenario where multiple threads try to runtime init the 
         same table at the same time.
         @note Prior to static initialization of this bool, it could be either 
            true or false; you should not be using this variable to check 
            whether the tables are initialized. Just call the getXTable() 
            accessor function; it will init the table if it is not already.
      */
      static const bool tablesMightBeInitialized;

};

}

#include "rutil/HashMap.hxx"

HashValue(resip::Uri);

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
