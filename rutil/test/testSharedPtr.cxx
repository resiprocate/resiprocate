#include <iostream>
#include "rutil/Log.hxx"
#include "rutil/SharedPtr.hxx"
#include "rutil/Data.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Timer.hxx"
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
      Foo(const Data& val)
         : mVal(val)
      {
          cerr << "Foo created." << endl;
      }

      ~Foo()
      {
          cerr << "Foo destroyed." << endl;
      }

      Data mVal;
};

class TestThread : public ThreadIf
{
  public: 
      TestThread(SharedPtr<Foo>&);
      virtual ~TestThread() 
      {
#ifdef VERBOSE
         cerr << "TestThread finishing..." << endl;
#endif
         shutdown();
         join();
#ifdef VERBOSE
         cerr << "TestThread finished" << endl;
#endif
      };

      void thread();

   private:
      SharedPtr<Foo> mFoo;
};

TestThread::TestThread(SharedPtr<Foo>& f) :
   mFoo(f)
{}

void TestThread::thread()
{
    while (!mShutdown) 
    {
       sleepMS(5);
       assert(mFoo->mVal == Data("data"));
    }
    mFoo.reset();
}

int
main()
{
   Log::initialize(Log::Cout, Log::Debug, Data::Empty);
   
   {
      cerr << "!! Test counter logic" << endl;

      SharedPtr<Foo> foo1(new Foo("data"));
      cerr << "foo1 mVal=" << foo1->mVal << " use count=" << foo1.use_count() << endl;
      assert(foo1.use_count() == 1);
      {
         SharedPtr<Foo> foo2(foo1);

         cerr << "foo1 mVal=" << foo1->mVal << " use count=" << foo1.use_count() << endl;
         assert(foo1.use_count() == 2);
         cerr << "foo2 mVal=" << foo2->mVal << " use count=" << foo2.use_count() << endl;
         assert(foo2.use_count() == 2);
      }
      cerr << "foo1 mVal=" << foo1->mVal << " use count=" << foo1.use_count() << endl;
      assert(foo1.use_count() == 1);
   }

   {
      cerr << "!! Test threads" << endl;

      SharedPtr<Foo> foo(new Foo("data"));

      cerr << "foo mVal=" << foo->mVal << " use count=" << foo.use_count() << endl;
      assert(foo.use_count() == 1);

      TestThread thread1(foo);
      TestThread thread2(foo);
      TestThread thread3(foo);
      TestThread thread4(foo);
      TestThread thread5(foo);
      TestThread thread6(foo);
      TestThread thread7(foo);
      TestThread thread8(foo);
      TestThread thread9(foo);
      TestThread thread10(foo);

      cerr << "foo mVal=" << foo->mVal << " use count=" << foo.use_count() << endl;
      assert(foo.use_count() == 11);

      thread1.run();
      thread2.run();
      thread3.run();
      thread4.run();
      thread5.run();
      thread6.run();
      thread7.run();
      thread8.run();
      thread9.run();
      thread10.run();

      sleepMS(500);

      thread1.shutdown();
      thread2.shutdown();
      thread3.shutdown();
      thread4.shutdown();
      thread5.shutdown();
      thread6.shutdown();
      thread7.shutdown();
      thread8.shutdown();
      thread9.shutdown();
      thread10.shutdown();

      thread1.join();
      thread2.join();
      thread3.join();
      thread4.join();
      thread5.join();
      thread6.join();
      thread7.join();
      thread8.join();
      thread9.join();
      thread10.join();

      cerr << "foo mVal=" << foo->mVal << " use count=" << foo.use_count() << endl;
      assert(foo.use_count() == 1);
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
