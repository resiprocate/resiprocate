#include "repro/Target.hxx"

#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Via.hxx"
#include "repro/Proxy.hxx"
#include "rutil/WinLeakCheck.hxx"

namespace repro
{

Target::Target()
   :mPriorityMetric(0),
   mShouldAutoProcess(true),
   mStatus(Candidate),
   mKeyValueStore(*Proxy::getTargetKeyValueStoreKeyAllocator())
{}

Target::Target(const resip::Uri& uri)
   :mPriorityMetric(0),
   mShouldAutoProcess(true),
   mStatus(Candidate),
   mKeyValueStore(*Proxy::getTargetKeyValueStoreKeyAllocator())
{  
   mRec.mContact.uri()=uri;
}

Target::Target(const resip::NameAddr& target)
   :mPriorityMetric(0),
   mShouldAutoProcess(true),
   mStatus(Candidate),
   mKeyValueStore(*Proxy::getTargetKeyValueStoreKeyAllocator())
{
   mRec.mContact=target;
}

Target::Target(const resip::ContactInstanceRecord& rec)
   :mPriorityMetric(0),
   mShouldAutoProcess(true),
   mStatus(Candidate),
   mRec(rec)
{
}

Target::~Target()
{
}

const resip::Data&
Target::tid() const
{
   return mVia.param(resip::p_branch).getTransactionId();
}

Target::Status&
Target::status()
{
   return mStatus;
}

const Target::Status&
Target::status() const
{
   return mStatus;
}

const resip::Via&
Target::setVia(const resip::Via& via)
{
   return mVia=via;
}

const resip::Via&
Target::via() const
{
   return mVia;
}

const resip::ContactInstanceRecord& 
Target::rec() const
{
   return mRec;
}

resip::ContactInstanceRecord& 
Target::rec()
{
   return mRec;
}

void 
Target::setRec(const resip::ContactInstanceRecord& rec)
{
   mRec=rec;
}

Target*
Target::clone() const
{
   return new Target(*this);
}

int
Target::getPriority() const
{
   return mPriorityMetric;
}

bool
Target::shouldAutoProcess() const
{
   return mShouldAutoProcess;
}

EncodeStream& 
operator<<(EncodeStream& strm, const repro::Target& t)
{
   strm << "Target: " << t.uri() << " status=" << t.status();
   return strm;
}

} // namespace repro

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
