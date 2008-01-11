#if !defined(RESIP_NAME_ADDR_HXX)
#define RESIP_NAME_ADDR_HXX

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/ParserContainer.hxx"
#include "resip/stack/Uri.hxx"

namespace resip
{

//====================
// NameAddr:
//====================
class NameAddr : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputMulti};

      NameAddr();
      NameAddr(HeaderFieldValue* hfv, Headers::Type type);
      explicit NameAddr(const Uri&);
      explicit NameAddr(const Data& unparsed);

      NameAddr(const NameAddr&);
      NameAddr& operator=(const NameAddr&);
      bool operator==(const NameAddr& other) const;

      virtual ~NameAddr();
      
      Uri& uri();
      const Uri& uri() const;
      Data& displayName();
      const Data& displayName() const;
      bool isAllContacts() const;
      void setAllContacts();
      
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      bool operator<(const NameAddr& other) const;

      bool mustQuoteDisplayName() const;      
   protected:
      bool mAllContacts;
      mutable Uri mUri;
      mutable Data mDisplayName;

   private:
#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) && (__GNUC_MINOR__ <= 3) )
      //disallow the following parameters from being accessed in NameAddr
      //this works on gcc 3.2 so far. definitely does not work on gcc 2.95 on
      //qnx
      // as well as on gcc 3.4.0 and 3.4.1
      using ParserCategory::param;
      transport_Param::DType& param(const transport_Param& paramType) const;
      method_Param::DType& param(const method_Param& paramType) const;
      ttl_Param::DType& param(const ttl_Param& paramType) const;
      maddr_Param::DType& param(const maddr_Param& paramType) const;
      lr_Param::DType& param(const lr_Param& paramType) const;
      comp_Param::DType& param(const comp_Param& paramType) const;
#endif
};
typedef ParserContainer<NameAddr> NameAddrs;
 
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
