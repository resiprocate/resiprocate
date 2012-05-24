/**
    Helper to test the speed of of the random generator. Used
    to optimize the different flavors.

**/
#include <cstdlib>
#include <cmath>        // for sqrt
#include <iostream>
#include <vector>
#include <memory>	// for auto_ptr
#include <set>

// #include "rutil/Data.hxx"
#include "rutil/Random.hxx"
#include "rutil/Time.hxx"	// for ResipClock
#include "rutil/Lock.hxx"

#define RANDINT_PER_CYCLE (1000000)
#define RANDSEQ_PER_CYCLE (1000)

using namespace std;
using namespace resip;

class Barrier {
   public:
      Barrier(int numThreads)
	 : mCurId(0), mHaveCnt(0), mWantCnt(numThreads) { };
      void sync(int id, bool isMaster=false);
   protected:
      Condition mCond;
      Mutex mMutex;
      volatile int mCurId;
      volatile int mHaveCnt;
      int mWantCnt;

   public:
      static volatile int sPreWaitCnt;
      static volatile int sPostWaitCnt;
};

volatile int Barrier::sPreWaitCnt = 0;
volatile int Barrier::sPostWaitCnt = 0;

void
Barrier::sync(int id, bool isMaster)
{
   Lock datalock(mMutex);
   if ( isMaster )
   {
      mHaveCnt = 0;
      mCurId = id;
      mCond.broadcast();
   }
   else
   {
      while ( mCurId != id )
      {
	 ++sPreWaitCnt;
         mCond.wait(mMutex);
      }
      ++mHaveCnt;
      mCond.broadcast();
   }
   while ( mCurId==id && mHaveCnt < mWantCnt )
   {
       ++sPostWaitCnt;
       mCond.wait(mMutex);
   }
}

class TestDummyThread : public ThreadIf
{
   public:
      TestDummyThread() { };
      ~TestDummyThread() { };

      virtual void thread() { }
};

typedef std::set<Data> DataSet;

class TestRandomThread : public ThreadIf
{
   public:
      TestRandomThread(int runs, Barrier& barrier, int storeBytes)
	 : mNumRuns(runs), mBarrier(barrier), mStoreBytes(storeBytes),
         mDupCnt(0)
      { };
      ~TestRandomThread() { };

      virtual void thread();
      static void makeRandoms(int numCycles);
      void makeAndStoreRandoms(int numCycles);
      const DataSet& getRandoms() const { return mRandoms; };
      int getDupCnt() const { return mDupCnt; }

   protected:
      int mNumRuns;
      Barrier& mBarrier;
      int mStoreBytes;
      DataSet mRandoms;
      int mDupCnt;
};


void
TestRandomThread::makeRandoms(int numCycles)
{
   int idx, cidx;
   for (cidx=0; cidx < numCycles; cidx++)
   {
      for (idx = 0; idx < RANDINT_PER_CYCLE; idx++)
      {
	 int val = Random::getRandom();
	 (void)val;
      }
   }
}

void
TestRandomThread::makeAndStoreRandoms(int numCycles)
{
   int idx, cidx;
   for (cidx=0; cidx < numCycles; cidx++)
   {
      for (idx = 0; idx < RANDSEQ_PER_CYCLE; idx++)
      {
         Data foo = Random::getRandomHex(mStoreBytes);
         if (mRandoms.insert(foo).second == false )
         {
            ++mDupCnt;
            // std::cerr << "RandomHex produced intra-thread duplicate after " 
            //   << idx << " calls."<<std::endl;
         }
      }
   }
}


void
TestRandomThread::thread()
{
   // do once to get over initialization
   Random::getRandom();
   // std::cerr << "Thread initialized." << std::endl;

   mBarrier.sync(1);

   if (mStoreBytes == 0 )
   {
      makeRandoms(mNumRuns);
   }
   else
   {
      makeAndStoreRandoms(mNumRuns);
   }

   mBarrier.sync(2);
}

static UInt64
doSingleTest(int numCycles)
{
   UInt64 startUs = ResipClock::getTimeMicroSec();
   TestRandomThread::makeRandoms(numCycles);
   UInt64 doneUs = ResipClock::getTimeMicroSec();
   return doneUs - startUs;
}

static int
mergeCheckRandoms(DataSet& all, const DataSet& more)
{
   int dupCnt = 0;
   DataSet::const_iterator it = more.begin();
   // DataSet::iterator at = all.begin();
   for ( ; it != more.end(); ++it)
   {
      // STL is unbelieable: this form isn't supported:
      // std::pair<DataSet::iterator,bool> res = all.insert( at, *it);
      // So we have to do this NlgN style:
      std::pair<DataSet::iterator,bool> res = all.insert( *it);
      // at = res->first;
      if ( res.second == false )
      {
         ++dupCnt;
         // std::cerr << "Inter-thread duplicate found." << std::endl;
      }
   }
   return dupCnt;
}

static UInt64
doThreadedTest(int numCycles, int numThreads, int storeBytes)
{
   std::vector<TestRandomThread*> threadList;
   Barrier bar(numThreads);
   int pidx;
   for (pidx=0; pidx < numThreads; pidx++)
   {
      TestRandomThread* rth = new TestRandomThread(numCycles,bar, storeBytes);
      rth->run();
      threadList.push_back(rth);
   }
   // std::cerr << "Threads started." << std::endl;


   bar.sync(1, true);

   UInt64 startUs = ResipClock::getTimeMicroSec();
   // TestRandomThread::makeRandoms(numCycles);

   bar.sync(2, true);

   UInt64 doneUs = ResipClock::getTimeMicroSec();
   // std::cerr << "Threads finished."
   //    << " (barrier pre="<<Barrier::sPreWaitCnt
   //    << " post="<<Barrier::sPostWaitCnt
   //    <<")" << std::endl;

   DataSet allRandoms;
   int intraDupCnt = 0;
   int interDupCnt = 0;
   for (pidx=0; pidx < numThreads; pidx++)
   {
      TestRandomThread* rth = threadList[pidx];
      if ( storeBytes > 0 )
      {
         intraDupCnt += rth->getDupCnt();
         interDupCnt += mergeCheckRandoms(allRandoms, rth->getRandoms());
      }
      rth->join();
      delete rth;
   }
   if ( storeBytes > 0 )
   {
      std::cout << "Found "<<interDupCnt<< " inter-thread and "
         <<intraDupCnt<<" intra-thread duplicates." << std::endl;
   }
   return doneUs - startUs;
}

static void
doVariationTest(int numCycles, int numThreads, int numPass, int storeBytes)
{
   UInt64 msMin = 0, msMax = 0;
   UInt64 msSum = 0;
   UInt64 msSumSq = 0;
   int passIdx=0;
   for (passIdx=0; passIdx < numPass; passIdx++)
   {
      UInt64 usTot = numThreads<=0
         ?  doSingleTest(numCycles) 
         : doThreadedTest(numCycles, numThreads, storeBytes);
      UInt64 usPerCycle = usTot/numCycles;
#if 0
      std::cerr << numCycles << " cycles/thread (1M plain 32-bit ints)"
         << " with " << numThreads << " threads"
          << " took " << usTot<<"us (" << usPerCycle << "us/cycle)"
          << std::endl;
#endif
      UInt64 msPerCycle = usPerCycle/1000;
      msSum += msPerCycle;
      msSumSq += msPerCycle*msPerCycle;
      if ( msPerCycle < msMin || passIdx==0 )
         msMin = msPerCycle;
      if ( msPerCycle > msMax || passIdx==0 )
         msMax = msPerCycle;
   }
   double msAvg = msSum/(numPass+0.0);
   double msVar = msSumSq/(numPass+0.0) - msAvg*msAvg;
   double msStd = sqrt(msVar);
   double msStdPct = msStd/msAvg*100;
   fprintf(stderr,"RESULT:plain:%s:cycles=%d,threads=%d,passes=%d:min=%d,avg=%d,max=%d,std=%.1f(%.1f%%)[ms/cycle]\n",
         Random::getImplName(),
         numCycles, numThreads, numPass,
         (int)msMin,(int)msAvg,(int)msMax, msStd, msStdPct);
}


int main(int argc, char** argv)
{
   bool doSweep = true;
   int numCycles = 2;
   int numThreads = 3;
   int numPass = 10;
   int storeBytes = 0;

   {
      doSweep = false;
      if(argc >= 2)
         numCycles = atoi(argv[1]);
      if(argc >= 3)
         numThreads = atoi(argv[2]);
      if(argc >= 4)
         numPass = atoi(argv[3]);
      if(argc >= 5)
         storeBytes = atoi(argv[4]);
   }

   if (numCycles <= 0 || numThreads < -1 || numPass < 1)
   {
       std::cerr
          << "usage: testRandomThread [numCycles numThreads numPasses storeBytes]" << std::endl
           << "numCycles>0 is number of cycles to run in each thread." << std::endl
           << "    each cycle is one million random 32bit integers" << std::endl
           << "    each cycle is one thousand random sequences when storing (see below) " << std::endl
           << "numThreads>=-1 is number of threads to run " << std::endl
           << "    -1 ==> work in main thread only" << std::endl
           << "    0  ==> work in main thread, plus dummy thread doing nothing" << std::endl
           << "    1  ==> single working thread, plus dummy & main doing nothing" << std::endl
           << "    N  ==> this many working threads, plus dummy & main doing nothing" << std::endl
           << "numPasses>=1 is number of time to repeat test (for min/max/stdev) " << std::endl
           << "Reported metric (min/avg/max/stdev) are milliseconds per cycle for all threads to complete." << std::endl
           << "storeBytes>=0 will store random sequences of this many bytes" << std::endl
           << "    and check for duplicates. Mainly to check for thread safety." << std::endl
           << "    Timing numbers in this case are not so useful." << std::endl
           << "    0 ==> [default] Don't store, just time." << std::endl
           ;
      exit(-1);
   }

   std::auto_ptr<TestDummyThread> dummyThread;
   if ( numThreads >= 0 )
   {
       dummyThread.reset(new TestDummyThread);
       dummyThread->run();
   }

   std::cerr << "Starting..." << std::endl;

   {
      doVariationTest(numCycles, numThreads, numPass, storeBytes);
   }

   std::cerr << "Success." << std::endl;
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
 * vi: set shiftwidth=3 expandtab:
 */
