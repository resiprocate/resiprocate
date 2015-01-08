#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/MessageFilterRule.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TRANSACTION

using namespace resip;

TransactionUser::TransactionUser(TransactionTermination t,
                                 ConnectionTermination c,
                                 KeepAlivePongs k) : 
   mFifo(0, 0),
   mCongestionManager(0),
   mRuleList(),
   mDomainList(),
   mRegisteredForTransactionTermination(t == RegisterForTransactionTermination),
   mRegisteredForConnectionTermination(c == RegisterForConnectionTermination),
   mRegisteredForKeepAlivePongs(k == RegisterForKeepAlivePongs)
{
  // This creates a default message filter rule, which
  // handles all sip:, sips:, and tel: requests.
  mRuleList.push_back(MessageFilterRule());

  // Set a default Fifo description - should be modified by override class to be
  // more desriptive
   mFifo.setDescription("TransactionUser::mFifo");
}

TransactionUser::TransactionUser(MessageFilterRuleList &mfrl, 
                                 TransactionTermination t,
                                 ConnectionTermination c,
                                 KeepAlivePongs k) : 
   mFifo(0, 0), 
   mCongestionManager(0),
   mRuleList(mfrl),
   mDomainList(),
   mRegisteredForTransactionTermination(t == RegisterForTransactionTermination),
   mRegisteredForConnectionTermination(c == RegisterForConnectionTermination),
   mRegisteredForKeepAlivePongs(k == RegisterForKeepAlivePongs)
{
  // Set a default Fifo description - should be modified by override class to be
  // more desriptive
   mFifo.setDescription("TransactionUser::mFifo");
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
   // do this for each MessageFilterRule
   for (MessageFilterRuleList::const_iterator i = mRuleList.begin() ; 
        i != mRuleList.end() ; ++i)
   {
       DebugLog(<< "TransactionUser::isForMe: TU=" << name() << ", Checking rule... : " << msg.brief());
       if (i->matches(msg))
       {
           DebugLog(<< "TransactionUser::isForMe: TU=" << name() << ", Match! : " << msg.brief());
          return true;
       }       
   }
   DebugLog(<< "TransactionUser::isForMe: TU=" << name() << ", No matching rule found : " << msg.brief());
   return false;
}

bool 
TransactionUser::isMyDomain(const Data& domain) const
{
   // Domain search should be case insensitive - search in lowercase only
   return mDomainList.count(Data(domain).lowercase()) > 0;
}

void TransactionUser::addDomain(const Data& domain)
{
   // Domain search should be case insensitive - store in lowercase only
   mDomainList.insert(Data(domain).lowercase());  
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
   MessageFilterRuleList::iterator it = mRuleList.begin();
   for(;it!=mRuleList.end();it++)
   {
      it->setTransactionUser(this);
   }
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

bool
TransactionUser::isRegisteredForKeepAlivePongs() const
{
   return mRegisteredForKeepAlivePongs;
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
