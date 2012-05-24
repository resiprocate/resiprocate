#if !defined(REND_SKETCH_HXX)
#define REND_SKETCH_HXX 1
/**
  * REND Sketch
  *
  * A sketch is a specific test pattern.
  *
  * Written by Kennard White (Logitech, Inc.), 2010..2011
**/

#include "RendDlg.hxx"
#include "RendDlgAcct.hxx"
#include "RendReport.hxx"

class RendOptsBase;
class RendSketchBase;

class RendSketchFactory 
{
public:
   RendSketchFactory() { };
   ~RendSketchFactory();
   virtual const char* getName() const  = 0;
   virtual RendOptsBase* getOpts() = 0;
   virtual RendSketchBase* makeSketch(RendTu& tu, RendCntMgr& cntMgr) = 0;
};


/*
 * Keep below in-sync with RendSketchRunStateTbl[]
 */
#define REND_SKETCH_RS_NONE      0
#define REND_SKETCH_RS_INIT      1
#define REND_SKETCH_RS_RUNNING   2
#define REND_SKETCH_RS_STOPPING  3
#define REND_SKETCH_RS_FINISHED  4

class RendTroopBase;

typedef std::vector<RendTroopBase*> RendTroopVec;

class RendSketchBase 
{
public:
   RendSketchBase(RendTu& tu, RendCntMgr& cntMgr) 
      : mTu(tu), mCntMgr(cntMgr), mStopUrgency(0), mRunState(0) 
   {
      runTimerReset();
      mRunState = REND_SKETCH_RS_INIT;
      mWorkPendAgeMax = 0;
      mStallWhy = 0;
      mFailAge = 45;
      mLastTransCnt = 0;
      mLastTransRate = 0;
      mTickPeriod = REND_MS2US(100);
   }

   virtual ~RendSketchBase();

   virtual int doWorkLoop(RendTimeUs reportPeriod);
   virtual void signalStop(int urgency) 
   {
      mStopUrgency = urgency;
   }
   virtual bool isFinished() 
   { 
      return mRunState == REND_SKETCH_RS_FINISHED;
   }
   virtual int getTroopReport(RendTroopReport& rpt);
   virtual void setWorkVolume(RendWorkVolume& vol);
   void setTickPeriod(RendTimeUs tickUs) 
   {
      mTickPeriod = tickUs;
   }

   virtual int doSimpleAction(RendSimpleAction act);

   // XXX: make accessors can be set from Factory
   // int mWorkPendCntMax;
   int mWorkPendAgeMax;
   int mFailAge;

protected:

   RendTu& mTu;
   RendCntMgr& mCntMgr;
   int mStopUrgency;
   int mRunState;
   resip::Mutex mUpperMutex;

   virtual void setWorkLevel(int level);
   virtual void getTroopReportCore(RendTimeUs now, RendTroopReport& rpt);
   int getSessionMoodCnt(RendSessionMood mood) const;
   virtual void getPendCnts(int& pendReqCnt, int& pendNotifyCnt) const;

   void runTimerReset() 
   { 
      mRunPriorDuration = 0;
      mRunCurStart = 0;
   }
   void runTimerGo(RendTimeUs now);
   void runTimerPause(RendTimeUs now);
   unsigned runTimerGetSecs(RendTimeUs now);
   RendTimeUs mRunPriorDuration;
   RendTimeUs mRunCurStart;

   virtual void printStatus(const RendTroopReport& rpt);
   int doTroopWork(RendTimeUs now, int minWork, int maxWork);
   virtual int doWorkChunk(RendTimeUs now, int minWork, int maxWork);
   virtual void validate(RendTimeUs now);
   virtual void checkStale(RendTimeUs now) { };

   /*
   * Basic polling period of the sketch
   */
   RendTimeUs mTickPeriod;

   /*
   * Values are set by upper layer, and used in work loop
   */
   RendWorkVolume mWorkVol;

   /*
   * Values below are updated by the work loop, and are reported
   * to upper layers.
   */
   float mActualWorkRate;
   float mActualLoopRate;
   const char* mStallWhy;

   int mLastTransCnt;
   float mLastTransRate;

   void deleteTroops();
   RendTroopVec mTroopList;
};

extern RendSketchFactory* TheRendReg1SketchFacPtr;
extern RendSketchFactory* TheRendPres1SketchFacPtr;

#endif // end-of-header

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
