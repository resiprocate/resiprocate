#include <cassert>
#include "resiprocate/BranchParameter.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/ParseException.hxx"

#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

static unsigned long
getNextTransactionCount()
{
   assert( sizeof(long) >= 4 );
#if 0 
   //static volatile unsigned long TransactionCount=random()*2;
   static volatile unsigned long transactionCount=0;
   if ( transactionCount == 0 )
   { 
      transactionCount = 0x3FFFffff & Random::getRandom();
      transactionCount -= (transactionCount%10000);
   }
   return ++transactionCount; // !jf! needs to be atomic
#else
	return ( 0x3FFFffff & Random::getRandom() );
#endif
}

BranchParameter::BranchParameter(ParameterTypes::Type type,
                                 ParseBuffer& pb, const char* terminators)
   : Parameter(type), 
     mHasMagicCookie(false),
     mIsMyBranch(false),
     mTransactionId(getNextTransactionCount()),
     mTransportSeq(1)
{
   pb.skipChar(Symbols::EQUALS[0]);
   if (strncasecmp(pb.position(), Symbols::MagicCookie, 7) == 0)
   {
      mHasMagicCookie = true;
      pb.skipN(7);
   }

   const char* anchor = pb.position();
   const char* end = pb.skipToOneOf(ParseBuffer::Whitespace, ";=?>"); // !dlb! add to ParseBuffer as terminator set

   if (mHasMagicCookie &&
       (end - anchor > 2*8) &&
       // look for prefix cookie
       (strncasecmp(anchor, Symbols::resipCookie, 8) == 0) &&
       // look for postfix cookie
       (strncasecmp(end - 8, Symbols::resipCookie, 8) == 0))
   {
      mIsMyBranch = true;
      anchor += 8;
      // rfc3261cookie-sip2cookie-tid-transportseq-sip2cookie
      //                          ^

      pb.skipBackN(8);
      pb.skipBackToChar(Symbols::DASH[0]);
      pb.skipBackChar(Symbols::DASH[0]);
      // rfc3261cookie-sip2cookie-tid-transportseq-sip2cookie
      //                          ^  $
      pb.data(mTransactionId, anchor);
         
      pb.skipChar();
      mTransportSeq = pb.integer();
      pb.reset(end);
   }
   else
   {
      pb.data(mTransactionId, anchor);
   }
}

BranchParameter::BranchParameter(ParameterTypes::Type type)
   : Parameter(type),
     mHasMagicCookie(true),
     mIsMyBranch(true),
     mTransactionId(getNextTransactionCount()),
     mTransportSeq(1)
{
}

BranchParameter::BranchParameter(const BranchParameter& other)
   : Parameter(other), 
     mHasMagicCookie(other.mHasMagicCookie),
     mIsMyBranch(other.mIsMyBranch),
     mTransactionId(other.mTransactionId),
     mTransportSeq(other.mTransportSeq)
{
}

BranchParameter& 
BranchParameter::operator=(const BranchParameter& other)
{
   if (this != &other)
   {
      mHasMagicCookie = other.mHasMagicCookie;
      mIsMyBranch = other.mIsMyBranch;
      mTransactionId = other.mTransactionId;
      mTransportSeq = other.mTransportSeq;
   }
   return *this;
}

bool
BranchParameter::hasMagicCookie()
{
   return mHasMagicCookie;
}

const Data& 
BranchParameter::getTransactionId()
{
   return mTransactionId;
}

void
BranchParameter::incrementTransportSequence()
{
   assert(mIsMyBranch);
   mTransportSeq++;
}

void
BranchParameter::reset(const Data& transactionId)
{
   mHasMagicCookie = true;
   mIsMyBranch = true;

   mTransportSeq = 1;
   if (!transactionId.empty())
   {
      mTransactionId = transactionId;
   }
   else
   {
      mTransactionId = Data(getNextTransactionCount());
   }
}

Parameter* 
BranchParameter::clone() const
{
   return new BranchParameter(*this);
}

ostream& 
BranchParameter::encode(ostream& stream) const
{
   stream << getName() << Symbols::EQUALS;
   if (mHasMagicCookie)
   {
      stream << Symbols::MagicCookie;
   }
   if (mIsMyBranch)
   {
      stream << Symbols::resipCookie 
             << mTransactionId 
             << Symbols::DASH[0]
             << mTransportSeq
             << Symbols::resipCookie;
   }
   else
   {
      stream << mTransactionId;
   }
      
   return stream;
}


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
