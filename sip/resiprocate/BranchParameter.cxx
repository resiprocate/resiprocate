#include <cassert>
#include "sip2/sipstack/BranchParameter.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/util/ParseBuffer.hxx"
#include "sip2/util/Random.hxx"
#include "sip2/sipstack/ParseException.hxx"

#include "sip2/util/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

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
     mClientSeq(0),
     mTransportSeq(1),
     mClientData()
{
   pb.skipChar(Symbols::EQUALS[0]);
   if (strncasecmp(pb.position(), Data(Symbols::MagicCookie).data(), 7) == 0)
   {
      mHasMagicCookie = true;
      pb.skipN(7);
   }

   if (mHasMagicCookie && (strncasecmp(pb.position(), Data(Symbols::Vocal2Cookie).data(), 7) == 0))
   {
      mIsMyBranch = true;
      pb.skipN(8);

      const char* anchor = pb.position();
      pb.skipToChar(Symbols::DOT[0]);
      pb.data(mTransactionId, anchor);

      pb.skipChar();
      mClientSeq = pb.integer();
      pb.skipChar(Symbols::DASH[0]);
         
      mTransportSeq = pb.integer();

      if (!pb.eof() && *pb.position() == Symbols::DASH[0])
      {
         anchor = pb.skipChar();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";=?>"); // !dlb! add to ParseBuffer as terminator set
         pb.data(mClientData, anchor);
      }
   }
   else
   {
      const char* anchor = pb.position();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";=?>"); // !dlb! ibid
      pb.data(mTransactionId, anchor);
   }
   setServerTransactionId();
}

BranchParameter::BranchParameter(ParameterTypes::Type type)
   : Parameter(type),
     mHasMagicCookie(true),
     mIsMyBranch(true),
     mTransactionId(getNextTransactionCount()),
     mClientSeq(0),
     mTransportSeq(1),
     mClientData()
{
   setServerTransactionId();
}

BranchParameter::BranchParameter(const BranchParameter& other)
   : Parameter(other), 
     mHasMagicCookie(other.mHasMagicCookie),
     mIsMyBranch(other.mIsMyBranch),
     mTransactionId(other.mTransactionId),
     mClientSeq(other.mClientSeq),
     mTransportSeq(other.mTransportSeq),
     mClientData(other.mClientData),
     mServerTransactionId(other.mServerTransactionId)
{
}

void
BranchParameter::setServerTransactionId()
{
   mServerTransactionId = mTransactionId;
   mServerTransactionId += Symbols::DOT[0];
   mServerTransactionId += Data(mClientSeq);
}

BranchParameter& 
BranchParameter::operator=(const BranchParameter& other)
{
   if (this != &other)
   {
      mHasMagicCookie = other.mHasMagicCookie;
      mIsMyBranch = other.mIsMyBranch;
      mTransactionId = other.mTransactionId;
      mServerTransactionId = other.mServerTransactionId;
      mClientSeq = other.mClientSeq;
      mTransportSeq = other.mTransportSeq;
      mClientData = other.mClientData;
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
   if (mIsMyBranch)
   {
      return mServerTransactionId;
   }
   else
   {
      return mTransactionId;
   }
}

const Data&
BranchParameter::getServerTransactionId()
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
BranchParameter::setClientSequence(unsigned int long seq)
{
   assert(mIsMyBranch);   
   mClientSeq = seq;
   setServerTransactionId();
}

void
BranchParameter::reset(const Data& serverTransactionId)
{
   mHasMagicCookie = true;
   mIsMyBranch = true;

   mClientSeq = 0;
   mTransportSeq = 1;
   if (!serverTransactionId.empty())
   {
      //assert(serverTransactionId.find(Symbols::DOT[0]) == Data::npos);
      //assert(serverTransactionId.find(Symbols::DASH[0]) == Data::npos);
      mTransactionId = serverTransactionId;
   }
   else
   {
      mTransactionId = Data(getNextTransactionCount());
   }

   setServerTransactionId();
}

Data& 
BranchParameter::clientData()
{
   return mClientData;
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
      stream << Symbols::Vocal2Cookie 
             << mServerTransactionId 
             << Symbols::DASH[0]
             << mTransportSeq;

      if (! mClientData.empty())
      {
         stream << Symbols::DASH[0]
                << mClientData;
      }
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
