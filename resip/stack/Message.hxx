#ifndef RESIP_Message_hxx
#define RESIP_Message_hxx 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Data.hxx"
#include <iosfwd>
#include "rutil/resipfaststreams.hxx"

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

      /// return a facet for brief encoding of message
      Brief brief() const;
      virtual Message* clone() const = 0;
      /// output the entire message to stream
      virtual EncodeStream& encode(EncodeStream& strm) const = 0;
      /// output a brief description to stream
      virtual EncodeStream& encodeBrief(EncodeStream& str) const = 0; 

   protected:
      friend class TuSelector;
      friend class TransactionController;
      friend class TransactionState;
      friend class SipStack;
      bool hasTransactionUser() const { return mTu != 0; }
      void setTransactionUser(TransactionUser* t) { mTu = t; }
      TransactionUser* getTransactionUser() { return mTu; }
      TransactionUser* mTu;      
};

//always need std streams where things are encoded to cout, cerr, MD5Stream, etc...
#ifndef  RESIP_USE_STL_STREAMS
EncodeStream& 
operator<<(EncodeStream& strm, const Message& msg);

EncodeStream& 
operator<<(EncodeStream& strm, const Message::Brief& brief);
#endif

std::ostream& 
operator<<(std::ostream& strm, const Message& msg);

std::ostream& 
operator<<(std::ostream& strm, const Message::Brief& brief);

}

#endif

/* ====================================================================
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

