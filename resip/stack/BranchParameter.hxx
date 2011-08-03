#if !defined(RESIP_BRANCHPARAMETER_HXX)
#define RESIP_BRANCHPARAMETER_HXX 

#include <iosfwd>

#include "resip/stack/Parameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "rutil/Data.hxx"
#include "rutil/PoolBase.hxx"

namespace resip
{

class ParseBuffer;

// BranchParameter of the form: 
// rfc3261cookie-sip2cookie-tid-transportseq-clientdata-sip2cookie
// Notably, the tid MAY contain dashes by the clientdata MUST NOT.
//

/**
   @ingroup sip_grammar
   @brief Represents the "via-branch" parameter of the RFC 3261 grammar.
*/

class BranchParameter : public Parameter
{
   public:
      typedef BranchParameter Type;
      
      BranchParameter(ParameterTypes::Type, ParseBuffer& pb, const std::bitset<256>& terminators);
      explicit BranchParameter(ParameterTypes::Type);

      ~BranchParameter();

      // contains z9hG4bK
      bool hasMagicCookie() const;

      // returns tid
      const Data& getTransactionId() const;

      // increments the transport sequence component - not part of tid
      void incrementTransportSequence();

      // pseudo-random tid if none specified, zero sequences either way
      void reset(const Data& transactionId = Data::Empty);

      // access the client specific portion of the branch - not part of tid
      Data& clientData();
      const Data& clientData() const;

      // access sigcomp id -- we do pre- and post-processing on this,
      // so we need "normal" setters and getters
      void setSigcompCompartment(const Data &);
      Data getSigcompCompartment() const;

      static Parameter* decode(ParameterTypes::Type type, 
                                 ParseBuffer& pb, 
                                 const std::bitset<256>& terminators,
                                 PoolBase* pool)
      {
         return new (pool) BranchParameter(type, pb, terminators);
      }
      
      virtual Parameter* clone() const;
      virtual EncodeStream& encode(EncodeStream& stream) const;

      BranchParameter(const BranchParameter& other);
      BranchParameter& operator=(const BranchParameter& other);
      bool operator==(const BranchParameter& other);

      Type& value() {return *this;}

   private:
      bool mHasMagicCookie;
      bool mIsMyBranch;
      Data mTransactionId;
      unsigned int mTransportSeq;
      Data mClientData;
      //magic cookie for interop; if case is different some proxies will treat this as a different tid
      const Data* mInteropMagicCookie; 

      // If we're compressing, this will hold the compartment ID
      // for the host that the request was sent to.
      Data mSigcompCompartment;
                                
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
