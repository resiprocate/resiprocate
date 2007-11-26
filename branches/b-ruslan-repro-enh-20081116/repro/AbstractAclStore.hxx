#if !defined(REPRO_ABSTRACTACLSTORE_HXX)
#define REPRO_ABSTRACTACLSTORE_HXX

#include <list>
#include "rutil/Data.hxx"
#include "resip/stack/Tuple.hxx"

namespace repro
{

class AbstractAclStore
{
   public:
      class TlsPeerNameRecord
      {
         public:
            resip::Data key;
            resip::Data mTlsPeerName;
      };

      class AddressRecord
      {
         public:
            AddressRecord(const resip::Data& printableAddress, const int port, const resip::TransportType type) : mAddressTuple(printableAddress, port, type) {};
            resip::Data key;
            resip::Tuple mAddressTuple;
            short mMask;
      };

      typedef resip::Data Key;
      typedef std::vector<TlsPeerNameRecord> TlsPeerNameList;
      typedef std::vector<AddressRecord> AddressList;
      
      virtual ~AbstractAclStore(){};
      
      virtual void addAcl(const resip::Data& tlsPeerName,
                  const resip::Data& address,
                  const short& mask,
                  const short& port,
                  const short& family,
                  const short& transport) = 0;

      virtual bool addAcl(const resip::Data& tlsPeerNameOrAddress,
                  const short& port,
                  const short& transport) = 0;
      
      virtual void eraseAcl(const resip::Data& key) = 0;
      
      virtual resip::Data getTlsPeerName( const resip::Data& key ) = 0;
      virtual resip::Tuple getAddressTuple( const resip::Data& key ) = 0;
      virtual short getAddressMask( const resip::Data& key ) = 0;
      
      virtual Key getFirstTlsPeerNameKey() = 0; // return empty if no more
      virtual Key getNextTlsPeerNameKey(Key& key) = 0; // return empty if no more  
      virtual Key getFirstAddressKey() = 0; // return empty if no more
      virtual Key getNextAddressKey(Key& key) = 0; // return empty if no more 

      virtual bool isTlsPeerNameTrusted(const std::list<resip::Data>& tlsPeerNames) = 0;
      virtual bool isAddressTrusted(const resip::Tuple& address) = 0;
      virtual bool isRequestTrusted(const resip::SipMessage& request) = 0;


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
 */
