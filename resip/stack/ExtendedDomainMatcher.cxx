#include "rutil/Data.hxx"
#include "resip/stack/ExtendedDomainMatcher.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

using namespace resip;

ExtendedDomainMatcher::ExtendedDomainMatcher() :
   mDomainSuffixList()
{
}

ExtendedDomainMatcher::~ExtendedDomainMatcher()
{
}

bool
ExtendedDomainMatcher::isMyDomain(const Data& domain) const
{
   if(BasicDomainMatcher::isMyDomain(domain))
   {
      return true;
   }

   // Domain search should be case insensitive - search in lowercase only
   Data _domain(domain);
   _domain.lowercase();

   if(mDomainSuffixList.empty())
   {
      return false;
   }

   static const Data dot(".");
   Data::size_type i = 0;
   for(i = _domain.find(dot, i); i < _domain.size() && i != Data::npos; i++)
   {
      Data _search = _domain.substr(i);
      if(mDomainSuffixList.count(_search) > 0)
      {
         return true;
      }
   }
   return false;
}

void
ExtendedDomainMatcher::addDomainSuffix(const Data& domainSuffix)
{
   // Domain search should be case insensitive - store in lowercase only
   mDomainSuffixList.insert(Data(domainSuffix).lowercase());
}

void
ExtendedDomainMatcher::removeDomainSuffix(const Data& domainSuffix)
{
   DomainSuffixList::iterator it = mDomainSuffixList.find(Data(domainSuffix).lowercase());
   if (it != mDomainSuffixList.end())
   {
      mDomainSuffixList.erase(it);
   }
}

/* ====================================================================
 *
 * Copyright 2017 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

