#if !defined(RESIP_TRANSACTIONMAP_HXX)
#define RESIP_TRANSACTIONMAP_HXX

#include <unordered_map>
#include "rutil/Data.hxx"

namespace resip
{
  class TransactionState;

/**
   @internal
*/
class TransactionMap 
{
  public:
     ~TransactionMap();
     
     TransactionState* find( const Data& transactionId ) const;
     void add( const Data& transactionId, TransactionState* state  );
     void erase( const Data& transactionId );
     int size() const;
     
  private:

     // We treat branch parameters as case insensitive (RFC3261):
     // 7.3.1 Header Field Format
     // ....
     //    When comparing header fields, field names are always case-
     //    insensitive.Unless otherwise stated in the definition of a
     //    particular header field, field values, parameter names, and parameter
     //    values are case-insensitive.Tokens are always case-insensitive.
     //    Unless specified otherwise, values expressed as quoted strings are
     //    case-sensitive.

      /**
         @internal
      */
      class BranchHasher
      {
         public:
            inline size_t operator()(const Data& branch) const
            {
               return branch.caseInsensitiveTokenHash();
            }
      };

      /**
         @internal
      */
      class BranchEqual
      {
         public:
            inline bool operator()(const Data& branch1, const Data& branch2) const
            {
               return isEqualNoCase(branch1,branch2);
            }
      };

     std::unordered_map<Data, TransactionState*, BranchHasher, BranchEqual> mMap;
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
