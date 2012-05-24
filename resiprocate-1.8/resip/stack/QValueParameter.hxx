#if !defined(RESIP_QVALUEPARAMETER_HXX)
#define RESIP_QVALUEPARAMETER_HXX 

#include "rutil/resipfaststreams.hxx"

#include "resip/stack/Parameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "resip/stack/QValue.hxx"
#include "rutil/PoolBase.hxx"
#include <iosfwd>

namespace resip
{

   class ParseBuffer;

/**
   @ingroup sip_grammar

   @brief   Represents the "q" parameter value of the RFC 3261 grammar.
   
   @see     Parameter
   
   First reference to q-values in 3261 reproduced here:

   8.1.3.4 Processing 3xx Responses

   Upon receipt of a redirection response (for example, a 301 response
   status code), clients SHOULD use the URI(s) in the Contact header
   field to formulate one or more new requests based on the redirected
   request.  This process is similar to that of a proxy recursing on a
   3xx class response as detailed in Sections 16.5 and 16.6.  A client
   starts with an initial target set containing exactly one URI, the
   Request-URI of the original request.  If a client wishes to formulate
   new requests based on a 3xx class response to that request, it places
   the URIs to try into the target set.  Subject to the restrictions in
   this specification, a client can choose which Contact URIs it places
   into the target set.  As with proxy recursion, a client processing
   3xx class responses MUST NOT add any given URI to the target set more
   than once.  If the original request had a SIPS URI in the Request-
   URI, the client MAY choose to recurse to a non-SIPS URI, but SHOULD
   inform the user of the redirection to an insecure URI.

      Any new request may receive 3xx responses themselves containing
      the original URI as a contact.  Two locations can be configured to
      redirect to each other.  Placing any given URI in the target set
      only once prevents infinite redirection loops.

   As the target set grows, the client MAY generate new requests to the
   URIs in any order.  A common mechanism is to order the set by the "q"
   parameter value from the Contact header field value.  Requests to the
   URIs MAY be generated serially or in parallel.  One approach is to
   process groups of decreasing q-values serially and process the URIs
   in each q-value group in parallel.  Another is to perform only serial
   processing in decreasing q-value order, arbitrarily choosing between
   contacts of equal q-value.
*/
   class QValueParameter : public Parameter
   {
   public:
      typedef QValue Type;

      /**
        @brief constructor
        @param type used to initialize Parameter
        @param pb input is expected to be in the format " = qvalue"
               The blanks before and after the "=" sign are optional in this 
               implementation. But absence of "=" will throw a ParseException.
        @param terminators not used
        @see   Parameter
        @throw ParseException
        */
      QValueParameter(ParameterTypes::Type, ParseBuffer& pb, const std::bitset<256>& terminators);
      /**
        @brief constructor creates object and sets q value to zero
        @param type used to initialize Parameter
        @throw nothing
        */
      explicit QValueParameter(ParameterTypes::Type type);

      /**
        @brief creates a QValueParameter object and returns a pointer to it.
        @param type used to initialize Parameter
        @param pb input is expected to be in the format " = qvalue"
               The blanks before and after the "=" sign are optional in this 
               implementation. But absence of "=" will throw a ParseException.
        @param terminators not used
        @return a new QValueParameter object that is a copy of this one.
        @throw ParseException
        */
      static Parameter* decode(ParameterTypes::Type type, 
                                 ParseBuffer& pb, 
                                 const std::bitset<256>& terminators,
                                 PoolBase* pool)
      {
         return new (pool) QValueParameter(type, pb, terminators);
      }

      /**
        @brief creates a new QValueParameter object that is a copy of this one.
        @return a new QValueParameter object that is a copy of this one.
        */
      virtual Parameter* clone() const;

      /**
        @brief returns "q=3" or equivalent in the stream it receives
        @param stream ostream to write into
        @return ostream with information written into it
        */
      virtual EncodeStream& encode(EncodeStream& stream) const;

      Type& value() {return mValue;}
      int qval() const {return mValue.getValue();}

   private:
      Type mValue;
   };

   /**
     @brief calls encode and returns "q=3" or equivalent ostream
     @param stream ostream to write into
     @param qvalue qvalue
     @return ostream with q value divided by 1000 and expressed in decimal to 
      3 decimal places
     @see QValue
     */
   EncodeStream& operator<<(EncodeStream& stream, const QValue& qvalue);

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
