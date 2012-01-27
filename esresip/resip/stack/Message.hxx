/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifndef RESIP_Message_hxx
#define RESIP_Message_hxx 

#include "rutil/Data.hxx"
#include <iosfwd>

namespace resip
{
class TransactionUser;

/**
   @ingroup resip_crit
   @brief The base-class used for message-passing.
*/
class Message 
{
   public:
      Message();
      virtual ~Message() {}

      /// facet for brief output to streams
      class Brief
      {
         public:
            Brief(const Message& src);
            const Message& mSource;
      };

      /// @brief return a facet for brief encoding of message, 
      /// primarily for logging
      /// @return facet for brief encoding of message
      Brief brief() const;
      /// @brief create a duplicate of message
      /// @return pointer to newly created message
      virtual Message* clone() const = 0;
      /// output the entire message to stream
      virtual std::ostream& encode(std::ostream& strm) const = 0;
      /// output a brief description to stream
      virtual std::ostream& encodeBrief(std::ostream& str) const = 0;

   protected:
      friend class TuSelector;
      friend class TransactionState;
      friend class SipStack;
      friend class TransactionController;
      /// check if the message is known to belong to a certain TransactionUser
      bool hasTransactionUser() const { return tu != 0; }
      /// route the responses to a message to the given TransactionUser
      void setTransactionUser(TransactionUser* t) { tu = t; }
      /// get the TransactionUser, if known, that will handle this message 
      TransactionUser* getTransactionUser() { return tu; }
      TransactionUser* tu;      
};

std::ostream& 
operator<<(std::ostream& strm, const Message& msg);

std::ostream& 
operator<<(std::ostream& strm, const Message::Brief& brief);

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

