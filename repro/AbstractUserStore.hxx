#if !defined(REPRO_ABSTRACTUSERSTORE_HXX)
#define REPRO_ABSTRACTUSERSTORE_HXX

#include "rutil/Data.hxx"
#include "repro/AbstractDb.hxx"
#include "rutil/Fifo.hxx"
#include "resip/stack/Message.hxx"



namespace resip
{
  class TransactionUser;
}

namespace repro
{

typedef resip::Fifo<resip::Message> MessageFifo;

class AbstractUserStore
{
   public:
      typedef resip::Data Key;
      
      virtual ~AbstractUserStore(){};
      
      virtual void requestUserAuthInfo( const resip::Data& user, 
                                const resip::Data& realm,
                                const resip::Data& transactionToken,
                                resip::TransactionUser& transactionUser ) const = 0;

      virtual AbstractDb::UserRecord getUserInfo( const Key& key ) const =0;

      virtual resip::Data getUserAuthInfo( const resip::Data& user,
                                   const resip::Data& realm ) const =0;
      
      virtual void addUser( const resip::Data& user, 
                    const resip::Data& domain, 
                    const resip::Data& realm, 
                    const resip::Data& password, 
                    bool  applyA1HashToPassword,
                    const resip::Data& fullName,
                    const resip::Data& emailAddress  ) = 0;
      
      virtual void eraseUser( const Key& key ) = 0;
      
      virtual void updateUser( const Key& originalKey,
                       const resip::Data& user, 
                       const resip::Data& domain, 
                       const resip::Data& realm, 
                       const resip::Data& password, 
                       bool  applyA1HashToPassword,
                       const resip::Data& fullName,
                       const resip::Data& emailAddress ) = 0;
      
	   virtual AbstractDb::UserRecordList getAllUsers() = 0;
      virtual Key getKey( const AbstractDb::UserRecord &ur ) = 0;

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
