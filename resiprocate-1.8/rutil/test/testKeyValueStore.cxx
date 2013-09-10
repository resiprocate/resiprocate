#include <iostream>
#include "rutil/Log.hxx"
#include "rutil/KeyValueStore.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace resip;
using namespace std;

void sleepMS(unsigned int ms)
{
#ifdef WIN32
   Sleep(ms);
#else
   usleep(ms*1000);
#endif
}

class Foo
{
public:
   Foo()
      : mDataValueKey(mStore.allocateNewKey())
      , mBoolValueKey(mStore.allocateNewKey())
      , mCharValueKey(mStore.allocateNewKey())
      , mShortValueKey(mStore.allocateNewKey())
      , mUShortValueKey(mStore.allocateNewKey())
      , mIntValueKey(mStore.allocateNewKey())
      , mUIntValueKey(mStore.allocateNewKey())
      , mUInt64ValueKey(mStore.allocateNewKey())
   {
      assert(mStore.getDataValue(mDataValueKey) == Data::Empty);
      assert(mStore.getBoolValue(mBoolValueKey) == false);
      assert(mStore.getCharValue(mCharValueKey) == 0);
      assert(mStore.getShortValue(mShortValueKey) == 0);
      assert(mStore.getUShortValue(mUShortValueKey) == 0);
      assert(mStore.getIntValue(mIntValueKey) == 0);
      assert(mStore.getUIntValue(mUIntValueKey) == 0);
      assert(mStore.getUInt64Value(mUInt64ValueKey) == 0);
   }

   ~Foo()
   {
   }

   void getSetTest()
   {
      mStore.setDataValue(mDataValueKey, "test");
      mStore.setBoolValue(mBoolValueKey, true);
      mStore.setCharValue(mCharValueKey, 'x');
      short testShort = -1000;
      mStore.setShortValue(mShortValueKey, testShort);
      unsigned short testUShort = 40000;
      mStore.setUShortValue(mUShortValueKey, testUShort);
      int testInt = -80000;
      mStore.setIntValue(mIntValueKey, testInt);
      unsigned int testUInt = 4294967295UL;
      mStore.setUIntValue(mUIntValueKey, testUInt);
      UInt64 testUInt64 = 18446744073709551615ULL;
      mStore.setUInt64Value(mUInt64ValueKey, testUInt64);

      assert(mStore.getDataValue(mDataValueKey) == "test");
      assert(mStore.getBoolValue(mBoolValueKey) == true);
      assert(mStore.getCharValue(mCharValueKey) == 'x');
      assert(mStore.getShortValue(mShortValueKey) == testShort);
      assert(mStore.getUShortValue(mUShortValueKey) == testUShort);
      assert(mStore.getIntValue(mIntValueKey) == testInt);
      assert(mStore.getUIntValue(mUIntValueKey) == testUInt);
      assert(mStore.getUInt64Value(mUInt64ValueKey) == testUInt64);

      checkConstAccessors(mStore);
   }

   void checkConstAccessors(const KeyValueStore& store)
   {
      KeyValueStore::Key newDataValueKey = mStore.allocateNewKey();
      assert(store.getDataValue(newDataValueKey) == Data::Empty);
      assert(store.getDataValue(mDataValueKey) == "test");
      assert(store.getBoolValue(mBoolValueKey) == true);
      assert(store.getCharValue(mCharValueKey) == 'x');
      assert(store.getShortValue(mShortValueKey) == -1000);
      assert(store.getUShortValue(mUShortValueKey) == 40000);
      assert(store.getIntValue(mIntValueKey) == -80000);
      assert(store.getUIntValue(mUIntValueKey) == 4294967295UL);
      assert(store.getUInt64Value(mUInt64ValueKey) == 18446744073709551615ULL);
   }

   void getModifyTest()
   {
      KeyValueStore::Key newDataValueKey = mStore.allocateNewKey();

      mStore.getDataValue(newDataValueKey) = "newDataValue";
      mStore.getDataValue(mDataValueKey) = "test2";
      mStore.getBoolValue(mBoolValueKey) = false;
      mStore.getCharValue(mCharValueKey) = 'y';
      short testShort = -2000;
      mStore.getShortValue(mShortValueKey) = testShort;
      unsigned short testUShort = 40001;
      mStore.getUShortValue(mUShortValueKey) = testUShort;
      int testInt = -80001;
      mStore.getIntValue(mIntValueKey) = testInt;
      unsigned int testUInt = 4294967294UL;
      mStore.getUIntValue(mUIntValueKey) = testUInt;
      UInt64 testUInt64 = 18446744073709551614ULL;
      mStore.getUInt64Value(mUInt64ValueKey) = testUInt64;

      assert(mStore.getDataValue(newDataValueKey) == "newDataValue");
      assert(mStore.getDataValue(mDataValueKey) == "test2");
      assert(mStore.getBoolValue(mBoolValueKey) == false);
      assert(mStore.getCharValue(mCharValueKey) == 'y');
      assert(mStore.getShortValue(mShortValueKey) == testShort);
      assert(mStore.getUShortValue(mUShortValueKey) == testUShort);
      assert(mStore.getIntValue(mIntValueKey) == testInt);
      assert(mStore.getUIntValue(mUIntValueKey) == testUInt);
      assert(mStore.getUInt64Value(mUInt64ValueKey) == testUInt64);
   }

   KeyValueStore mStore;
   KeyValueStore::Key mDataValueKey;
   KeyValueStore::Key mBoolValueKey;
   KeyValueStore::Key mCharValueKey;
   KeyValueStore::Key mShortValueKey;
   KeyValueStore::Key mUShortValueKey;
   KeyValueStore::Key mIntValueKey;
   KeyValueStore::Key mUIntValueKey;
   KeyValueStore::Key mUInt64ValueKey;
};

int
main()
{
   Log::initialize(Log::Cout, Log::Debug, Data::Empty);
   
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   FindMemoryLeaks fml;
#endif

   {
      cerr << "!! Test initialization" << endl;
      Foo foo;
      foo.getSetTest();
      foo.getSetTest(); // Set and get a second time
      foo.getModifyTest();
   }

   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
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
