#include "resip/stack/TupleMarkManager.hxx"

#include "resip/stack/MarkListener.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Timer.hxx"

namespace resip
{

TupleMarkManager::MarkType 
TupleMarkManager::getMarkType(const Tuple& tuple)
{
   ListEntry entry(tuple,0);
   TupleList::iterator i=mList.find(entry);
   
   if(i!=mList.end())
   {
      UInt64 now=Timer::getTimeMs();
      if(i->first.mExpiry > now)
      {
         return i->second;
      }
      else
      {
         mList.erase(i);
         // ?bwc? Should we do this?
         UInt64 expiry = 0;
         MarkType mark = OK;
         notifyListeners(tuple,expiry,mark);
      }
   }
   
   return OK;
}

void TupleMarkManager::mark(const Tuple& tuple,UInt64 expiry,MarkType mark)
{
   // .amr. Notify listeners first so they can change the entry if they want
   notifyListeners(tuple,expiry,mark);
   ListEntry entry(tuple,expiry);
   mList[entry]=mark;
}

void TupleMarkManager::registerMarkListener(MarkListener* listener)
{
   mListeners.insert(listener);
}

void TupleMarkManager::unregisterMarkListener(MarkListener* listener)
{
   mListeners.erase(listener);
}

void
TupleMarkManager::notifyListeners(const resip::Tuple& tuple, UInt64& expiry, MarkType& mark)
{
   for(Listeners::iterator i = mListeners.begin(); i!=mListeners.end(); ++i)
   {
      (*i)->onMark(tuple,expiry,mark);
   }
}

TupleMarkManager::ListEntry::ListEntry(const Tuple& tuple, UInt64 expiry)
   : mTuple(tuple),
   mExpiry(expiry)
{}

TupleMarkManager::ListEntry::ListEntry(const TupleMarkManager::ListEntry& orig)
   : mTuple(orig.mTuple),
   mExpiry(orig.mExpiry)
{}

TupleMarkManager::ListEntry::~ListEntry()
{}

bool 
TupleMarkManager::ListEntry::operator<(const TupleMarkManager::ListEntry& rhs) const
{
   if(mTuple < rhs.mTuple)
   {
      return true;
   }
   else if(rhs.mTuple < mTuple)
   {
      return false;
   }
   
   return mTuple.getTargetDomain() < rhs.mTuple.getTargetDomain();
}

bool 
TupleMarkManager::ListEntry::operator>(const TupleMarkManager::ListEntry& rhs) const
{
   if(rhs.mTuple < mTuple)
   {
      return true;
   }
   else if(mTuple < rhs.mTuple)
   {
      return false;
   }
   
   return mTuple.getTargetDomain() > rhs.mTuple.getTargetDomain();
}

bool 
TupleMarkManager::ListEntry::operator==(const TupleMarkManager::ListEntry& rhs) const
{
   return (mTuple==rhs.mTuple && mTuple.getTargetDomain()==rhs.mTuple.getTargetDomain());
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
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
