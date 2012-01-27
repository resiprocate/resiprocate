/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_NONCEHELPER_HXX)
#define RESIP_NONCEHELPER_HXX

#include "resip/stack/SipMessage.hxx"
#include "rutil/Data.hxx"

namespace resip
{
/**
@brief NonceHelper is an interface that the developer can extend
to implement custom nonce creation and parsing. It is used in
conjunction with the resip::Helper class.
@sa resip::Helper::setNonceHelper()
@sa resip::Helper::getNonceHelper()
@sa resip::Helper::makeNonce()
**/
class NonceHelper 
{
   public:
      /**
        @brief Nonce stores the creation time of a nonce which
        is set upon construction.
      **/
      class Nonce 
      {
         private:
            UInt64 creationTime;

         public:
            /**
            @brief constructor that sets the creation time
            @param creationTime sets the creation time
            **/
            Nonce(UInt64 creationTime);
            /**
            @brief destructor
            **/
            virtual ~Nonce();
            /**
            @brief gets the creation time
            @return the creation time
            **/
            UInt64 getCreationTime() { return creationTime; };
      };

      virtual ~NonceHelper()=0;
      
      /**
      @brief generate a nonce string from a message and a timestamp
      @param request the message to generate the nonce from
      @param timestamp the timestamp
      @return a nonce string
      **/
      virtual Data makeNonce(const SipMessage& request, const Data& timestamp) = 0;

      /**
        @brief Read a nonce string into a Nonce instance, so that we can inspect 
        the un-encrypted time stamp.
        @param nonce the nonce string to parse
      **/
      virtual NonceHelper::Nonce parseNonce(const Data& nonce) = 0;
};

}

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

