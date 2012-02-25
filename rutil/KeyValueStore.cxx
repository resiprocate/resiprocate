#include "rutil/KeyValueStore.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{

KeyValueStore::KeyValueStore()
{
}

KeyValueStore::KeyValueStore(const KeyValueStoreKeyAllocator& keyAllocator) :
   mKeyAllocator(keyAllocator)
{
   if(mKeyAllocator.mNextKey > 1)  // Only resize if there are allocated keys
   {
      Value defaultValue;
      defaultValue.dataValue = 0;
      defaultValue.uint64Value = 0;  // largest member of union - will zero out all data
      mKeyValueStore.resize(mKeyAllocator.mNextKey, defaultValue);
   }
}

KeyValueStore::~KeyValueStore()
{
   KeyValueStoreContainer::iterator it = mKeyValueStore.begin();
   for(; it != mKeyValueStore.end(); it++)
   {
      delete it->dataValue;
   }
}

KeyValueStore::Key 
KeyValueStore::allocateNewKey()
{
   Value defaultValue;
   defaultValue.dataValue = 0;
   defaultValue.uint64Value = 0;  // largest member of union - will zero out all data
   Key key = mKeyAllocator.allocateNewKey();
   mKeyValueStore.resize(key+1, defaultValue);
   return key;
}

void 
KeyValueStore::setDataValue(Key key, const Data& value) 
{ 
   if(mKeyValueStore[key].dataValue)
   {
      *mKeyValueStore[key].dataValue = value;
   }
   else
   {
      mKeyValueStore[key].dataValue = new Data(value);
   }
}

const Data& 
KeyValueStore::getDataValue(Key key) const 
{
   if(!mKeyValueStore[key].dataValue)
   {
      return Data::Empty;
   }
   return *mKeyValueStore[key].dataValue; 
}

Data& 
KeyValueStore::getDataValue(Key key) 
{ 
   if(!mKeyValueStore[key].dataValue)
   {
      mKeyValueStore[key].dataValue = new Data();
   }
   return *mKeyValueStore[key].dataValue; 
}

EncodeStream& 
operator<<(EncodeStream& strm, const KeyValueStore& store)
{
   strm << "[KeyValueStore]";
   return strm;
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
