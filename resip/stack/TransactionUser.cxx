#include "precompile.h"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/MessageFilterRule.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TRANSACTION

using namespace resip;

/*ivr mod*/TransactionUser::TransactionUser(TransactionTermination t,
                                 ConnectionTermination c,
					  unsigned int maxTuFifoDurationSecs/*=0*/,
					  unsigned int maxTuMessages/*=0*/)
   : mFifo(maxTuFifoDurationSecs, maxTuMessages),
     mRuleList(),
     mDomainList(),
     mRegisteredForTransactionTermination(t == RegisterForTransactionTermination),
     mRegisteredForConnectionTermination(c == RegisterForConnectionTermination)
{
  // This creates a default message filter rule, which
  // handles all sip: and sips: requests.
  mRuleList.push_back(MessageFilterRule());
}

TransactionUser::TransactionUser(MessageFilterRuleList &mfrl, 
                                 TransactionTermination t,
                                 ConnectionTermination c)
  : mFifo(0, 0), 
    mRuleList(mfrl),
    mDomainList(),
    mRegisteredForTransactionTermination(t == RegisterForTransactionTermination),
    mRegisteredForConnectionTermination(c == RegisterForConnectionTermination)    
{
}

TransactionUser::~TransactionUser()
{
}

void 
TransactionUser::post(Message* msg)
{
  mFifo.add(msg, TimeLimitFifo<Message>::InternalElement);
}

void 
TransactionUser::postToTransactionUser(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   mFifo.add(msg, usage);
   //DebugLog (<< "TransactionUser::postToTransactionUser " << msg->brief() << " &=" << &mFifo << " size=" << mFifo.size());
}

unsigned int 
TransactionUser::size() const
{
   return mFifo.size();
}    

bool 
TransactionUser::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   return mFifo.wouldAccept(usage);
}

bool
TransactionUser::isForMe(const SipMessage& msg) const
{
   DebugLog (<< "Checking if " << msg.brief() << " is for me");
   // do this for each MessageFilterRule
   for (MessageFilterRuleList::const_iterator i = mRuleList.begin() ; 
        i != mRuleList.end() ; ++i)
   {
       DebugLog (<< "Checking rule...");
       if (i->matches(msg))
       {
          DebugLog (<< "Match!");
          return true;
       }       
   }
   DebugLog (<< "No matching rule found");
   return false;
}

bool 
TransactionUser::isMyDomain(const Data& domain) const
{
   return mDomainList.count(domain) > 0;
}

void TransactionUser::addDomain(const Data& domain)
{
   mDomainList.insert(domain);
}

EncodeStream& 
TransactionUser::encode(EncodeStream& strm) const
{
   strm << "TU: " << name() << " size=" << mFifo.size();
   return strm;
}

void
TransactionUser::setMessageFilterRuleList(MessageFilterRuleList &rules)
{
   mRuleList = rules;
}

bool 
TransactionUser::isRegisteredForTransactionTermination() const
{
   return mRegisteredForTransactionTermination;
}

bool
TransactionUser::isRegisteredForConnectionTermination() const
{
   return mRegisteredForConnectionTermination;
}

EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::TransactionUser& tu)
{
   tu.encode(strm);
   return strm;
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
