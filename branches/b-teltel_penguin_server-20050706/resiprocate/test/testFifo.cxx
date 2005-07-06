#include <iostream>
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/FiniteFifo.hxx"
#include "resiprocate/os/TimeLimitFifo.hxx"
#include "resiprocate/os/Data.hxx"
#include <unistd.h>

using namespace resip;
using namespace std;

class Foo
{
   public:
      Foo(const Data& val)
         : mVal(val)
      {}

      Data mVal;
};

bool
isNear(int value, int reference, int epsilon=250)
{
   int diff = ::abs(value-reference);
   return (diff < epsilon);
}

int
main()
{
   Log::initialize(Log::Cout, Log::Debug, Data::Empty);

   Fifo<Foo> f;
   FiniteFifo<Foo> ff(5);

   {
      bool c;
      TimeLimitFifo<Foo> tlf(5, 10); // 5 seconds or 10 count limit

      assert(tlf.empty());
      assert(tlf.size() == 0);
      assert(tlf.timeDepth() == 0);

      c = tlf.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);
   
      assert(!tlf.empty());
      assert(tlf.size() == 1);
      cerr << tlf.timeDepth() << endl;
      assert(tlf.timeDepth() == 0);

      sleep(2);

      assert(!tlf.empty());
      assert(tlf.size() == 1);
      assert(tlf.timeDepth() > 1);

      delete tlf.getNext();

      assert(tlf.empty());
      assert(tlf.size() == 0);
      assert(tlf.timeDepth() == 0);

      c = tlf.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);
      sleep(3);
      c = tlf.add(new Foo("second"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);
      sleep(3);
      c = tlf.add(new Foo("nope"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(!c);
      c = tlf.add(new Foo("yep"), TimeLimitFifo<Foo>::IgnoreTimeDepth);
      assert(c);
      c = tlf.add(new Foo("internal"), TimeLimitFifo<Foo>::InternalElement);
      assert(c);

      Foo* fp = tlf.getNext();
      assert(fp->mVal == "first");
      delete fp;
      c = tlf.add(new Foo("third"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);

      while (!tlf.empty())
      {
         delete tlf.getNext();
      }
   }

   {
      TimeLimitFifo<Foo> tlfNS(5, 0); // 5 seconds, no count limit
      bool c;

      assert(tlfNS.empty());
      assert(tlfNS.size() == 0);
      assert(tlfNS.timeDepth() == 0);

      c = tlfNS.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);
      sleep(3);
      c = tlfNS.add(new Foo("second"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);
      sleep(3);
      c = tlfNS.add(new Foo("nope"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(!c);
      Foo* fp = tlfNS.getNext();
      assert(fp->mVal == "first");
      delete fp;
      c = tlfNS.add(new Foo("third"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }
   }

   {
      TimeLimitFifo<Foo> tlfNS(5, 0); // 5 seconds, no count limit
      bool c;

      assert(tlfNS.empty());
      assert(tlfNS.size() == 0);
      assert(tlfNS.timeDepth() == 0);

      for (int i = 0; i < 100; ++i)
      {
         c = tlfNS.add(new Foo(Data("element") + Data(i)), TimeLimitFifo<Foo>::EnforceTimeDepth);
         assert(c);
      }

      sleep(6);
      c = tlfNS.add(new Foo("nope"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(!c);

      c = tlfNS.add(new Foo("yep"), TimeLimitFifo<Foo>::IgnoreTimeDepth);
      assert(c);

      assert(tlfNS.size() == 101);

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }

      c = tlfNS.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }
   }

   {
      TimeLimitFifo<Foo> tlfNS(5, 10); // 5 seconds, limit 10 (2 reserved)
      bool c;

      assert(tlfNS.empty());
      assert(tlfNS.size() == 0);
      assert(tlfNS.timeDepth() == 0);

      for (int i = 0; i < 8; ++i)
      {
         c = tlfNS.add(new Foo(Data("element") + Data(i)), TimeLimitFifo<Foo>::EnforceTimeDepth);
         assert(c);
      }

      c = tlfNS.add(new Foo("nope"), TimeLimitFifo<Foo>::IgnoreTimeDepth);
      assert(!c);

      assert(tlfNS.size() == 8);
 
      c = tlfNS.add(new Foo("yep"), TimeLimitFifo<Foo>::InternalElement);
      assert(c);

      c = tlfNS.add(new Foo("yepAgain"), TimeLimitFifo<Foo>::InternalElement);
      assert(c);

      c = tlfNS.add(new Foo("hard nope!"), TimeLimitFifo<Foo>::InternalElement);
      assert(!c);

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }

      c = tlfNS.add(new Foo("first"), TimeLimitFifo<Foo>::EnforceTimeDepth);
      assert(c);

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }
   }

   {
      TimeLimitFifo<Foo> tlfNS(0, 0); // unlimited

      bool c;

      assert(tlfNS.empty());
      assert(tlfNS.size() == 0);
      assert(tlfNS.timeDepth() == 0);

      for (int i = 0; i < 100; ++i)
      {
         c = tlfNS.add(new Foo(Data("element") + Data(i)), TimeLimitFifo<Foo>::EnforceTimeDepth);
         assert(c);
         sleep(1);
      }

      while (!tlfNS.empty())
      {
         delete tlfNS.getNext();
      }
   }
   
   cerr << "All OK" << endl;
   return 0;
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
