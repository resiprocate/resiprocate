#if !defined(REPRO_TLSPEERIDENTITYSTORE_HXX)
#define REPRO_TLSPEERIDENTITYSTORE_HXX

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "resip/stack/Message.hxx"

#include "repro/AbstractDb.hxx"

namespace resip
{
  class TransactionUser;
}

namespace repro
{

typedef resip::Fifo<resip::Message> MessageFifo;

class TlsPeerIdentityStore
{
   public:
      typedef resip::Data Key;
      
      TlsPeerIdentityStore(AbstractDb& db);
      
      virtual ~TlsPeerIdentityStore();
      
      AbstractDb::TlsPeerIdentityRecord getTlsPeerIdentityInfo( const Key& key ) const;

      bool isAuthorized( const std::set<resip::Data>& peerNames,
                                   const std::set<resip::Data>& identities ) const;
      
      bool addTlsPeerIdentity( const resip::Data& peerName, 
                    const resip::Data& authorizedIdentity);

      void eraseTlsPeerIdentity( const Key& key );
      
      bool updateTlsPeerIdentity( const Key& originalKey,
                       const resip::Data& peerName,
                       const resip::Data& authorizedIdentity);
      
      Key getFirstKey();// return empty if no more
      Key getNextKey(); // return empty if no more 
      
      static Key buildKey(const resip::Data& peerName, const resip::Data& authorizedIdentity);
      static void getTlsPeerIdentityFromKey(const AbstractDb::Key& key, resip::Data& peerName, resip::Data& authorizedIdentity);

   private:

      AbstractDb& mDb;
      static const resip::Data SEPARATOR;
};

}
#endif  

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

