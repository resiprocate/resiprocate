#include "repro/QValueTarget.hxx"
#include "rutil/WinLeakCheck.hxx"


namespace repro
{

static int DefaultPriorityMetric = 1000;  // If no q-value is present, when we treat the target as having a 1.0 priority

QValueTarget::QValueTarget(const resip::Uri& target) :
   Target(target)
{
   // Note:  no neeed to call storePriorityMetric(), since q value parameter 
   //        is a NameAddr parameter and all we have here is a URI
}

QValueTarget::QValueTarget(const resip::NameAddr& target) :
   Target(target)
{
   storePriorityMetric();
}

QValueTarget::QValueTarget(const resip::ContactInstanceRecord& rec) :
   Target(rec)
{
   storePriorityMetric();
}
   
QValueTarget::~QValueTarget()
{
}

QValueTarget* 
QValueTarget::clone() const
{
   return new QValueTarget(*this);
}

void 
QValueTarget::storePriorityMetric()
{
   if(mRec.mContact.exists(resip::p_q))
   {
      try
      {
         mPriorityMetric=mRec.mContact.param(resip::p_q);
      }
      catch(resip::ParseBuffer::Exception& /*e*/)
      {
         mPriorityMetric=DefaultPriorityMetric;
      }
   }
   else
   {
      mPriorityMetric=DefaultPriorityMetric;
   }
}

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
 */
