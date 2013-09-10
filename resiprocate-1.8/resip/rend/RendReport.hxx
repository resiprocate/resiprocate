#if !defined(REND_REPORT_HXX)
#define REND_REPORT_HXX 1
/**
  * REND Report
  *
  * Data-types used to report status
  *
  *  This header is processed by SWIG to create python interface,
  *  so keep it simple. Avoid function overloading.
  *
  * Written by Kennard White (Logitech, Inc.) over 2010..2011
**/

enum RendSimpleAction 
{
   REND_SA_None,
   REND_SA_Ping,
   REND_SA_SkipPendNotify,
   REND_SA_Exit,
};

enum RendSessionMood
{
   REND_SM_None,
   REND_SM_Idle,
   REND_SM_Open,
   REND_SM_PendReq,     // opening or closing
   REND_SM_PendNotify,  // Got response, wait for NOTIFY
   REND_SM_Recycle,     // closed, move to idle when possible
   REND_SM_Wave,
   REND_SM_MAX
};

#define REND_SessionMoodNameInitializer "none", "idle", "open", "preq", "pnot", "recy", "wave"
extern const char* RendSessionMoodFmt(RendSessionMood mood);

#define REND_SM_GROUP_Pend   ((RendSessionMood)(REND_SM_MAX+1))
#define REND_SM_GROUP_Alive  ((RendSessionMood)(REND_SM_MAX+2))

#define REND_SM_GROUP_ByPendReason  ((RendSessionMood)(REND_SM_MAX+3))
#define REND_SM_GROUP_ByDlgState    ((RendSessionMood)(REND_SM_MAX+4))

class RendWorkVolume 
{
public:
   RendWorkVolume() : mWorkLevel(0), mWorkRateMin(0), 
                      mWorkRateMax(0), mWorkPendCntMax(0) { }
   RendWorkVolume(int lvl, float minr, float maxr, int pendCnt) 
      : mWorkLevel(lvl), mWorkRateMin(minr), mWorkRateMax(maxr),
        mWorkPendCntMax(pendCnt) { }

   int   mWorkLevel;
   float mWorkRateMin;
   float mWorkRateMax;
   int   mWorkPendCntMax;
};


class RendTroopReport 
{
public:
   RendTroopReport() { reset(); }

   void reset() 
   {
      // memset(mMoodCnts, 0, sizeof(mMoodCnts));
      mRunState = 0;
      mRunStateStr = 0;
      mStopUrgency = 0;

      mMoodIdleCnt = 0;
      mMoodOpenCnt = 0;
      mMoodPendReqCnt = 0;
      mMoodPendNotifyCnt = 0;
      mMoodWaveCnt = 0;

      mOpenGoodCnt = 0;
      mRenewGoodCnt = 0;
      mCloseGoodCnt = 0;
      mOpenFailCnt = 0;
      mRenewFailCnt = 0;
      mCloseFailCnt = 0;

      mMaxSessions = 0;
      mActualWorkRate = 0;
      mLoopRate = 0;

      mRunSecs = 0;

      mStallWhy = 0;
      mWaveStateStr = 0;

      mWaveRegAliveCnt = 0;
      mWavePubAliveCnt = 0;
      mWaveSubAliveCnt = 0;
      mWaveCnt = 0;
      mWaveState = 0;
      mWaveStateSecs = 0;
      mWaveWorkLen = 0;
      mWaveWorkRemain = 0;

      mWaveLastPubDur = 0;
      mWaveLastSubDur = 0;

      mWavePubReqAvgDur = 0;
      mWavePubReqMaxDur = 0;
      mWavePubNotAvgDur = 0;
      mWavePubNotMaxDur = 0;

      mWaveSubReqAvgDur = 0;
      mWaveSubReqMaxDur = 0;
      mWaveSubNotAvgDur = 0;
      mWaveSubNotMaxDur = 0;

      mWaveCurReqTotCnt = 0;
      mWaveCurReqRemCnt = 0;
      mWaveCurTupTotCnt = 0;
      mWaveCurTupRemCnt = 0;

      mWaveCurTupPendCnt = 0;


      mAcctBase = 0;
   }

   void add(const RendTroopReport& y) 
   {
      mMoodIdleCnt += y.mMoodIdleCnt;
      mMoodOpenCnt += y.mMoodOpenCnt;
      mMoodPendReqCnt += y.mMoodPendReqCnt;
      mMoodPendNotifyCnt += y.mMoodPendNotifyCnt;
      mMoodWaveCnt += y.mMoodWaveCnt;

      mOpenGoodCnt += y.mOpenGoodCnt;
      mRenewGoodCnt += y.mRenewGoodCnt;
      mCloseGoodCnt += y.mCloseGoodCnt;
      mOpenFailCnt += y.mOpenFailCnt;
      mRenewFailCnt += y.mRenewFailCnt;
      mCloseFailCnt += y.mCloseFailCnt;

      mMaxSessions += y.mMaxSessions;
      mActualWorkRate += y.mActualWorkRate;
   };

   // This is static configuration information, doesn't change during run
   int mMaxSessions;

   // This comes from Tu
   int mTransTxReqCnt;
   int mTransTxRspCnt;
   int mTransRxReqCnt;
   int mTransRxRspCnt;

   // This accessor is here because I cannot get SWIG to make the
   // raw array readable in python
   float mActualWorkRate;
   float mLoopRate;
   // int getMoodCnt(RendSessionMood mood) { return mMoodCnts[mood]; }

   // int mMoodCnts[REND_SM_MAX];
   int mMoodIdleCnt;
   int mMoodOpenCnt;
   int mMoodPendReqCnt;
   int mMoodPendNotifyCnt;
   int mMoodWaveCnt;

   int mOpenGoodCnt;
   int mRenewGoodCnt;
   int mCloseGoodCnt;
   int mOpenFailCnt;
   int mRenewFailCnt;
   int mCloseFailCnt;

   // below here is actually sketch state
   int mRunState;
   const char* mRunStateStr;
   int mStopUrgency;
   long mRunSecs;
   const char* mStallWhy;

   // below here is wave state, and sub-case of sketch state
   int mWaveRegAliveCnt;
   int mWavePubAliveCnt;
   //int mWavePubDoneCnt;
   int mWaveSubAliveCnt;
   //int mWaveNotifyDoneCnt;
   int mWaveCnt;	// count of waves done
   int mWaveState;	// aka mWorkState
   const char* mWaveStateStr;
   int mWaveStateSecs;	// how long in this state
   // waveWork could be viewed as just PUBs or PUBS+ implied NOTIFY
   int mWaveWorkLen;	// how much total work in this state
   int mWaveWorkRemain;// how much of that total remains

   // Duration of the whole wave, in sec
   int mWaveLastPubDur;
   int mWaveLastSubDur;

   // below are in MS
   int mWavePubReqAvgDur;
   int mWavePubReqMaxDur;
   int mWavePubNotAvgDur;
   int mWavePubNotMaxDur;

   // below are in MS
   int mWaveSubReqAvgDur;
   int mWaveSubReqMaxDur;
   int mWaveSubNotAvgDur;
   int mWaveSubNotMaxDur;

   int mWaveCurReqTotCnt;
   int mWaveCurReqRemCnt;
   int mWaveCurTupTotCnt;
   int mWaveCurTupRemCnt;
   int mWaveCurTupPendCnt;

   float mLastTransRate;	// trans/sec

   // below here is really static info that could be split off
   int mAcctBase;
};

#endif // header

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
