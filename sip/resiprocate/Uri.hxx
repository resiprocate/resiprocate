#ifndef Uri_hxx
#define Uri_hxx

#include <cassert>

#include "resiprocate/ParserCategory.hxx"

namespace resip
{
class SipMessage;

class Uri : public ParserCategory
{
   public:
      Uri();
      Uri(const Uri&);
      explicit Uri(const Data& data);

      ~Uri();
      
      static Uri fromTel(const Uri&, const Data& host);

      Data& host() const {checkParsed(); return mHost;}
      Data& user() const {checkParsed(); return mUser;}
      Data& userParameters() const {checkParsed(); return mUserParameters;}
      Data& opaque() const {checkParsed(); return mHost;}

      const Data& getAor() const;
      const Data getAorNoPort() const;
      Data& scheme() const {checkParsed(); return mScheme;}
      int& port() const {checkParsed(); return mPort;}
      Data& password() const {checkParsed(); return mPassword;}

      bool hasEmbedded() const;
      SipMessage& embedded() const;
      
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      
      // parse the headers into this as SipMessage
      void parseEmbeddedHeaders(ParseBuffer& pb);
      std::ostream& encodeEmbeddedHeaders(std::ostream& str) const;

      Uri& operator=(const Uri& rhs);
      bool operator==(const Uri& other) const;
      bool operator!=(const Uri& other) const;
      bool operator<(const Uri& other) const;
      
   protected:
      mutable Data mScheme;
      mutable Data mHost;
      mutable Data mUser;
      mutable Data mUserParameters;
      mutable int mPort;
      mutable Data mAor;
      mutable Data mPassword;

      mutable Data mOldScheme;
      mutable Data mOldHost;
      mutable Data mOldUser;
      mutable int mOldPort;

   private:
      Data mEmbeddedHeadersText;
      SipMessage* mEmbeddedHeaders;
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
