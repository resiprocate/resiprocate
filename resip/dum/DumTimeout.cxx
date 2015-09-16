#include "rutil/ResipAssert.h"
#include "DumTimeout.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/dum/BaseUsage.hxx"

using namespace resip;

DumTimeout::DumTimeout(Type type, 
                       unsigned long duration, 
                       BaseUsageHandle targetBu, 
                       unsigned int seq, 
                       unsigned int altSeq,
                       const Data &transactionId)
    : mType(type),
      mDuration(duration),
      mUsageHandle(targetBu),
      mSeq(seq),
      mSecondarySeq(altSeq),
      mTransactionId(transactionId)
{}

DumTimeout::DumTimeout(const DumTimeout& source)
    : mType(source.mType),
      mDuration(source.mDuration),
      mUsageHandle(source.mUsageHandle),
      mSeq(source.mSeq),
      mSecondarySeq(source.mSecondarySeq),
      mTransactionId(source.mTransactionId)
{}

DumTimeout::~DumTimeout()
{}

Message*
DumTimeout::clone() const
{
   return new DumTimeout(*this);
}
      
DumTimeout::Type 
DumTimeout::type() const
{
   return mType;
}

unsigned int 
DumTimeout::seq() const
{
   return mSeq;
}

unsigned int 
DumTimeout::secondarySeq() const
{
   return mSecondarySeq;
}  

const Data & DumTimeout::transactionId() const
{
	return mTransactionId;
}

bool 
DumTimeout::isClientTransaction() const
{
   resip_assert(0);
   return false;
}
      
EncodeStream&
DumTimeout::encodeBrief(EncodeStream& strm) const
{
   return encode(strm);
}

EncodeStream& 
DumTimeout::encode(EncodeStream& strm) const
{
   strm << "DumTimeout::";
   switch (mType)
   {
      case SessionExpiration:
         strm <<"SessionExpiration";
         break;
      case SessionRefresh:
         strm <<"SessionRefresh";
         break;
      case Registration:
         strm <<"Registration";
         break;
      case RegistrationRetry:
         strm <<"RegistrationRetry";
         break;
      case Publication:
         strm <<"Publication";
         break;
      case Retransmit200:
         strm <<"Retransmit200";
         break;
      case Retransmit1xx:
         strm <<"Retransmit1xx";
         break;
      case Retransmit1xxRel:
         strm <<"Retransmit1xxRel";
         break;
      case Resubmit1xxRel:
         strm <<"Resubmit1xxRel";
         break;
      case WaitForAck:
         strm <<"WaitForAck";
         break;
      case CanDiscardAck:
         strm <<"CanDiscardAck";
         break;
      case StaleCall:
         strm <<"StaleCall";
         break;
      case Subscription:
         strm <<"Subscription";
         break;
      case SubscriptionRetry:
         strm <<"SubscriptionRetry";
         break;
      case WaitForNotify:
         strm <<"WaitForNotify";
         break;
      case StaleReInvite:
         strm <<"StaleReInvite";
         break;
      case Glare:
         strm <<"Glare";
         break;
      case Cancelled:
         strm <<"Cancelled";
         break;
      case WaitingForForked2xx:
         strm <<"WaitingForForked2xx";
         break;
      case SendNextNotify:
         strm <<"SendNextNotify";
         break;
   }
   // Accessing mUsageHandle is not threadsafe, and this encode method is used outside
   // the dum thread, in the stack thread via the TuSelector::add method for logging.
   //if (mUsageHandle.isValid()) 
   //{
   //   strm << " " << *mUsageHandle;
   //}
   //else 
   //{
   //   strm << " defunct";
   //}
   
   strm << ": duration=" << mDuration << " seq=" << mSeq;
   return strm;
}

BaseUsageHandle 
DumTimeout::getBaseUsage() const
{
   return mUsageHandle;
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
