#if !defined(REND_TROOP_HXX)
#define REND_TROOP_HXX 1
/**
 * REND Troop
 *
 * Written by Kennard White (Logitech, Inc.)
**/

#include "boost/intrusive/list.hpp"
#include "boost/function.hpp"

// #include "RendAcct.hxx"
#include "RendReport.hxx"	// for enum RendSessionMood
#include "RendDlg.hxx"
#include "RendDlgAcct.hxx"	// for enum RendDlgCat

//#include <map>

//#include "rutil/SharedPtr.hxx"
//#include "resip/stack/SipStack.hxx"
//#include "resip/stack/TransactionUser.hxx"
//
//#include "RendAcct.hxx"

/**
    The dialog range is [0,mDlgLen)
    The account range is [mAcctBase,mAcctBase+mAcctLen)
    And this maps the dialog range on to the account range via:
    	acctIdx = ((dlgIdx%mDlgLen)*mStride)%mAcctLen + mAcctBase
**/
class RendAcctSlice
{
public:
   RendAcctSlice() 
   {
      clearMap();
   }

   void clearMap() 
   {
      mDlgLen = 0;
      mAcctBase = -1;
      mAcctLen = 1;
      mStride = 1;
   }

   void setMap(int dlgLen, int acctBase, int acctLen, int stride = 1) 
   {
         mDlgLen = dlgLen;
         mAcctBase = acctBase;
         mAcctLen = acctLen<=1?1:acctLen;
         mStride = stride;
   }

   int getDlgLen() const { return mDlgLen<=1 ? 1 : mDlgLen; }

   bool checkIdx(RendAcctIdx acctIdx) 
   {
      return mDlgLen < 1 ? true 
         : (acctIdx >= mAcctBase && acctIdx < mAcctBase+mAcctLen);
   }

   RendAcctIdx mapSimpleIdx(int x) const 
   { 
      return ((x*mStride)%mAcctLen) + mAcctBase; 
   }

   /**
   Lookup the account index based upon dialog index. The {idx}
   argument is modified for the next dimension of the lookup.
   **/
   RendAcctIdx mapDlgIdx(int& idx) 
   {
      if ( mDlgLen <= 1 )
         return mAcctBase;
      int x = idx % mDlgLen;
      idx /= mDlgLen;
      return mapSimpleIdx(x);
   }

   void foldIdx(int& idx, RendAcctIdx acctIdx) 
   {
      // XXX: add stride and AcctLen logic, currently missing!
      idx = mDlgLen<=1 ? idx : 
         (idx * mDlgLen + ((acctIdx-mAcctBase) % mDlgLen));
   }

   int mDlgLen;
   int mAcctBase;
   int mStride;
   int mAcctLen;	// what space does this live in?
};


/**
Within a troup, all possible dialogs are given a serial index. This
is just an int, but we use this typedef for code clarity.
**/
typedef int RendSessionIdx;

typedef boost::intrusive::list_member_hook<
boost::intrusive::link_mode<boost::intrusive::safe_link> > RendCommonHook;

typedef boost::intrusive::list_member_hook<
boost::intrusive::link_mode<boost::intrusive::auto_unlink> > RendExpireHook;

typedef int RendPendReason;
#define REND_PR_None         0
#define REND_PR_Open         1
#define REND_PR_Modify       2
#define REND_PR_OpenOrModify 3
#define REND_PR_Renew        4
#define REND_PR_Close        5

// below are legal values for PendReason, but are really RecycleReasons
#define REND_PR_ConnTerm     11 // connection terminated
#define REND_PR_SessionError 12 // connection terminated
#define REND_PR_BadRsp       13 // explicit SIP error response
#define REND_PR_BadRspOpen   14 // explicit SIP error response
#define REND_PR_BadRspRenew  15 // explicit SIP error response
#define REND_PR_BadRspClose  16 // explicit SIP error response
#define REND_PR_CheckAge     17 // too old
#define REND_PR_NotifyTermClose 18 // NOTIFY w/terminated state
#define REND_PR_NotifyTermSuprise 19 // NOTIFY w/terminated state

class RendTroopDlg;

/**
Tracks state about a particular dialog belonging to a troup.
XXX: Why don't we just inherit from RendDlg?
**/
class RendSession 
{
public:
   RendSession() : mMood(REND_SM_None) {}

   int getRenewSecs(RendTimeUs now) const 
   {
      // secs until renewal is scheduled
      return mRenewAbsTime==0?-123456:REND_US2S(mRenewAbsTime-now);
   }

   int getExpireSecs(RendTimeUs now) const 
   {
      // secs until it expires
      return mExpireAbsTime==0?-123456:REND_US2S(mExpireAbsTime-now);
   }

   int getLastMoodSecs(RendTimeUs now) const 
   { 
      // secs since last mood change
      return REND_US2S(now-mLastMoodTime);
   }

   RendPendReason	getPendReason() const { return mPendReason; }

   bool isOpenish() const 
   {
      return mMood==REND_SM_Open || mMood==REND_SM_PendReq
         || mMood==REND_SM_PendNotify;
   }

   bool isPendish() const 
   {
      return mMood==REND_SM_PendReq || mMood==REND_SM_PendNotify;
   }

   RendDlgSharedPtr mDlgPtr;
   RendSessionIdx mSessionIdx;
   RendSessionMood mMood;
   RendCommonHook mMoodLink;
   RendPendReason mPendReason;
   RendTimeUs mLastMoodTime;

   RendTimeUs mExpireAbsTime;	// do we really need this?
   RendTimeUs mRenewAbsTime;
   RendExpireHook mExpireLink;

   RendTroopDlg* getDlg() const { return (RendTroopDlg*)mDlgPtr.get(); }

   std::ostream& fmt(RendTimeUs now, std::ostream& os) const;
};

class RendSessionFmtr 
{
public:
   RendSessionFmtr(RendTimeUs now, const RendSession& sess) :
      mNow(now), mSess(sess) {}

   RendTimeUs mNow;
   const RendSession& mSess;
};

inline std::ostream&
operator<<(std::ostream& os, const RendSessionFmtr& fmtr)
{
   return fmtr.mSess.fmt(fmtr.mNow, os);
}

typedef std::vector<RendSession> RendSessionVec;

typedef boost::intrusive::list<RendSession, 
                               boost::intrusive::member_hook<RendSession,
                                                             RendCommonHook,
                                                             &RendSession::mMoodLink>,
                               boost::intrusive::constant_time_size<true>
                              > RendSessionMoodChain;

class RendExpireTracker;
class RendTroopBase;

//    virtual RendSessionMood	
// checkGoalMood(RendTimeUs now, RendSession& sess, 
//  const RendDlgAcctKey& key, RendPendReason& pr);

struct RendHandleMoodCxt 
{ 
   RendHandleMoodCxt(RendTimeUs now, RendDlgCat cat, RendTroopBase* tr) 
      : mNow(now), mCat(cat), mTroop(tr), mSess(0), mAge(0) { }

   RendTimeUs mNow;
   RendDlgCat mCat;
   RendTroopBase* mTroop;
   RendSession* mSess;
   int mAge;
   RendDlgAcctKey mKey;
   RendPendReason mNewPendReason;
};

struct RendMoodTracker
{
   RendSessionMoodChain mChain;
   unsigned mMostCnt;
   RendStatAcc mDurStat;

   void clearStats() 
   {
      mMostCnt = 0; 
      mDurStat.clear();
   }
};

/**
A troop is a collection of dialogs. All dialogs are of the same
category, and spread accross Accts.
**/
class RendTroopBase 
{
public:
   RendTroopBase(RendTimeUs now, RendTu& tu, RendDlgCat cat, RendCntMgr& cntMgr);
   virtual ~RendTroopBase();

   void validate(RendTimeUs now);

   RendDlgCat getDlgCat() const { return mCat; }

   int doWork(RendTimeUs now, int minWork, int maxWork);
   int changeDlgCnt(RendTimeUs now, int numDlgs, 
                    RendSessionMood fromMood, RendSessionMood toMood);
   int renewDlgCnt(RendTimeUs now, int numDlgs);

   void setVectorDlgs(int numDlgs, 
                      int acctBase, int acctLen, int acctStride,
                      int repeatBase, int repeatLen);
   void setMatrixDlgs(int numFrom, int numTo,
                      int repeatBase, int repeatLen);

   unsigned getToAcctBase() const { return mToSlice.mAcctBase; }
   unsigned getNumToAccts() const { return mToSlice.mAcctLen; }
   unsigned getFromAcctBase() const { return mFromSlice.mAcctBase; }
   unsigned getNumFromAccts() const { return mFromSlice.mAcctLen; }
   unsigned getRepeatBase() const { return mRepeatSlice.mAcctBase; }
   unsigned getNumRepeats() const { return mRepeatSlice.mAcctLen; }
   unsigned getRepeatMax() const { return mRepeatSlice.mAcctBase + mRepeatSlice.mAcctLen; }

   void setTgtOpenDlgs(int dlgs) 
   {
      mTgtOpenDlgs = dlgs;
   }

   void setOpenToIdxRange(int base, int len) 
   {
      mOpenToIdxBase = base;
      mOpenToIdxLen = len;
   }

   virtual int doSimpleAction(RendTimeUs now, RendSimpleAction act);

   virtual int startDlgCmd(RendTimeUs now, RendSession& sess,
                           RendTroopDlg *dlg, int expSecs, 
                           RendPendReason pendReason);

   virtual int startDlgOpen(RendTimeUs now, RendSession& sess, 
                            RendPendReason pr);

   virtual int startDlgClose(RendTimeUs now, RendSession& sess);
   // virtual int startRenewDlg(RendSessionIdx idx) = 0;
   // virtual int startCloseDlg(RendSessionIdx idx) = 0;

   virtual RendTroopDlg* createNewDlg(const RendDlgAcctKey& key) = 0;

   virtual int checkPendNotify(RendTimeUs now, bool skipNow=false) 
   {
      return -10;
   }

   typedef boost::function<RendSessionMood (RendHandleMoodCxt&)> RendHandleMoodFnc;
   virtual int checkAge(RendTimeUs now, RendHandleMoodFnc handler,
                        RendSessionMood mood,
                        int failAge, bool failAll,
                        int &maxAgeRet);

   int adjustAllDlgs(RendTimeUs now, RendHandleMoodFnc handler, int maxWork);

   RendDlgAcctKey cvtIdxToKey(RendSessionIdx idx);
   RendSessionIdx cvtKeyToIdx(const RendDlgAcctKey& key);

   RendSession* getSession(RendSessionIdx idx);
   RendSession* getSession(const RendDlgAcctKey& key) 
   {
      return getSession(cvtKeyToIdx(key));
   }

   void setSessionMood(RendTimeUs now, 
                       RendSession& sess, RendSessionMood mood,
                       RendPendReason pendReason=0);

   void addTroopReport(RendTroopReport& rpt);

   void setExpires(int expireSecs, int scatterLoPct=80, int scatterHiPct=100) 
   {
      mExpireSecs = expireSecs;
      mExpireScatterLoPct = scatterLoPct;
      mExpireScatterRngPct = scatterHiPct-scatterLoPct;
   }

   void scheduleRenew(RendTimeUs now, RendSession& sess, int expireSecs);

   int getSessionMoodCnt(RendSessionMood mood) const 
   {
      if (mood==REND_SM_GROUP_Pend) 
      {
         return mMoods[REND_SM_PendReq].mChain.size()
            + mMoods[REND_SM_PendNotify].mChain.size();
      } 
      else if (mood==REND_SM_GROUP_Alive) 
      {
         return mMoods[REND_SM_Open].mChain.size()
            + mMoods[REND_SM_PendReq].mChain.size()
            + mMoods[REND_SM_PendNotify].mChain.size()
            + mMoods[REND_SM_Wave].mChain.size();
      }
      resip_assert( mood > REND_SM_None && mood < REND_SM_MAX );
      return mMoods[mood].mChain.size();
   }

   unsigned getMostMoodCnt(RendSessionMood mood) const 
   {
      return mMoods[mood].mMostCnt;
   }

   const RendStatAcc& getMoodDur(RendSessionMood mood) const 
   {
      return mMoods[mood].mDurStat;
   }
   
   void addMoodDur(RendSessionMood mood, RendTimeUs dur) 
   {
      mMoods[mood].mDurStat.add(dur);
   }

   // local accumulator, access from TroopDlgs
   RendCntMgr& mCntMgr;
   RendTroopReport mRpt;

   // accumulators for request durations. Not all moods are used

   void spliceMoodList(RendTimeUs now, RendSessionMood fromMood,
                       RendSessionMood toMood, RendPendReason pendReason=0);

   void updateWorkStats(bool clearB = false);

protected:
   resip::Data mDesc;
   RendTroopDlg* getDlg(RendSession& sess) 
   {
      return (RendTroopDlg*)sess.mDlgPtr.get(); // may be NULL
   }

   int startMoodChange(RendTimeUs now, RendSession& sess,RendSessionMood toMood);
   void prepareDlgs();
   void releaseDlgs();
   void bindDlg(RendSession& sess, RendTroopDlg *dlg);
   int getScatterExpires();

   void sessionError(RendTimeUs now, RendSession& sess, 
      const char *msg);

   int mAdjustIdx;

   // virtual RendSessionMood	
   // checkGoalMood(RendTimeUs now, RendSession& sess, 
   //  const RendDlgAcctKey& key, RendPendReason& pr);

   RendTu& mTu;
   const RendDlgCat mCat;
   RendAcctSlice mToSlice;
   RendAcctSlice mFromSlice;
   RendAcctSlice mRepeatSlice;

   // The maximum number of dialogs we can have (open or closed)
   RendSessionIdx mMaxSessions;

   /**
   The Expires time to request when opening or renewing a dialog,
   and associated pararmeters for randomly scattering it
   **/
   int mExpireSecs;
   int mExpireScatterLoPct;
   int mExpireScatterRngPct;

   int mCurOpenDlgs;

   // Set of all possible dialogs we can have. Any given dialog
   // may or may not be open or exist.
   RendSessionVec mDlgVec;

   RendMoodTracker mMoods[REND_SM_MAX];
   // RendSessionMoodChain mMoodChains[REND_SM_MAX];
   // RendStatAcc mMoodDurStat[REND_SM_MAX];

   RendExpireTracker* mTracker;

   /* Params for helping to manage which dialogs are open.
   */
   int mTgtOpenDlgs;
   int mOpenToIdxBase;
   int mOpenToIdxLen;
};

class RendTroopDlg : public RendDlg 
{
public:
   RendTroopDlg(RendTu& tu, int fromAcctIdx, int toAcctIdx=-1) 
      : RendDlg(tu, fromAcctIdx, toAcctIdx) { }

   virtual void handleResponse(RendTimeUs now, 
                               const resip::SipMessage *msg,
                               const char *failWhy);

   virtual void handleConnTerm(RendTimeUs now);

   virtual RendTroopBase& getTroop() = 0;

   // Default version extracts from h_Expires
   virtual int getRspExpire(const resip::SipMessage& rsp);

   virtual RendSessionMood getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood);

   void handleSessResponse(RendTimeUs now, RendSession& sess,
                           const resip::SipMessage *msg, const char *failWhy);

   RendLocalKey mTroopSessionIdx;
};

#endif	// end-of-header

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
