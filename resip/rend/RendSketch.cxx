/**
    REND Sketch base and factory classes

    Written by Kennard White (Logitech, Inc.), 2010..2011
**/

// #include "boost/intrusive/list.hpp"

#include "rutil/Logger.hxx"
// #include "rutil/Random.hxx"

// #include "RendAcct.hxx"
// #include "RendDlg.hxx"
// #include "RendDlgAcct.hxx"
// #include "RendSketch.hxx"

// #include "popt.h"

#include "RendSketch.hxx"
#include "RendTroop.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

static const char* RendSketchRunStateTbl[] = 
{
        "none", "init", "running", "stopping", "finished",
};

RendSketchBase::~RendSketchBase() 
{
   // WATCHOUT: subclass has already destructed, so key state
   // (like troops) might be gone

   // make sure main loop is done
   resip_assert( mRunCurStart == 0 );
   resip_assert( mRunState == REND_SKETCH_RS_FINISHED);

   deleteTroops();
}

void
RendSketchBase::runTimerGo(RendTimeUs now) 
{
   if ( mRunCurStart != 0 )
      return;  // already running
   mRunCurStart = now;
}

void
RendSketchBase::runTimerPause(RendTimeUs now) 
{
   if ( mRunCurStart != 0 )
      mRunPriorDuration += now - mRunCurStart;
   mRunCurStart = 0;
}

unsigned
RendSketchBase::runTimerGetSecs(RendTimeUs now) 
{
   unsigned secs = REND_US2S(mRunPriorDuration);
   if ( mRunCurStart != 0 )
      secs += REND_US2S(now-mRunCurStart);
   return secs;
}

void
RendSketchBase::getTroopReportCore(RendTimeUs now, RendTroopReport& rpt) 
{
   unsigned idx;
   for (idx=0; idx < mTroopList.size(); idx++) 
   {
      mTroopList[idx]->addTroopReport(rpt);
   }
}

int
RendSketchBase::doSimpleAction(RendSimpleAction act)
{
   resip::Lock lock(mUpperMutex);
   unsigned idx;
   RendTimeUs now = RendGetTimeUsRel();

   switch ( act ) {
    case REND_SA_Ping:
    case REND_SA_SkipPendNotify:
       for (idx=0; idx < mTroopList.size(); idx++) 
       {
          int sts = mTroopList[idx]->doSimpleAction(now, act);
          if ( sts < 0 ) 
          {
             WarningLog(<<"Action "<<act<<" on troop "<<idx<<" failed: sts="
                <<sts);
          }
       }
       return 0;
    default:
       ;
   }
   return -2;
}

int
RendSketchBase::getTroopReport(RendTroopReport& rpt) 
{
   resip::Lock lock(mUpperMutex);
   RendTimeUs now = RendGetTimeUsRel();

   getTroopReportCore(now, rpt);
   rpt.mRunState = mRunState;
   rpt.mRunStateStr = "bogus";
   if (mRunState>=0 && mRunState<(int)REND_NELTS(RendSketchRunStateTbl))
   {
      rpt.mRunStateStr = RendSketchRunStateTbl[mRunState];
   }
   rpt.mStopUrgency = mStopUrgency;

   rpt.mRunSecs = runTimerGetSecs(now);
   rpt.mActualWorkRate = mActualWorkRate;
   rpt.mLoopRate = mActualLoopRate;

   rpt.mStallWhy = mStallWhy;

   // static state
   rpt.mAcctBase = mTu.getAcctMgr().getAcctBase();

   rpt.mTransTxReqCnt = mTu.mStats.mTransTxReqCnt;
   rpt.mTransTxRspCnt = mTu.mStats.mTransTxRspCnt;
   rpt.mTransRxReqCnt = mTu.mStats.mTransRxReqCnt;
   rpt.mTransRxRspCnt = mTu.mStats.mTransRxRspCnt;

   rpt.mLastTransRate = mLastTransRate;

   return 1;
}

/**
    This is internal helper function only; it has no mutex protection
    and must not be called directly from application.
**/
void
RendSketchBase::validate(RendTimeUs now) 
{
   unsigned idx;
   for (idx=0; idx < mTroopList.size(); idx++) 
   {
      mTroopList[idx]->validate(now);
   }
}


/**
    This is internal helper function only; it has no mutex protection
    and must not be called directly from application.
**/
void
RendSketchBase::setWorkLevel(int level) 
{
   // XXX: should be more clever about this, and apporptions the work out
   unsigned idx;
   for (idx=0; idx < mTroopList.size(); idx++) 
   {
      mTroopList[idx]->setTgtOpenDlgs(level);
   }
}

void
RendSketchBase::setWorkVolume(RendWorkVolume& vol) 
{
   InfoLog(<<"Set workLevel="<<vol.mWorkLevel
      <<" minrate="<<vol.mWorkRateMin<<" maxrate="<<vol.mWorkRateMax
      <<" pendmax="<<vol.mWorkPendCntMax);
   resip::Lock lock(mUpperMutex);
   mWorkVol = vol;
   setWorkLevel(vol.mWorkLevel);
}

int
RendSketchBase::doTroopWork(RendTimeUs now, int minWork, int maxWork) 
{
   int totWork = 0;
   int numWork;
   unsigned idx;
   for (idx=0; idx < mTroopList.size() && maxWork > 0; idx++) 
   {
      if ( mStopUrgency>=2 ) 
      {
         // don't wait for NOTIFY: put on correct (Idle/Open) list
         // This doesn't do any actual work
         mTroopList[idx]->changeDlgCnt(now, 1000000, REND_SM_PendNotify, REND_SM_GROUP_ByDlgState);
      }
      numWork = mTroopList[idx]->doWork(now, minWork, maxWork);
      totWork += numWork;
      minWork -= numWork;
      maxWork -= numWork;
   }
   return totWork;
}

int
RendSketchBase::getSessionMoodCnt(RendSessionMood mood) const 
{
   int cnt = 0;
   unsigned idx;
   for (idx=0; idx < mTroopList.size(); idx++) 
   {
      cnt += mTroopList[idx]->getSessionMoodCnt(mood);
   }
   return cnt;
}

void
RendSketchBase::getPendCnts(int& pendReqCnt, int& pendNotifyCnt) const 
{
   pendReqCnt = getSessionMoodCnt(REND_SM_PendReq);
   pendNotifyCnt = getSessionMoodCnt(REND_SM_PendNotify);
}

void
RendSketchBase::deleteTroops() 
{
   unsigned idx;
   for (idx=0; idx < mTroopList.size(); idx++) 
   {
      RendTroopBase *tr = mTroopList[idx];
      if ( tr ) 
      {
         delete tr;
         mTroopList[idx] = 0;
      }
   }
   mTroopList.clear();
}

int
RendSketchBase::doWorkChunk(RendTimeUs now, int minWork, int maxWork) 
{
   return doTroopWork(now, minWork, maxWork);
}

void
RendSketchBase::printStatus(const RendTroopReport& rpt) 
{
   printf("tick rs=%d idle=%d open=%d pend=%d\n",
      mRunState,
      rpt.mMoodIdleCnt, 
      rpt.mMoodOpenCnt,
      rpt.mMoodPendReqCnt);
}

int
RendSketchBase::doWorkLoop(RendTimeUs reportPeriod)
{
   RendTimeUs now = RendGetTimeUsRel();

   resip_assert(mRunState == REND_SKETCH_RS_INIT);
   mRunState = REND_SKETCH_RS_RUNNING;
   runTimerReset();
   runTimerGo(now);
   InfoLog(<<"Starting work loop rptPer="<<REND_US2S(reportPeriod));
   RendTimeUs lastWorkTime = now;
   RendTimeUs lastUpdateTime = 0;
   RendTimeUs lastValidateTime = 0;
   RendTimeUs validatePeriod = REND_S2US(10);
   double minWorkCarry = 0.0;
   double maxWorkCarry = 0.0;
   int workDoneSum = 0;
   int workDoneCnt = 0;
   RendTimeUs workDoneStart = now;
   RendTimeUs workDonePeriod = REND_S2US(1); // accum over 1 sec

   RendTimeUs lastStaleTime = 0;
   RendTimeUs stalePeriod = REND_S2US(5);

   while ( mStopUrgency<2 ) 
   {
      int stepSizeMs = REND_US2MS(mTickPeriod);
      mTu.processAll(stepSizeMs);

      resip::Lock lock(mUpperMutex);
      now = RendGetTimeUsRel();

      RendTimeUs staleDelta = now - lastStaleTime;
      if ( staleDelta > stalePeriod )
      {
         lastStaleTime = now;
         checkStale(now);
         // piggy back on StaleTime for slow-ish transaction rate
         int newCnt = mTu.mStats.mTransRxReqCnt + mTu.mStats.mTransTxReqCnt;
         mLastTransRate = 1e6 * (newCnt - mLastTransCnt) / (double)staleDelta;
         mLastTransCnt = newCnt;
      }

      int curPendReq = 0;
      int curPendNot = 0;
      getPendCnts(curPendReq, curPendNot);

      if ( mRunState==REND_SKETCH_RS_RUNNING && mStopUrgency>=1 )
      {
         setWorkLevel(0);
         mRunState = REND_SKETCH_RS_STOPPING;
         WarningLog(<<"Got stop signal urgency="<<mStopUrgency);
         // fprintf(stderr, "\n\nStopping sketch due to signal.\n");
      }

      if ( mRunState==REND_SKETCH_RS_STOPPING ) 
      {
         int curOpen = getSessionMoodCnt(REND_SM_Open);
         int curWave = getSessionMoodCnt(REND_SM_Wave);
         if ( curPendReq==0 && curPendNot==0 && curOpen==0 && curWave==0 ) 
         {
            break;
         }
      }

      if ( reportPeriod>0 && now > lastUpdateTime+reportPeriod ) 
      {
         RendTroopReport rpt;
         getTroopReportCore(now, rpt);
         printStatus(rpt);
         lastUpdateTime = now;
      }

      resip_assert( lastWorkTime <= now );
      double deltaSecs = (now - lastWorkTime) * 1e-6;
      double minWorkFlt = mWorkVol.mWorkRateMin * deltaSecs + minWorkCarry;
      double maxWorkFlt = mWorkVol.mWorkRateMax * deltaSecs + maxWorkCarry;

      int minWork = (int)minWorkFlt;
      int maxWork = (int)maxWorkFlt;
      minWorkCarry = minWorkFlt - minWork;
      maxWorkCarry = maxWorkFlt - maxWork;

      int workDone = 0;
      lastWorkTime = now;

      // TBD: add stopUrgency level to defeat pending check below
      int pendAllow = mWorkVol.mWorkPendCntMax-curPendReq-curPendNot;
      if ( maxWork<=0 ) 
      {
         mStallWhy = "rate";
      } 
      else if ( pendAllow <= 0 ) 
      {
         mStallWhy = curPendReq>=mWorkVol.mWorkPendCntMax ? "pendrc" : "pendnc";
      } 
      else 
      {
         mStallWhy = "none";
         workDone = doWorkChunk(now, minWork, rendMIN(maxWork,pendAllow));
      }

      workDoneCnt++;
      workDoneSum += workDone;
      RendTimeUs workDoneElapsed = now - workDoneStart;
      if ( workDoneElapsed > workDonePeriod ) 
      {
         double workDoneLastAvg = 1e6 * workDoneSum / ((double)workDoneElapsed);
         mActualWorkRate = workDoneLastAvg;
         mActualLoopRate = 1e6*workDoneCnt / ((double)workDoneElapsed);
#if 0
         InfoLog(<<"workdone: elapsed="<<workDoneElapsed<<" sum="<<workDoneSum<<" avg="<<workDoneLastAvg);
#endif
         workDoneSum = 0;
         workDoneCnt = 0;
         workDoneStart = now;
      }

      if ( now - lastValidateTime > validatePeriod ) 
      {
         validate(now);
         lastValidateTime = now;
      }
   }
   mRunState = REND_SKETCH_RS_FINISHED;
   now = RendGetTimeUsRel();
   runTimerPause(now);

   InfoLog(<<"Finished work loop");
   return 1;
}


/****************************************************************************
 *
 * RendSketchFactory
 *
 ***************************************************************************/

// XXX: move this to different file?
RendSketchFactory::~RendSketchFactory() 
{
}

/* ====================================================================

 Copyright (c) 2011, Logitech, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Logitech nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
