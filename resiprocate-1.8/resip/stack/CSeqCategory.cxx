#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/CSeqCategory.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// CSeqCategory:
//====================
CSeqCategory::CSeqCategory(const HeaderFieldValue& hfv, 
                           Headers::Type type,
                           PoolBase* pool)
   : ParserCategory(hfv, type, pool), mMethod(UNKNOWN), mSequence(0) 
{}

CSeqCategory::CSeqCategory() 
   : ParserCategory(), 
     mMethod(UNKNOWN), 
     mUnknownMethodName(getMethodName(UNKNOWN)),
     mSequence(0) 
{}

CSeqCategory::CSeqCategory(const CSeqCategory& rhs, PoolBase* pool)
   : ParserCategory(rhs, pool),
     mMethod(rhs.mMethod),
     mUnknownMethodName(rhs.mUnknownMethodName),
     mSequence(rhs.mSequence)
{}

CSeqCategory&
CSeqCategory::operator=(const CSeqCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mMethod = rhs.mMethod;
      mUnknownMethodName = rhs.mUnknownMethodName;
      mSequence = rhs.mSequence;
   }
   return *this;
}

bool
CSeqCategory::operator==(const CSeqCategory& rhs) const
{
   return (mMethod == rhs.mMethod &&
           (mMethod != UNKNOWN || mUnknownMethodName == rhs.mUnknownMethodName) &&
           mSequence == rhs.mSequence);
}

bool
CSeqCategory::operator<(const CSeqCategory& rhs) const
{
   if (mUnknownMethodName < rhs.mUnknownMethodName) 
   {
      return true;
   }
   else if (mUnknownMethodName > rhs.mUnknownMethodName) 
   {
      return false;
   }
   
   return mSequence < rhs.mSequence;
}


ParserCategory* 
CSeqCategory::clone() const
{
   return new CSeqCategory(*this);
}

ParserCategory* 
CSeqCategory::clone(void* location) const
{
   return new (location) CSeqCategory(*this);
}

ParserCategory* 
CSeqCategory::clone(PoolBase* pool) const
{
   return new (pool) CSeqCategory(*this, pool);
}

MethodTypes& 
CSeqCategory::method()
{
   checkParsed(); 
   return mMethod;
}

MethodTypes 
CSeqCategory::method() const 
{
   checkParsed(); return mMethod;
}

Data&
CSeqCategory::unknownMethodName()
{
   checkParsed(); 
   return mUnknownMethodName;
}

const Data& 
CSeqCategory::unknownMethodName() const 
{
   checkParsed(); 
   return mUnknownMethodName;
}

unsigned int& 
CSeqCategory::sequence()
{
   checkParsed(); 
   return mSequence;
}

unsigned int 
CSeqCategory::sequence() const
{
   checkParsed(); 
   return mSequence;
}

// examples to test: 
// "CSeq:15 ACK"  // ok
// "CSeq:ACK"     // bad
// "CSeq:JOE"     // ok
// "CSeq:1 JOE"   // ok
// "CSeq:1323333 INVITE" // ok 
// "CSeq:1323333 Invite" // ok - not invite
// "CSeq:1323333 InviTe" // ok - not invite
// "CSeq:\t\t  \t15\t\t\t    \t ACK"  // ok
// "CSeq:\t\t  \t15\t\t\t    \t"  // bad
// "CSeq:1xihzihsihtqnognsd INVITE" // not ok, but parses (?)

void
CSeqCategory::parse(ParseBuffer& pb)
{
   pb.skipWhitespace();
   mSequence = pb.uInt32();

   const char* anchorPtr = pb.skipWhitespace();
   pb.skipNonWhitespace(); // .dcm. maybe pass an arg that says throw if you
                           // don't move
   mMethod = getMethodType(anchorPtr, int(pb.position() - anchorPtr));
   // for backward compatibility, set the method name even if the method is known
   pb.data(mUnknownMethodName, anchorPtr);
}

EncodeStream& 
CSeqCategory::encodeParsed(EncodeStream& str) const
{
   str << mSequence 
       << Symbols::SPACE 
       << (mMethod != UNKNOWN ? getMethodName(mMethod) : mUnknownMethodName);
   return str;
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
