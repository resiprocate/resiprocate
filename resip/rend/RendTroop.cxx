/**
    REND Troop

    Written by Kennard White (Logitech, Inc.), 2010..2011
**/


#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"

// #include "RendDlg.hxx"
// #include "RendDlgAcct.hxx"
#include "RendTroop.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP


/***********************************************************************
 *
 * Mood impl & RendSession
 *
 ***********************************************************************/

static const char *RendSessionMoodNameTbl[] = { REND_SessionMoodNameInitializer };

const char*
RendSessionMoodFmt(RendSessionMood mood) 
{
    return ((unsigned)mood) < REND_NELTS(RendSessionMoodNameTbl) ? RendSessionMoodNameTbl[mood] : "????";
}

std::ostream&
RendSession::fmt(RendTimeUs now, std::ostream& os) const {
    os<<"["<<RendSessionMoodFmt(mMood)
      <<" pr="<<getPendReason()
      <<" idx="<<mSessionIdx
      <<" E="<<(getExpireSecs(now))
      <<" re="<<(getRenewSecs(now))
      <<" last="<<(getLastMoodSecs(now))
      <<"]";
    return os;
}


/***********************************************************************
 *
 * ExpireTracker
 *
 ***********************************************************************/

typedef boost::intrusive::list<RendSession, 
                               boost::intrusive::member_hook<RendSession, 
                                                             RendExpireHook,
                                                             &RendSession::mExpireLink>,
                               boost::intrusive::constant_time_size<false> 
                              > RendDlgExpireChain;


class RendExpireBucket 
{
public:
   RendExpireBucket() { }
   ~RendExpireBucket() 
   { 
      // below should happen automatically, but make more explicit!
      // purpose is to unlink all the children so they get NULL pointers
      mChain.clear(); 
   }

   RendDlgExpireChain	mChain;
};

/**
    Roughly, 
      now ~= mHeadIdx*mSecsPerBucket
      future > (mHeadIdx+mNumBuckets)*mSecsPerBuckets
      far-future > (mHeadIdx+2*mNumBuckets)*mSecsPerBuckets
**/
class RendExpireTracker 
{
  public:
    RendExpireTracker(RendTimeUs now, resip::Data desc, unsigned secsPerBucket, unsigned numBuckets);
    ~RendExpireTracker();

    resip::Data mDesc;

    unsigned mSecsPerBucket;

    /* The number of buckets.
     * assert( mBuckets.size() == mNumBuckets )
     */
    unsigned mNumBuckets;

    /*
     * The (non-modulo) index corresponding to latest idea of "now"
     * In generally, mHeadIdx is much graater than mNumBuckets.
     */
    unsigned mHeadIdx;

    /*
     * The start of epoch; all times are relative to this (and must be
     * after). Strictly, we don't need this, but it helps avoid
     * big numbers which makes debugging easier
     */
    RendTimeUs mEpoch;

    RendDlgExpireChain mPastChain;
    RendExpireBucket*  mBuckets;
    RendDlgExpireChain mFutureChain;

    void pushDlg(RendSession& sess, RendTimeUs when);
    RendSession* popDlg(RendTimeUs now);
};

RendExpireTracker::RendExpireTracker(RendTimeUs now, resip::Data desc,
                                     unsigned secsPerBucket, unsigned numBuckets) :
   mDesc(desc) 
{
   resip_assert( secsPerBucket >= 1 );
   resip_assert( numBuckets >= 1 );
   mSecsPerBucket = secsPerBucket;
   mNumBuckets = numBuckets;
   mEpoch = now;
   mHeadIdx = 0;
   mBuckets = new RendExpireBucket[numBuckets];
}

RendExpireTracker::~RendExpireTracker() 
{
   InfoLog(<<"Destructing Tracker "<<mDesc<<" ...");
   delete [] mBuckets; mBuckets = NULL;
   InfoLog(<<"Destructed Tracker "<<mDesc<<" .");
}

void
RendExpireTracker::pushDlg(RendSession& sess, RendTimeUs when_abs) 
{
   resip_assert( when_abs >= mEpoch );
   RendTimeUs when = when_abs - mEpoch;
   unsigned bIdx = REND_US2S(when) / mSecsPerBucket;
   if ( bIdx < mHeadIdx ) 
   {
      WarningLog(<<"Tracker "<<mDesc<<": Got time that is early.");
      bIdx = mHeadIdx;
   }
   if ( bIdx >= mHeadIdx+mNumBuckets ) 
   {
      WarningLog(<<"Tracker "<<mDesc<<": Got time that is too far in future"
         <<" per="<<mSecsPerBucket
         <<" num="<<mNumBuckets
         <<" head="<<mHeadIdx
         <<" when="<<bIdx);
   } 
   else 
   {
      unsigned bmIdx = bIdx % mNumBuckets;
      mBuckets[bmIdx].mChain.push_back(sess);
      // Above depends upon auto-unlinking from existing chain!
   }
}

RendSession*
RendExpireTracker::popDlg(RendTimeUs now_abs)
{
   resip_assert( now_abs >= mEpoch );
   RendTimeUs now = now_abs - mEpoch;
   unsigned nowIdx = REND_US2S(now) / mSecsPerBucket;
   if ( nowIdx < mHeadIdx ) 
   {
      ErrLog(<<"NOW jumped backwards. Ignoring.");
      return NULL;
   }
   if ( nowIdx > mHeadIdx+mNumBuckets ) 
   {
      ErrLog(<<"NOW jumped far forwards:"
         <<" now="<<now
         <<" nowIdx="<<nowIdx
         <<" headIdx="<<mHeadIdx
         <<" numBuck="<<mNumBuckets
         <<". Ignoring.");
   }
   for ( ; mHeadIdx < nowIdx; mHeadIdx++) 
   {
      unsigned bmIdx = mHeadIdx % mNumBuckets;
      RendDlgExpireChain& chain = mBuckets[bmIdx].mChain;
      mPastChain.splice( mPastChain.end(), chain);
   }

   if ( !mPastChain.empty() ) 
   {
      RendSession& dlg = mPastChain.front();
      mPastChain.pop_front();
      return &dlg;
   }
   return NULL;
}


/***********************************************************************
 *
 * RendTroopBase
 *
 ***********************************************************************/

// Renew MUCH before expiration, because otherwise everything breaks 
#define REND_TROOP_RENEW_EARLY_SECS	(4*60)
#define REND_TROOP_RENEW_EARLY_FRAC	(0.75)
#define REND_TROOP_RENEW_MIN_SECS	(30)
#define REND_TROOP_EXPIRE_MIN		(60)

#define EXPIRE_SECS_PER_BUCKET 15
#define EXPIRE_MAX_SECS (60*60)

RendTroopBase::RendTroopBase(RendTimeUs now, RendTu& tu, RendDlgCat cat, RendCntMgr& cntMgr) 
  : mCntMgr(cntMgr), mTu(tu), mCat(cat) 
{
   mMaxSessions = 0;
   mTgtOpenDlgs = -1;
   mOpenToIdxBase = -1;
   mOpenToIdxLen = -1;
   setExpires(60*60);
   mDesc = RendDlgAcctKey::CatFmt(cat);
   mTracker = new RendExpireTracker(now, mDesc,
      EXPIRE_SECS_PER_BUCKET,
      EXPIRE_MAX_SECS/EXPIRE_SECS_PER_BUCKET);
}

RendTroopBase::~RendTroopBase()
{
   InfoLog(<<"Destructing Troop foo...");
   if ( mTracker ) 
   {
      delete mTracker; mTracker = NULL;
   }
   releaseDlgs();

   // below would happen automatically, but do it explicit now so
   // can control timing
   mDlgVec.clear();
   InfoLog(<<"Destructed Troop.");
}

void
RendTroopBase::releaseDlgs()
{
   unsigned didx;
   for (didx=0; didx < mDlgVec.size(); didx++) 
   {
      RendSession& sess = mDlgVec[didx];
      if ( sess.mMood!=REND_SM_None ) 
      {
         // below takes it out of linked lists
         setSessionMood(/*now*/0, sess, REND_SM_None);
         // maybe should call release on it first?
         RendDlg *dlg = sess.getDlg();
         if ( dlg ) 
         {
            dlg->relDlgState();
            sess.mDlgPtr.reset();
         }
      }
   }

   unsigned moodIdx=1;
   for ( ; moodIdx < REND_SM_MAX; moodIdx++) 
   {
      RendSessionMoodChain& chain = mMoods[moodIdx].mChain;
      resip_assert( chain.size()==0 );
   }
}

void
RendTroopBase::updateWorkStats(bool clearB) 
{
   int mood = REND_SM_None+1;
   for ( ; mood < REND_SM_MAX; mood++) 
   {
      if ( clearB ) 
      {
         mMoods[mood].clearStats();
      } 
      else 
      {
         unsigned cnt = getSessionMoodCnt((RendSessionMood)mood);
         if ( cnt > mMoods[mood].mMostCnt ) 
            mMoods[mood].mMostCnt = cnt;
      }
   }
}

void
RendTroopBase::addTroopReport(RendTroopReport& rpt) 
{
#if 0
   for (idx=0; idx < REND_SM_MAX; idx++) 
   {
      rpt.mMoodCnts[idx] = mMoodChains[idx].size();
   }
#else
   rpt.add(mRpt);
   rpt.mMoodIdleCnt += mMoods[REND_SM_Idle].mChain.size();
   rpt.mMoodOpenCnt += mMoods[REND_SM_Open].mChain.size();
   rpt.mMoodPendReqCnt += mMoods[REND_SM_PendReq].mChain.size();
   rpt.mMoodPendNotifyCnt += mMoods[REND_SM_PendNotify].mChain.size();
   rpt.mMoodWaveCnt += mMoods[REND_SM_Wave].mChain.size();
#endif
}

#if 0
#define TROOP_SCATTER_BASE (1<<16)
void
RendTroopBase::setRenewParams(float scatterLo, float scatterHi, unsigned minSecs) 
{
   mRenewScatterLoFix = scatterLo * TROOP_SCATTER_BASE;
   mRenewScatterRngFix = (scatterHi-scatterLo) * TROOP_SCATTER_BASE;
   mRenewMinSecs = minSecs;
}
#endif

#define RV_FRACBITS 12
#define RV_FRACVAL (1<<RV_FRACBITS)
int
RendTroopBase::getScatterExpires() 
{
   unsigned long rv = resip::Random::getRandom();
   long rvPct = (rv&(RV_FRACVAL-1)) * mExpireScatterRngPct  + (mExpireScatterLoPct<<RV_FRACBITS);
   int expSecs = ((rvPct * mExpireSecs)/100) >> RV_FRACBITS;
   return rendMAX(expSecs, REND_TROOP_EXPIRE_MIN);
}

void
RendTroopBase::scheduleRenew(RendTimeUs now, RendSession& sess, int expireSecs) 
{
   // WATCHOUT: unlink() requires link_mode=auto_unlink
   sess.mExpireLink.unlink();
   if ( expireSecs <= 0 ) 
   {
      sess.mExpireAbsTime = 0;
      sess.mRenewAbsTime = 0;
      return;
   }
#if 0
   unsigned long rv = resip::Random::getRandom();
   rv = (rv%TROOP_SCATTER_BASE)*mRenewScatterRngFix + mRenewScatterLoFix;
   rv = (rv*expireSecs)/TROOP_SCATTER_BASE;
   if ( rv < mRenewMinSecs )
      rv = mRenewMinSecs;
#endif
   sess.mExpireAbsTime = now + REND_S2US(expireSecs);
   int renewSecs = rendMAX(expireSecs - REND_TROOP_RENEW_EARLY_SECS,
      int(expireSecs * REND_TROOP_RENEW_EARLY_FRAC));
   renewSecs = rendMAX(renewSecs, REND_TROOP_RENEW_MIN_SECS);
   DebugLog(<<"schedRenew idx="<<sess.mSessionIdx<<" exp="<<expireSecs<<" renew="<<renewSecs);
   RendTimeUs renewWhen = now + REND_S2US(renewSecs);
   sess.mRenewAbsTime = renewWhen;
   mTracker->pushDlg(sess, renewWhen);
}

void
RendTroopBase::setSessionMood(RendTimeUs now, RendSession& sess, 
                              RendSessionMood mood,
                              RendPendReason pendReason) 
{
   if ( mood==sess.mMood )
      return;

   if ( mood==REND_SM_GROUP_ByPendReason ) 
   {
      if ( sess.mPendReason==REND_PR_None ) 
      {
         RendSessionFmtr fmtr(now, sess);
         WarningLog(<<"setMood ByPendReason that is None: "
            <<" cat="<<mCat
            <<" sess="<<fmtr
            <<" newpr="<<pendReason);
         resip_assert( 0 );
      }
      mood = sess.mPendReason==REND_PR_Close 
         ?  REND_SM_Recycle : REND_SM_Open;
   }

   DebugLog(<<"setMood"
      <<" idx="<<sess.mSessionIdx
      <<" cur="<<sess.mMood
      <<" new="<<mood);

   if ( sess.mMood != REND_SM_None ) 
   {
      RendSessionMoodChain& chain = mMoods[sess.mMood].mChain;
      RendSessionMoodChain::iterator it = chain.iterator_to(sess);
      chain.erase(it);
   }

   if ( mood == REND_SM_PendNotify ) 
   {
      resip_assert(sess.mPendReason!=0);	// must have reason from before
   } 
   else 
   {
      sess.mPendReason = pendReason;
   }

   sess.mMood = mood;
   sess.mLastMoodTime = now;
   
   if ( mood!=REND_SM_None ) 
   {
      mMoods[mood].mChain.push_back(sess);
   }

   if ( mood==REND_SM_Idle || mood==REND_SM_Recycle ) 
   {
      scheduleRenew(now, sess, 0);	// remove from tracker
   }
}

void
RendTroopBase::spliceMoodList(RendTimeUs now, RendSessionMood fromMood,
                              RendSessionMood toMood, RendPendReason pendReason) 
{
   resip_assert(toMood!=REND_SM_None);
   resip_assert(fromMood!=REND_SM_None);
   resip_assert(toMood!=fromMood);
   RendSessionMoodChain& chain = mMoods[fromMood].mChain;
   RendSessionMoodChain::iterator it = chain.begin();
   for ( ; it != chain.end(); ++it) 
   {
      RendSession& sess = *it;
      sess.mMood = toMood;
      sess.mPendReason = pendReason;
      sess.mLastMoodTime = now;
   }
   RendSessionMoodChain& toChain = mMoods[toMood].mChain;
   toChain.splice( toChain.end(), chain);
   validate(now); // XXX
}

int
RendTroopBase::startMoodChange(RendTimeUs now, RendSession& sess, RendSessionMood toMood) 
{
   int cnt = 0;
   RendSessionMood fromMood = sess.mMood;
   RendDlg *dlg = sess.mDlgPtr.get();

   if ( toMood==REND_SM_GROUP_ByPendReason ) 
   {
      // special value outside enum
      switch ( sess.getPendReason() ) 
      {
      case REND_PR_Open:
      case REND_PR_Modify:
      case REND_PR_OpenOrModify:
      case REND_PR_Renew:
         cnt = startDlgOpen(now, sess, sess.getPendReason()); 
         break;
      case REND_PR_Close:
         cnt = startDlgClose(now, sess); 
         break;
      default: resip_assert(0);
      }
      return cnt;
   }
   if ( toMood==REND_SM_GROUP_ByDlgState ) 
   {
      // put it into either Open or Closed depending upon underlying state
      if ( dlg==NULL || dlg->mState==REND_DS_None ) 
      {
         setSessionMood(now, sess, REND_SM_Idle);
      } 
      else if ( dlg->mState==REND_DS_Established ) 
      {
         setSessionMood(now, sess, REND_SM_Open);
      } 
      else 
      {
         resip_assert(0);
      }
      return 0;
   }
   switch ( toMood ) 
   {
   case REND_SM_Open: 
      cnt = startDlgOpen(now, sess, REND_PR_OpenOrModify); 
      break;

   case REND_SM_Idle:
      if ( fromMood==REND_SM_Recycle ) 
      {
         resip_assert(dlg);
         dlg->relDlgState();
         // wondering if something with SharedPtr logic is messed up
         // and we are killing dialog object, not just clearing state
         resip_assert( dlg == sess.mDlgPtr.get() );
         setSessionMood(now, sess, REND_SM_Idle);
      } 
      else 
      {
         cnt = startDlgClose(now, sess); 
      }
      break;

   default:
      CritLog(<<"Illegal session mood change:"
              <<" fromMood="<<RendSessionMoodFmt(fromMood)<<"."<<fromMood
              <<" toMood="<<RendSessionMoodFmt(toMood)<<"."<<toMood);
      resip_assert(0);
   }
   return cnt;
}

/**
    Open {numDlgs} that are currently Closed. XXX
**/
int
RendTroopBase::changeDlgCnt(RendTimeUs now, int numDlgs, RendSessionMood fromMood, RendSessionMood toMood) 
{
   if ( numDlgs <= 0 )
      return 0;

   int loopcnt = 0;
   int dcnt = 0;
   RendSessionMoodChain& chain = mMoods[fromMood].mChain;
   RendSessionMoodChain::iterator it = chain.begin();
   for ( ; it != chain.end() && dcnt < numDlgs; loopcnt++) 
   {
      resip_assert( loopcnt < 100000 );
      RendSession& sess = *it;
      resip_assert( sess.mMood == fromMood );
      ++it; // do it now before sess moves lists
      DebugLog(<<"ChangeDlg"
         <<" idx="<<sess.mSessionIdx
         <<" from="<<fromMood
         <<" to="<<toMood);
      int cnt = startMoodChange(now, sess, toMood);
      if ( cnt > 0 )
         ++dcnt;
   }
   return dcnt;
}

int
RendTroopBase::renewDlgCnt(RendTimeUs now, int numDlgs) 
{
   int workDone = 0;
   for (; workDone < numDlgs; workDone++) 
   {
      RendSession* sess = mTracker->popDlg(now);
      if ( sess==NULL )
         break;
      // better be open and ready to expire 
      // (should have been removed from tracker when closed)
      resip_assert( sess->mExpireAbsTime != 0 );
      resip_assert( sess->mMood != REND_SM_Idle );
      RendSessionFmtr fmtr(now, *sess);
      InfoLog(<<"RenewDlg sess="<<fmtr);
      startDlgOpen(now, *sess, REND_PR_Renew);
   }
   return workDone;
}

int
RendTroopBase::checkAge(RendTimeUs now, RendHandleMoodFnc handler, 
                        RendSessionMood mood, int failAge, bool failAll,
                        int &maxAgeRet) 
{
   RendHandleMoodCxt cxt(now, mCat, this);
   // assert below is critical, but reflects current usage
   resip_assert( mood==REND_SM_PendReq || mood==REND_SM_PendNotify );
   RendSessionMoodChain& chain = mMoods[mood].mChain;
   RendSessionMoodChain::iterator it = chain.begin();
   int maxAge = 0;
   int totCnt = 0;
   int oldCnt = 0;
   for ( ; it != chain.end(); ) 
   {
      RendSession& sess = *it;
      ++it; // increment now before we change session list
      // RendDlgAcctKey key = cvtIdxToKey(sess.mSessionIdx);
      ++totCnt;
      int age = sess.getLastMoodSecs(now);
      resip_assert( age >= 0 );

      if ( age > maxAge )
         maxAge = age;

      if ( failAll || age > failAge ) 
      {
         ++oldCnt;
         // RendPubDlg* pdlg = getPubDlg(sess);
         // assert( pdlg );
         cxt.mSess = &sess;
         cxt.mAge = age;
         cxt.mKey = cvtIdxToKey(sess.mSessionIdx);
         cxt.mNewPendReason = 0;
         RendSessionMood newMood = handler(cxt);
         if ( newMood != REND_SM_None ) 
         {
            setSessionMood(now, sess, newMood, cxt.mNewPendReason);
         }
      }
   }

   if ( totCnt > 0 ) 
   {
      WarningLog(<<"checkAge: #pend="<<totCnt
                 <<" #old="<<oldCnt
                 <<" maxage="<<maxAge);
   }
   maxAgeRet = maxAge;
   return oldCnt;
}

/**
    Do work within the troup. Our work is defined by:
    - mTargetCalls	-- total number of "calls" we want to have
    - mTargetRate	-- maximum number of changes-per-second

    The target level (number calls) is tracked per-troup and
    is internal state.
**/
int
RendTroopBase::doWork(RendTimeUs now, int minWork, int maxWork)
{
   int numDone;
   int workDone = 0;

   changeDlgCnt( now, 1000*1000, REND_SM_Recycle, REND_SM_Idle);

   // clean-up any left-over wave dialogs
   changeDlgCnt( now, 1000*1000, REND_SM_Wave, REND_SM_GROUP_ByDlgState);

   int curLevel = mMoods[REND_SM_Open].mChain.size() + mMoods[REND_SM_PendReq].mChain.size();

   // if tgt negative, disable level-seeking logic
   int moreCalls = mTgtOpenDlgs>=0 ? mTgtOpenDlgs - curLevel : 0;

   // InfoLog(<<"doWork: moreCalls="<<moreCalls<<" min="<<minWork<<" max="<<maxWork);

   if ( maxWork-workDone>0 && moreCalls < 0 ) 
   {
      // close some open dialogs to meet the work target
      numDone = changeDlgCnt( now, rendMIN(-moreCalls,maxWork-workDone), 
         REND_SM_Open, REND_SM_Idle);
      workDone += numDone;
      moreCalls += numDone;
   }

   if ( maxWork-workDone > 0 && moreCalls >= 0 ) 
   {
      // renew some open dialogs, but only if we didn't close anything
      numDone = renewDlgCnt( now, maxWork-workDone);
      workDone += numDone;
   }

   if ( maxWork-workDone > 0 && moreCalls > 0 ) 
   {
      // open some idle dialogs to meet the work target
      numDone = changeDlgCnt( now, rendMIN(moreCalls,maxWork-workDone), 
         REND_SM_Idle, REND_SM_Open);
      workDone += numDone;
      moreCalls -= numDone;
   }

   if ( minWork-workDone > 0 ) 
   {
      // if not enough done, close some open dialogs
      // only do half the work: re-opening will be the other half
      numDone = changeDlgCnt( now, (minWork-workDone+1)/2, REND_SM_Open, REND_SM_Idle);
      workDone += numDone;
      moreCalls -= numDone;
   }

   return workDone;
}

int
RendTroopBase::adjustAllDlgs(RendTimeUs now, RendHandleMoodFnc handler, int maxWork) 
{
   if ( now==0 ) 
   {
      // special value that means to start from begining
      changeDlgCnt( now, 1000*1000, REND_SM_Recycle, REND_SM_Idle);
      mAdjustIdx = 0;
      return 0;
   }

   int totWork = 0;
   resip_assert( mAdjustIdx >= 0 );
   resip_assert( maxWork > 0 );
   RendHandleMoodCxt cxt(now, mCat, this);
   for ( ; (unsigned)mAdjustIdx < mDlgVec.size() && maxWork > 0; mAdjustIdx++) 
   {
      RendSession& sess = mDlgVec[mAdjustIdx];
      cxt.mSess = &sess;
      cxt.mAge = sess.getLastMoodSecs(now);
      cxt.mKey = cvtIdxToKey(mAdjustIdx);
      cxt.mNewPendReason = 0;
      RendSessionMood newMood = handler(cxt);
      if ( newMood==0 || newMood==sess.mMood )
         continue;
      if ( newMood==REND_SM_Wave ) 
      {
         setSessionMood(now, sess, newMood, cxt.mNewPendReason);
      } 
      else 
      {
         int cnt = startMoodChange(now, sess, newMood);
         InfoLog(<<"TroopAdjust: key="<<cxt.mKey
            <<" from="<<RendSessionMoodFmt(sess.mMood)
            <<" to="<<RendSessionMoodFmt(newMood));
         resip_assert(cnt==1);
         ++totWork;
         --maxWork;
      }
   }
   return totWork;
}

#if 0
RendSessionMood
RendTroopBase::checkGoalMood(RendTimeUs now, RendSession& sess, const RendDlgAcctKey& key, RendPendReason &pr) 
{
   if ( mOpenToIdxBase >= 0 && mOpenToIdxLen >= 0 ) 
   {
      // NOTE: This code is no longer used; moved into TroopPub
      int ndlgs = mToSlice.getDlgLen();
      resip_assert( mOpenToIdxBase < ndlgs );
      int idx = (key.mToIdx - mOpenToIdxBase + ndlgs) % ndlgs;
      return idx >= 0 && idx < mOpenToIdxLen ? REND_SM_Open : REND_SM_Idle;
   }
   resip_assert(0);	// subclass should define this
   return REND_SM_None;
}
#endif


void
RendTroopBase::sessionError(RendTimeUs now, RendSession& sess, const char *msg)
{
   RendDlgAcctKey key = cvtIdxToKey(sess.mSessionIdx);
   RendSessionFmtr fmtr(now, sess);
   ErrLog(<<"Session error: (" <<" key=" << key <<" sess=" << fmtr <<"): "<< msg << ".  Recycling.");
   setSessionMood(now, sess, REND_SM_Recycle, REND_PR_SessionError);
   // XXX: increment error cnt
}

void
RendTroopBase::validate(RendTimeUs now)
{
   int didx;
   for (didx=0; didx  < mMaxSessions; didx++)
   {
      RendSession& sess = mDlgVec[didx];
      RendDlg* dlg = sess.mDlgPtr.get();
      if ( dlg==NULL && !(sess.mMood==REND_SM_Idle 
         || (sess.mMood==REND_SM_Wave && sess.getPendReason()==REND_PR_Open) ) )
      {
         sessionError(now, sess, "validate: missing dialog");
         continue;
      }
      switch ( sess.mMood ) 
      {
      case REND_SM_Idle:
         if ( dlg && dlg->mState!=REND_DS_None ) 
         {
            sessionError(now, sess,"validate: idle session with non-None dialog state.");
            continue;
         }
         break;

      case REND_SM_Open:
         if ( dlg && dlg->mState!=REND_DS_Established ) 
         {
            sessionError(now, sess,"validate: open session with non-Est dialog state.");
            continue;
         }
         break;

      case REND_SM_PendReq:
      case REND_SM_PendNotify:
         // if pending on NOTIFY, might want to wait much longer!
         if ( sess.getLastMoodSecs(now) > 45 ) 
         {
            sessionError(now,sess,"validate: pending too long.");
            continue;
         }
         break;

      case REND_SM_Recycle:
         // should check to make sure not stuck!
         break;

      case REND_SM_Wave:
         break;

      default:
         resip_assert(0);
      }
   }

   int moodIdx=0;
   for ( ; moodIdx < REND_SM_MAX; moodIdx++) 
   {
      RendSessionMood mood = (RendSessionMood)moodIdx;

      if ( mood==REND_SM_None )
         continue;

      RendSessionMoodChain& chain = mMoods[mood].mChain;
      RendSessionMoodChain::iterator it = chain.begin();
      unsigned cnt = 0;
      for ( ; it != chain.end(); ++it) 
      {
         ++cnt;
         resip_assert( cnt < 1000000 );
         resip_assert( it->mMood == mood );
      }
      resip_assert( cnt == chain.size() );
   }

   // RendSessionMoodChain& chain = mMoodChains[fromMood];
   // RendSessionMoodChain::iterator it = chain.begin();
   // for ( ; it != chain.end() && dcnt < numDlgs; )
}

/**
    Initialize list of candidate dialogs.
    See cvtIdxToKey() for example of how used.
    This initializes {repeatLen} dialogs for each of {acctLen}
    accounts. The first account is {acctBase} and the first repeast
    index is {repeatBase}.
**/
void
RendTroopBase::setVectorDlgs(int numDlgs, 
                             int acctBase, int acctLen, int acctStride,
                             int repeatBase, int repeatLen)
{
   mToSlice.setMap(numDlgs, acctBase, acctLen, acctStride);
   mFromSlice.clearMap();
   mRepeatSlice.setMap(repeatLen, repeatBase, repeatLen, 1);
   prepareDlgs();
}

/**
   For subscriptions: 
   FROM corresponds to watcher
   TO corresponds to watchee (PUBLISHer)
**/
void
RendTroopBase::setMatrixDlgs(int numFrom, int numTo, int repeatBase, int repeatLen) 
{
   int acctBase = 0;
   int acctStride = 1;
   mToSlice.setMap(numTo, acctBase, numTo, acctStride);
   mFromSlice.setMap(numFrom, acctBase, numFrom, acctStride);
   mRepeatSlice.setMap(repeatLen, repeatBase, repeatLen, 1);
   prepareDlgs();
}

void
RendTroopBase::prepareDlgs() 
{
   // assuming the slide parameters have been changed...

   // XXX: explicit clean up old dialogs?
   RendTimeUs now = 0;

   mMaxSessions = mToSlice.getDlgLen() * mFromSlice.getDlgLen() * mRepeatSlice.getDlgLen();
   mDlgVec.reserve(mMaxSessions);
   // NOTE: above reserve not strictly required, but seems safer
   mDlgVec.resize(mMaxSessions);	// use default constructor
   int didx;
   for (didx=0; didx  < mMaxSessions; didx++)
   {
      RendSession& sess = mDlgVec[didx];
      sess.mSessionIdx = didx;
      setSessionMood(now, sess, REND_SM_Idle);
      RendDlgAcctKey key = cvtIdxToKey(didx);
      RendSessionIdx didx2 = cvtKeyToIdx(key);
      if ( didx2 != didx )
      {
         CritLog(<<"Key mapping error: " <<" idxIn="<<didx <<" key="<<key <<" idxOut="<<didx2);
      }
      resip_assert( didx2 == didx );
   }

   mRpt.mMaxSessions = mMaxSessions;
}

RendDlgAcctKey
RendTroopBase::cvtIdxToKey(RendSessionIdx idx)
{
   RendAcctIdx remidx = idx;
   // NOTE that the toIdx is first, to mimic SUBSCRIBEs where
   // one client (one from addr) has "adjacent" SUBs to different watchees
   // (the to addr).
   // WATCHOUT: below modifies remidx
   RendAcctIdx toIdx = mToSlice.mapDlgIdx(remidx);
   RendAcctIdx fromIdx = mFromSlice.mapDlgIdx(remidx);
   RendAcctIdx repIdx = mRepeatSlice.mapDlgIdx(remidx);
   resip_assert( remidx==0 );
   return RendDlgAcctKey(mCat,fromIdx,toIdx,repIdx);
}

RendSessionIdx
RendTroopBase::cvtKeyToIdx(const RendDlgAcctKey& key)
{
   resip_assert( key.mCat==mCat || key.mCat==0 );
   // mMaxSessions = mToSlice.getDlgLen() * mFromSlice.getDlgLen() * mRepeatSlice.getDlgLen();
   if ( key.mCat!=mCat 
      || !mRepeatSlice.checkIdx(key.mRepeatIdx)
      || !mFromSlice.checkIdx(key.mFromIdx)
      || !mToSlice.checkIdx(key.mToIdx) )
   {
      return -1;
   }

   RendSessionIdx idx = 0;
   // XXX: need to range check the key indecies or we get garbage
   // below modifies idx as we built it up.
   mRepeatSlice.foldIdx(idx, key.mRepeatIdx);
   mFromSlice.foldIdx(idx, key.mFromIdx);
   mToSlice.foldIdx(idx, key.mToIdx);
   return idx;
}

RendSession*
RendTroopBase::getSession(RendSessionIdx idx)
{
   if ( idx < 0 || (unsigned)idx >= mDlgVec.size() )
      return NULL;
   RendSession *sess = &mDlgVec[idx];
   return sess;
}

void
RendTroopBase::bindDlg(RendSession& sess, RendTroopDlg *dlg)
{
   dlg->buildAndRegLocalTag(sess.mDlgPtr);
   dlg->mTroopSessionIdx = sess.mSessionIdx;
}

int
RendTroopBase::startDlgCmd(RendTimeUs now, RendSession& sess, 
                           RendTroopDlg *dlg, int expSecs, RendPendReason pendReason)
{
   int sts = dlg->sendNextCmd(expSecs, /*retry*/0);
   resip_assert( sts==1 );	// could be busy, we don't handle this yet
   setSessionMood(now, sess, REND_SM_PendReq, pendReason);
   return sts;
}

int
RendTroopBase::startDlgOpen(RendTimeUs now, RendSession& sess, RendPendReason pendReason) 
{
   RendTroopDlg *dlg = getDlg(sess);
   if ( dlg==NULL )
   {
      resip_assert( pendReason==REND_PR_Open || pendReason==REND_PR_OpenOrModify );
      pendReason = REND_PR_Open;
      RendDlgAcctKey key = cvtIdxToKey(sess.mSessionIdx);
      dlg = createNewDlg(key);
   }
   if ( dlg->mState == REND_DS_None ) 
   {
      // either brand-new (above) or previous released
      // let's hope all prior state was properly released!
      resip_assert( pendReason==REND_PR_Open || pendReason==REND_PR_OpenOrModify );
      pendReason = REND_PR_Open;
      bindDlg(sess, dlg);
   } 
   else if ( dlg->mState == REND_DS_Closing || dlg->mState==REND_DS_Closed ) 
   {
      resip_assert(0);
   } 
   else 
   {
      if ( pendReason==REND_PR_OpenOrModify ) 
      {
         pendReason = REND_PR_Modify;
      } 
      else 
      {
         resip_assert( pendReason==REND_PR_Modify || pendReason==REND_PR_Renew );
      }
   }
   int expSecs = getScatterExpires();
   return startDlgCmd(now, sess, dlg, expSecs, pendReason);
}

int
RendTroopBase::startDlgClose(RendTimeUs now, RendSession& sess)
{
   RendTroopDlg *dlg = getDlg(sess);
   resip_assert(dlg);
   // XXX: Below might not be true due to race; fix later
   resip_assert(dlg->mState == REND_DS_Established);
   int expSecs = 0;
   // InfoLog(<<"closing dialog idx="<<sess.mSessionIdx);
   return startDlgCmd(now, sess, dlg, expSecs, REND_PR_Close);
}

int
RendTroopBase::doSimpleAction(RendTimeUs now, RendSimpleAction act)
{
   switch ( act ) 
   {
   case REND_SA_Ping:
      return 1;

   case REND_SA_SkipPendNotify:
      return checkPendNotify(now, /*skipNow*/true);

   default:
      ;
   }
   return -3;
}


/***********************************************************************
 *
 * RendTroopDlg
 *
 ***********************************************************************/


int
RendTroopDlg::getRspExpire(const resip::SipMessage& rsp) 
{
   if ( rsp.exists(resip::h_Expires) ) 
   {
      return rsp.header(resip::h_Expires).value();
   }
   return -100;
}

void
RendTroopDlg::handleResponse(RendTimeUs now, const resip::SipMessage *msg, const char *failWhy) 
{
   RendTroopBase& troop = getTroop();
   RendSession *sess = troop.getSession(mTroopSessionIdx);

   if ( sess==NULL )
      return;

   RendDlg *boundDlg = sess->getDlg();
   if ( boundDlg != this )
      return;

   if ( sess->mMood!=REND_SM_PendReq ) 
   {
      RendDlgAcctKey key = troop.cvtIdxToKey(mTroopSessionIdx);
      int delaySec = REND_US2S(now-mReqSendTime);
      WarningLog(<<"Bad response: Got response when not pending:"
         <<" key="<<key
         <<" mood=" <<(sess->mMood)
         <<" code=" <<mLastRspCode
         <<" delay="<<delaySec
         );
      // This is "standard" error case, where we send request,
      // server is slow, we give up waiting, and then
      // response comes in. We just drop it, because who
      // knows what the test app has decided to do since giving up
      return;
   }
   handleSessResponse(now, *sess, msg, failWhy);
} 

void
RendTroopDlg::handleSessResponse(RendTimeUs now, RendSession& sess, 
                                 const resip::SipMessage *rsp, const char *failWhy)
{
   // assert( sess.mMood==REND_SM_PendReq ); our caller checks this
   int rspCode = rsp->header(resip::h_StatusLine).responseCode();
   RendTroopBase& troop = getTroop();

   if ( mState==REND_DS_Established ) 
   {
      if ( rspCode >= 300 ) 
      {
         /* have established dialog from earlier transaction, but this
         * one failed. Teardown entire session. Common reasons for
         * this would be:
         *   412 Conditional request failed (PUB etag expired out)
         *   481 Unknown call (SUB expired out)
         */
         WarningLog(<<"Troop: in-dialog request failed:"
            <<" PR="<<sess.getPendReason()
            <<" code="<<rspCode
            <<" reason="<<rsp->header(resip::h_StatusLine).reason()
            <<" warn="<<mLastRspWarnings);
         resip_assert(sess.getPendReason()!=REND_PR_Open); 
         troop.mCntMgr.inc(REND_CntCode_ReqFailRsp, troop.getDlgCat());
         troop.setSessionMood(now, sess, getPostRspMood(now, sess, false),
            REND_PR_BadRspRenew);
         return;
      }
      int serverExpire = getRspExpire(*rsp);
      // we don't yet handle the polling SUBSCRIBE (E=0)
      resip_assert( serverExpire > 1 );
      if ( sess.getPendReason() == REND_PR_Open ) 
      {
         ++troop.mRpt.mOpenGoodCnt;
      } 
      else if ( sess.getPendReason() == REND_PR_Renew || sess.getPendReason()==REND_PR_Modify ) 
      {
         ++troop.mRpt.mRenewGoodCnt;
      } 
      else 
      {
         resip_assert(0);
      }
      troop.mCntMgr.inc(REND_CntCode_ReqGood, troop.getDlgCat());
      troop.addMoodDur(sess.mMood, mRspRecvTime - mReqSendTime);
      RendSessionMood toMood = getPostRspMood(now, sess, true);
      troop.setSessionMood(now, sess, toMood);
      if ( toMood != REND_SM_Idle && toMood != REND_SM_Recycle )
      {
         troop.scheduleRenew(now, sess, serverExpire);
      }
   } 
   else if ( mState==REND_DS_Opening ) 
   {
      ++troop.mRpt.mOpenFailCnt;
      // TBD: increment fail cause count
      // XXX: should check to make sure actually fail rsp and not
      // something else like malformed message
      troop.mCntMgr.inc(REND_CntCode_ReqFailRsp, troop.getDlgCat());
      troop.setSessionMood(now, sess, getPostRspMood(now, sess, false),
         REND_PR_BadRspOpen);
   } 
   else if ( mState==REND_DS_Closed ) 
   {
      resip_assert( sess.getPendReason() == REND_PR_Close );
      ++troop.mRpt.mCloseGoodCnt;
      resip_assert( rspCode >= 200 && rspCode < 299 );
      troop.addMoodDur(sess.mMood, mRspRecvTime - mReqSendTime);
      RendSessionMood toMood = getPostRspMood(now, sess, true);
      troop.setSessionMood(now, sess, toMood);
   } 
   else if ( mState==REND_DS_Closing ) 
   {
      resip_assert( sess.getPendReason() == REND_PR_Close );
      ++troop.mRpt.mCloseFailCnt;
      troop.setSessionMood(now, sess, getPostRspMood(now, sess, false),
         REND_PR_BadRspClose);
   } 
   else 
   {
      WarningLog(<<"Bad response: unexpected state="<<mState<<
         " while mood pending,"
         <<" code=" <<mLastRspCode
         );
      resip_assert(0);
   }
}

RendSessionMood
RendTroopDlg::getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood) 
{
   if ( isGood )
   {
      return sess.getPendReason()==REND_PR_Close ? REND_SM_Recycle : REND_SM_Open;
   }
   return REND_SM_Recycle;
}

void
RendTroopDlg::handleConnTerm(RendTimeUs now) 
{
   RendTroopBase& troop = getTroop();
   RendSession *sess = troop.getSession(mTroopSessionIdx);
   if ( sess==NULL )
      return;

   RendDlg *boundDlg = sess->getDlg();
   if ( boundDlg != this )
      return;

   switch ( sess->mMood ) 
   {
    case REND_SM_Recycle:
       // some error (probably 408/503 response) kick us out already
       return;

    case REND_SM_Open:
       // this is "normal" case for this error
       // really? maybe at end?
       WarningLog(<<"Got ConnTerm in Open; need to clean up");
       break;

    case REND_SM_Wave:
       // this is "normal" case for this error
       // really? maybe at end?
       WarningLog(<<"Got ConnTerm in Wave; need to clean up");
       break;

    case REND_SM_PendReq:
       WarningLog(<<"Got ConnTerm in PendReq (what happened to local 503?)");
       break;

    case REND_SM_PendNotify:
       WarningLog(<<"Got ConnTerm in PendNot (what happened to local 503?)");
       break;

    case REND_SM_Idle:
       WarningLog(<<"Got ConnTerm in Idle (why?)");
       break;

    default:
       WarningLog(<<"Got ConnTerm in stange state");
   }
   troop.setSessionMood(now, *sess, REND_SM_Recycle, REND_PR_ConnTerm);
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
