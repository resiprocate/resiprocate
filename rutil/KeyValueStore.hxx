#if !defined(KeyValueStore_hxx)
#define KeyValueStore_hxx

#include <vector>
#include "rutil/Data.hxx"
#include "rutil/compat.hxx"

namespace resip
{
class KeyValueStoreKeyAllocator;

class KeyValueStore
{
public:
   typedef unsigned long Key;
   class KeyValueStoreKeyAllocator
   {
   public:
      KeyValueStoreKeyAllocator() : mNextKey(1)  { }
      KeyValueStore::Key allocateNewKey() { return mNextKey++; }
   private:
      friend class KeyValueStore;
      KeyValueStore::Key mNextKey;
   };

   KeyValueStore();
   KeyValueStore(const KeyValueStoreKeyAllocator& keyAllocator);
   virtual ~KeyValueStore();
   
   Key allocateNewKey();

   void setDataValue(Key key, const Data& value);
   const Data& getDataValue(Key key) const;
   Data& getDataValue(Key key);

   void setBoolValue(Key key, bool value) { mKeyValueStore[key].boolValue = value; }
   const bool& getBoolValue(Key key) const { return mKeyValueStore[key].boolValue; }
   bool& getBoolValue(Key key) { return mKeyValueStore[key].boolValue; }

   void setCharValue(Key key, char value) { mKeyValueStore[key].charValue = value; }
   const char& getCharValue(Key key) const { return mKeyValueStore[key].charValue; }
   char& getCharValue(Key key) { return mKeyValueStore[key].charValue; }

   void setShortValue(Key key, short value) { mKeyValueStore[key].shortValue = value; }
   const short& getShortValue(Key key) const { return mKeyValueStore[key].shortValue; }
   short& getShortValue(Key key) { return mKeyValueStore[key].shortValue; }

   void setUShortValue(Key key, unsigned short value) { mKeyValueStore[key].ushortValue = value; }
   const unsigned short& getUShortValue(Key key) const { return mKeyValueStore[key].ushortValue; }
   unsigned short& getUShortValue(Key key) { return mKeyValueStore[key].ushortValue; }

   void setIntValue(Key key, int value) { mKeyValueStore[key].intValue = value; }
   const int& getIntValue(Key key) const { return mKeyValueStore[key].intValue; }
   int& getIntValue(Key key) { return mKeyValueStore[key].intValue; }

   void setUIntValue(Key key, unsigned int value) { mKeyValueStore[key].uintValue = value; }
   const unsigned int& getUIntValue(Key key) const { return mKeyValueStore[key].uintValue; }
   unsigned int& getUIntValue(Key key) { return mKeyValueStore[key].uintValue; }

   void setUInt64Value(Key key, UInt64 value) { mKeyValueStore[key].uint64Value = value; }
   const UInt64& getUInt64Value(Key key) const { return mKeyValueStore[key].uint64Value; }
   UInt64& getUInt64Value(Key key) { return mKeyValueStore[key].uint64Value; }

protected:
   struct Value
   {
      resip::Data* dataValue;
      union
      {
         bool           boolValue;
         char           charValue;
         short          shortValue;
         unsigned short ushortValue;
         int            intValue;
         unsigned int   uintValue;
         UInt64         uint64Value;
      };
   };
   typedef std::vector<Value> KeyValueStoreContainer;
   KeyValueStoreContainer mKeyValueStore;
   KeyValueStoreKeyAllocator mKeyAllocator;

private:
   friend EncodeStream& operator<<(EncodeStream& strm, const KeyValueStore& store);
};

EncodeStream& operator<<(EncodeStream& strm, const KeyValueStore& store);

}

#endif

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
