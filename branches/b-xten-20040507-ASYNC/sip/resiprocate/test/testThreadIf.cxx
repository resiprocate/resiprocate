#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/os/Timer.hxx"

#include <unistd.h>
#include <iostream>
#include <vector>
#include <cassert>

using namespace resip;
using namespace std;

class Every4 : public ThreadIf
{
   public:
      void thread()
      {
         while(!waitForShutdown(4000))
         {
            cerr << Timer::getTimeMs() << endl;
         }
         cerr << "Stopped at: " << Timer::getTimeMs() << endl;
      }
};

class Old2 : public ThreadIf
{
    public:
        void thread()
        {
            while(!isShutdown())
            {
                sleep(2);
                cerr << Timer::getTimeMs() << endl;
            }
            cerr << "Stopped at: " << Timer::getTimeMs() << endl;
        }
};

class ShutdownSelf : public ThreadIf
{
    public:
        void thread()
        {
           while(!waitForShutdown(4000))
           {
           }
        }
};

class DerivedShutsDown : public ThreadIf
{
   public:
      DerivedShutsDown() {}
      ~DerivedShutsDown()
      {
         shutdown();
         join();
      }
         
      void thread()
      {
         while(!isShutdown())
         {
         }
      }
};



int main()
{
   {
      DerivedShutsDown d;
   }
      
   {
      vector<DerivedShutsDown*> threads;
      int numThreads = 20;
      
      for (int i=0; i < numThreads; i++)
      {
         threads.push_back(new DerivedShutsDown);
         threads.back()->run();
      }
      sleep(1);
      for (int i=0; i < numThreads; i++)
      {
         delete threads[i];
      }
      cerr << "finished many-thread test" << endl;
   }
   
   {
      DerivedShutsDown d;
      d.run();
      usleep(10000);
   }
   {
      ShutdownSelf* s= new ShutdownSelf();
      s->run();
      usleep(10000);
      s->shutdown();
      delete s;
   }
   {
        Every4 e;
        e.run();
        sleep(5);
        UInt64 t = Timer::getTimeMs();
        cerr << "Stopping thread at: " << Timer::getTimeMs() << endl;
        e.shutdown();
        e.join();
        UInt64 j = Timer::getTimeMs();
        cerr << j - t << endl;
        
        assert(j - t < 10);
    }
    //this should not cause problems(signal is being called on a condition with
    //no waiting threads).
    {
        Old2 o;
        o.run();
        sleep(3);
        o.shutdown();
        o.join();
    }
    cerr << "OK" << endl;
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
