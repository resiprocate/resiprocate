#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <cassert>
#include "resip/stack/BranchParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Random.hxx"
#include "rutil/Coders.hxx"
#include "resip/stack/ParseException.hxx"

#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

BranchParameter::BranchParameter(ParameterTypes::Type type,
                                 ParseBuffer& pb, const char* terminators)
   : Parameter(type), 
     mHasMagicCookie(false),
     mIsMyBranch(false),
     mTransactionId(Random::getRandomHex(8)),
     mTransportSeq(1),
     mClientData(),
     mInteropMagicCookie(0),
     mSigcompCompartment(Data::Empty)
{
   pb.skipWhitespace();
   pb.skipChar(Symbols::EQUALS[0]);
   pb.skipWhitespace();
   if (strncasecmp(pb.position(), Symbols::MagicCookie, 7) == 0)
   {
      mHasMagicCookie = true;
      if (strncmp(pb.position(), Symbols::MagicCookie, 7) != 0)
      {
         mInteropMagicCookie = new Data(pb.position(), 7);         
      }
      pb.skipN(7);
   }

   const char* start = pb.position();
   const char* end = pb.skipToOneOf(ParseBuffer::Whitespace, ";=?>"); // !dlb! add to ParseBuffer as terminator set

   if (mHasMagicCookie &&
       (end - start > 2*8) &&
       // look for prefix cookie
       (strncasecmp(start, Symbols::resipCookie, 8) == 0) &&
       // look for postfix cookie
       (strncasecmp(end - 8, Symbols::resipCookie, 8) == 0))
   {
      mIsMyBranch = true;
      start += 8;

      // s = start, e = end, S = anchorStart, E = anchorEnd, ^ = pb.position
      // rfc3261cookie-sip2cookie-tid-transportseq-clientdata-scid-sip2cookie
      //                          s                                          e
      //                                                                     ^

      pb.skipBackN(8);

      // Parse out SigComp Compartment Id
      const char* anchorEnd = pb.position();
      pb.skipBackToChar(Symbols::DASH[0]);
      const char* anchorStart = pb.position();
      // rfc3261cookie-sip2cookie-tid-transportseq-clientdata-scid-sip2cookie
      //                          s                           S   E          e
      //                                                      ^ 
      if ((anchorEnd - anchorStart) > 1)
      {
         pb.reset(anchorEnd);
         Data encoded;
         pb.data(encoded, anchorStart);
         mSigcompCompartment = encoded.base64decode();
         pb.reset(anchorStart);
      }
      pb.skipBackChar(Symbols::DASH[0]);

      // Parse out Client Data
      anchorEnd = pb.position();
      pb.skipBackToChar(Symbols::DASH[0]);
      anchorStart = pb.position();
      // rfc3261cookie-sip2cookie-tid-transportseq-clientdata-scid-sip2cookie
      //                          s                S         E               e
      //                                           ^

      if ((anchorEnd - anchorStart) > 1)
      {
         pb.reset(anchorEnd);
         Data encoded;
         pb.data(encoded, anchorStart);
         mClientData = encoded.base64decode();
         pb.reset(anchorStart);
      }
      
      pb.skipBackChar(Symbols::DASH[0]);
      pb.skipBackToChar(Symbols::DASH[0]);
      pb.skipBackChar(Symbols::DASH[0]);
      // rfc3261cookie-sip2cookie-tid-transportseq-clientdata-scid-sip2cookie
      //                          s  ^             S         E               e

      pb.data(mTransactionId, start);
      pb.skipChar();
      mTransportSeq = pb.integer();
      pb.reset(end);
   }
   else
   {
      pb.data(mTransactionId, start);
   }
}

BranchParameter::BranchParameter(ParameterTypes::Type type)
   : Parameter(type),
     mHasMagicCookie(true),
     mIsMyBranch(true),
     mTransactionId(Random::getRandomHex(8)),
     mTransportSeq(1),
     mInteropMagicCookie(0),
     mSigcompCompartment(Data::Empty)
{
}

BranchParameter::BranchParameter(const BranchParameter& other)
   : Parameter(other), 
     mHasMagicCookie(other.mHasMagicCookie),
     mIsMyBranch(other.mIsMyBranch),
     mTransactionId(other.mTransactionId),
     mTransportSeq(other.mTransportSeq),
     mClientData(other.mClientData),
     mSigcompCompartment(other.mSigcompCompartment)
{
   if (other.mInteropMagicCookie)
   {
      mInteropMagicCookie = new Data(*other.mInteropMagicCookie);
   }
   else
   {
      mInteropMagicCookie = 0;
   }
}

BranchParameter::~BranchParameter()
{
   delete mInteropMagicCookie;
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
      mClientData = other.mClientData;
      mSigcompCompartment = other.mSigcompCompartment;
      if (other.mInteropMagicCookie)
      {
         delete mInteropMagicCookie;         
         mInteropMagicCookie = new Data(*other.mInteropMagicCookie);
      }
      else
      {
         delete mInteropMagicCookie;
         mInteropMagicCookie = 0;
      }
   }
   return *this;
}

bool
BranchParameter::operator==(const BranchParameter& other)
{
   if (mIsMyBranch != other.mIsMyBranch ||
       mHasMagicCookie != other.mHasMagicCookie ||
       mTransportSeq != other.mTransportSeq ||
       mTransactionId != other.mTransactionId ||
       mClientData != other.mClientData ||
       mSigcompCompartment != other.mSigcompCompartment)
   {
      return false;
   }
   return true;
}

bool
BranchParameter::hasMagicCookie() const
{
   return mHasMagicCookie;
}

const Data& 
BranchParameter::getTransactionId() const
{
   return mTransactionId;
}

void
BranchParameter::incrementTransportSequence()
{
   assert(mIsMyBranch);
   mTransportSeq++;
}

Data&
BranchParameter::clientData()
{
    return mClientData;
}

const Data&
BranchParameter::clientData() const
{
    return mClientData;
}

/**
  @todo The encoding here could be more efficient.
*/
void
BranchParameter::setSigcompCompartment(const Data &id)
{
  if (id.size() == 0)
  {
    mSigcompCompartment = Data::Empty;
  }

  // These will often (but not always) be UUID URNs in angle brackets;
  // e.g.: <urn:uuid:fa33c72d-121f-47e8-42e2-1eb6e24aba64>

  // Ideally, we would detect this, strip out everything that isn't
  // hex, and convert the hex to raw data.

  mSigcompCompartment = id;
}

Data
BranchParameter::getSigcompCompartment() const
{
  return mSigcompCompartment;
}

void
BranchParameter::reset(const Data& transactionId)
{
   mHasMagicCookie = true;
   mIsMyBranch = true;
   delete mInteropMagicCookie;
   mInteropMagicCookie = 0;   

   mSigcompCompartment = Data::Empty;

   mTransportSeq = 1;
   if (!transactionId.empty())
   {
      mTransactionId = transactionId;
   }
   else
   {
      mTransactionId = Random::getRandomHex(8);
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
      if (mInteropMagicCookie)
      {
         stream << *mInteropMagicCookie;         
      }
      else
      {
         stream << Symbols::MagicCookie;
      }
   }
   if (mIsMyBranch)
   {
      stream << Symbols::resipCookie 
             << mTransactionId 
             << Symbols::DASH[0]
             << mTransportSeq
             << Symbols::DASH[0]
             << mClientData.base64encode(true/*safe URL*/)
             << Symbols::DASH[0]
             << mSigcompCompartment.base64encode(true)
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
