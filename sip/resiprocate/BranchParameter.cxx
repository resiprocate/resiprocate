#include <cassert>
#include <sipstack/BranchParameter.hxx>
#include <sipstack/Symbols.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/ParseException.hxx>

using namespace Vocal2;
using namespace std;

static unsigned long
getNextTransactionCount()
{
   static volatile unsigned long TransactionCount=random()*2;
   return TransactionCount+=2; // !jf! needs to be atomic
}


BranchParameter::BranchParameter(ParameterTypes::Type type,
                                 ParseBuffer& pb)
   : Parameter(type), 
     mHasMagicCookie(false),
     mTransactionId(),
     mCounter(1111111),
     mClientData()
{
   if (strncasecmp(pb.position(), Symbols::MagicCookie, 7) == 0)
   {
      mHasMagicCookie = true;
      pb.skipN(7);
   }
   if (mHasMagicCookie)
   {
      const char* anchor = pb.position();
      pb.skipToChar(Symbols::DASH[0]);
      pb.data(mTransactionId, anchor);

      if (!pb.eof())
      {
         pb.skipChar();
         if (isdigit(*pb.position()))
         {
            mCounter = pb.integer();
            
            if (*pb.position() == Symbols::DASH[0])
            {
               anchor = pb.skipChar();
               pb.skipToEnd();
               pb.data(mClientData, anchor);
            }
         }
      }
   }
   else
   {
      const char* anchor = pb.position();
      pb.skipToEnd();
      pb.data(mTransactionId, anchor);
   }
}

BranchParameter::BranchParameter(ParameterTypes::Type type)
   : Parameter(type),
     mHasMagicCookie(true),
     mTransactionId(getNextTransactionCount()),
     mCounter(1111111),
     mClientData()
{
}

BranchParameter& 
BranchParameter::operator=(const BranchParameter& other)
{
   if (this != &other)
   {
      mHasMagicCookie = other.mHasMagicCookie;
      mTransactionId = other.mTransactionId;
      mCounter = other.mCounter;
      mClientData = other.mClientData;
   }
   return *this;
}


bool
BranchParameter::hasMagicCookie()
{
   return mHasMagicCookie;
}

Data& 
BranchParameter::transactionId()
{
   return mTransactionId;
}

unsigned long& 
BranchParameter::counter()
{
   return mCounter;
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
   if (mHasMagicCookie)
   {
      stream << Symbols::MagicCookie;
   }
   stream << mTransactionId;
   if (mCounter != 1111111)
   {
      stream << Symbols::DASH[0];
      stream << mCounter;

      if (! mClientData.empty())
      {
         stream << Symbols::DASH[0];
         stream << mClientData;
      }
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
