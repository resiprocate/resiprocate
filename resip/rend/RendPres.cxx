/**
    REND presence (PUBLISH, SUBSCRIBE, NOTIFY)

    Written by Kennard White (Logitech, Inc.), 2010..2011
**/

#include <iomanip>
#include <boost/bind.hpp>

#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"

#include "RendTroop.hxx"
#include "RendSketch.hxx"

#include "popt.h"

#ifdef WIN32
int ffsl(int mask) 
{
   int bit;
   if (mask == 0)
   {
      return(0);
   }
   
   for (bit=1; !(mask & 1); bit++)
   {
      mask >>= 1;
   }
   return(bit);
}
#endif


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

/**
    It is somewhat unclear what is the target of the SUBSCRIBE: it this 
    is the To URI or the R-URI of the initial request.


    For presence, we have multiple troops (one for PUBs, one for SUBs).
    How do we apportion workLevel and workRate among the two troops?
    Consider workLevel. How should this be defined? Options:
    - Sum of open dialogs. But how to apportion between open PUBs
      and open SUBs?
    - Keep PUBs fixed, and vary number of SUBs with level.
    - Use 

    Given N accts and a limit of at most M NOTIFYs per PUBLISH, then
    #dlgs = N + N*MIN(N,M)
    Thus could consider N as the work level. Once level>#accts, number
    of open publishs would stop. For SUBs, repeats could be used.

    Changing #subs has challenge in that not all SUBs are the same.
    Want to keep the number of SUBs-per-publisher the balanced. Thus
    current linked list structure is problematic.

    One option is to link all SUBs watching a given PUB together.
    When scaling up or down, a given PUB can be walked to find
    SUBs, which can then be state transitioned. This linkage would
    also help in checking notifies.

    The maxWork rate has simple interpretation wrt to delaying
    wave-style PUB/SUBs (accounting for NOTIFYs). minWork rate
    could also be view as driving how fast we do the wave-style
    traversal.

    The core wave style could be viewed as just PUBs, with the
    SUBs handled explicitly and with automatic renews. But could
    add option to handle explicit reSUBs and/or explict tear downs
    and stores every N cycles.

    SUB new wave
    PUB[new] wave
    loop PUB[renew] wave
    PUB[remove] wave

    Note: repeated PUBs do make sense in context of simulating multiple
    clients (devices) for same AOR. Need to have unqieue tuples and such,
    but otherwise makes sense to do.

    Above isn't true: can have distict tuples, and make we get them all!

**/

/**
    Next challenge is defining when to "wait" before going on.
    e.g., not just rate limited but also some maximum level of outstanding
    work.

    PendingLimit: just count the pending queue, and don't do more work
    until this completes. This is O(1) complexity.

    PendingAge: if oldest pending is older than this, then stop doing new
    work. This is handle case where a message has been lost (due to
    too much work), and if we quite down then the retrans will get it thru.
    This is hard, need research queue periodically to find oldest, or
    maintain queue in sort order. Thus O(N) with small factor.

    Requests are easy, since we expect exactly one final response for each
    request.

    Notify are harder. When PUB, expect one notify for each SUB linked
    to that PUB.

    The "right" solution is to mirror the sipserver. Have DB of presentities,
    and link to the PUBs and SUBs of that AOR. For given PUB, can
    find presentity, then traverse down to all the SUBs and verify correct.

    One idea is that we use bitmasks/arrays. Support at most 32 publication
    per account (via repeat index). Each SUB keeps array of NOTIFY
    that it got. It really just needs to keep the latest PUB SN for each
    PUB repeat(aka tuple). Can use just bitmask, which indicates if
    latest is received or not. Bit is invalidated on PUB, and updated
    on each NOTIFY. Checking if PUB is complete is just walking the SUBS
    of the PUB and checking bits.

    How to index the SUBs of the presentity? If no PUB repeats, this
    would be easy: just link all the SUBS into the PUB they are watching.
    But with multiple PUBs, need multiple links in each SUB. SO then
    into arrays. Doable, but gets expensive.

    Another option would be to allocate huge table:
       pidfStatus[fromAcct][fromRpt][toAcct][toRpt]
       fromAcct is watcher (subscriber)
       fromRpt is number of SUB repeats
       toAcct is watchee (publisher) (presentity index)
       toRpt is the number of PUB repeats

    The value is a single bit: PIDF is up-to-date

    A given PUB dlg has fixed (fromAcct,fromRpt)
    A given SUB dlg has fixed (fromAcct,toAcct,toRpt) (but fromRpt is free)

    On publish, would increment PUB SN, and:
       pidfStatus[*][*][toAcct][toRpt] = 0
    On NOTIFY, would extract the tuples from the PIDF
       getting set {(fromRpt,fromSN)} for fixed (fromAcct,toAcct,toRpt)
    Each can be checked check for up-to-date-ness, and:
       foreach fromRpt that is valid:
           pidfStatus[fromAcct][fromRpt][toAcct][toRpt] = 0
    A PUB is complete when:
           min(pidfStatus[fromAcct][fromRpt][*][*]) = 1

    Note that an optimization is possible if #SUB rpts is less than 32.
    Then the toRpt index drops out and the computations above are easier.

    Another optimization is to not check the PIDF for validity,
    but to simple assume good and on each NOTIFY:
           pidfStatus[fromAcct][*][toAcct][toRpt] = 1
    Above can be done for each fromRpt in the PIDF, or just accross the
    board.

    It isn't entirely clear the best place to keep PUBLISH state
    such as the current serial number and the existence of the publish.
    It could be kept in the dialog and replicated in the Event state,
    or vv. Decided to keep it in Event state because more DB-like,
    and also because the dialog might be wiped in failure cases (maybe).

    How to handle unPUBs? Ideally, we would like to verify that
    every watching SUB gets a new NOTIFY that doesn't contain
    a tuple from the PUB. When we unPUB, we advance PidfSn in
    internal state and mark expired. Then when we get NOTIFY, there
    are two actions:
    a) [positive case] Search for all provided tuples, and mark those ready.
    b) [negative case] Iterate all PUBs of the presentity we are watching,
       and if expired, and we didn't get it, then mark it ready.

   For negative case, we need to walk the PUB repeats of the presentity.
   One idea, is when doing the positive case, build bitmask of 
   presentity repeats that we got (regardless if current or not).
   The complement of this is then the PUB repeats we didn't get.
   Each of these PUBs that is expired should be marked ready (for this SUB).
   
   Above is a lot of walking. Another idea is to track unPUBs separately.
   Could have unpubReady[pubAcct][subAcct][subRpt] = ready mask(pubRpt)
    or
      unpubReady[pubAcct][pubRpt][subAcct] = ready mask(subRpt)
    For later, on unPUB do:
      unpubReady[pubAcct][pubRpt][*] = 0
    And on NOTIFY do
      unpubReady[pubAcct][pubRpt][subAcct][subRpt] = 1
    Hmmm, just like pidfReady, so use that for now
**/

/**
    Monitoring the status of the wave is hard.

    Want to count tuples, not NOTIFYs, due to multiple PUBLISH repeats.
    Several approaches:
    1. Count bits. Count the number of tuples not yet received. One
       complication is that we dont mark PUBs stale until set, so
       they look good early in the wave, and could go stale later
       in the wave. One approach here is to bump the SN of everything
       at start of wave, and then again when sending the PUB.
    2. Do math. Based upon number PUBs and SUB matrix, can predict this.

    Related to above, need to how many more NOTIFY messages we
    expect to receive based upon outstanding PUBs (actually issues
    ones, not ones we will issue in future). This is used for proper
    throttling to delay sending more PUBs. There are several different
    approaches to this.
    1. Count bits. Iterate thru the pend PUB queues, and for each
       one count the number of pending tuples.  Would be best to
       collapse into expected message count rather than just
       tuples, but this is difficult.
    2. Do math. We already compute the MaxSubsPerPubs. So can get
       upper bound by #PUBs * MaxSubsPerPubs. This probably makes
       sense for stuff in the PendReq.
    3. One idea is to use math upper bound for PendReq, and then
       counts bits for PendNot.

    The thing we do MOST is check size of pending list -- we do this
    every work chunk in order to compute how much more work we can
    issue. Since this is what we do MOST, then that is what we need
    to optimize for. That means extra book keeping to count size
    of pending list.
**/

/********************************************************************
 *
 * XXX-specific classes
 *
 ********************************************************************/

/**
    mWatching[] = conceptually
        isWatching[toAcct][fromAcct][fromRpt] = bool
    With same conventions as below, with [fromRpt] treated as bitmask

    mPidfReady[] = conceptually
      ready[toAcct][toRpt][fromAcct][fromRpt] = bool
    where 
       fromAcct = is watcher (subscriber)
       fromRpt = is repeat of that watcher (another client)
       toAcct = is watchee (publisher)
       toRpt = is repeat of that watchee (another client)
    We limit the number of subscriber repeats to be < 32,
    and store the [fromRpt] dimension of the array as a bitmask.

    mPubPosture[] 
       posture[pubAcct][pubRpt] = (SN, exp)
**/


/* This is the HAKMEM169/MIT/X11 algorithm for counting bits.
 * It is widely published on web. It seems to be the fastest
 * non-lookup table approach.
 */
static inline unsigned rendPopcount(UInt32 val)
{
   UInt32 tmp = val - ((val >> 1) & 033333333333) - ((val >> 2) & 011111111111);
   return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}

struct RendEventPubPosture 
{
   RendEventPubPosture() : mWatchable(false), mPidfSn(0), 
      mExpireAbsSecs(0), mSessionIdx(0), 
      mAbsPubTime(0), mPendCnt(-1) { }
   bool mWatchable;
   unsigned mPidfSn;
   UInt32 mExpireAbsSecs; // absolute time, 0 if not active
   unsigned mSessionIdx; // kind of like callback data
   RendTimeUs mAbsPubTime; // Used for timing server latency
   int mPendCnt;
};


struct RendTupDetail 
{
   unsigned mPubAcctIdx;
   unsigned mPubRptIdx;
   int mPubExpireSecs; // remaining time
   unsigned mPubPidfSn;
   unsigned mSubAcctIdx;
   unsigned mSubRptIdx;
   UInt32 mWatchMask;
   UInt32 mReadyMask;
   UInt32 mPendMask;

   // Total number of subscriptions (incl repleats) to this presentity
   unsigned mWatchCnt;
   // Total number of pending subscriptions (incl repeats) to this presentity
   unsigned mPendCnt;

   EncodeStream& fmt(EncodeStream& os) const;
   RendDlgAcctKey toPubKey() const 
   {
      // TBD: add asserts?
      // should from be zero?
      return RendDlgAcctKey(REND_DlgCat_Sub, mPubAcctIdx, mPubAcctIdx, mPubRptIdx);
   }
   RendDlgAcctKey	toSubKey() const
   {
      // TBD: add asserts?
      return RendDlgAcctKey(REND_DlgCat_Sub, mPubAcctIdx, mSubAcctIdx, mSubRptIdx);
   }
};

static inline EncodeStream& operator<<(EncodeStream& os, const RendTupDetail& detail) 
{
   return detail.fmt(os);
}


class RendEventNotifyIf
{
public:
   virtual void handleNotifyDone(RendTimeUs now, unsigned sessionIdx, RendTimeUs absPubTime, bool isNotify) = 0;
};

class RendPubSubEvent
{
public:
   RendPubSubEvent(resip::Data evName, resip::Mime contentType)
      : mEventName(evName), mContentType(contentType), 
        mTuplePrefix(resip::Random::getRandomBase64(4)), // should be 6 char long
        mPosture(0),
        mWatching(0),
        mPidfReady(0),
        mNotifyObj(0) 
   {
   }
   ~RendPubSubEvent();

   const resip::Data& getEventName() const { return mEventName; }
   const resip::Mime& getContentType() const { return mContentType; }
   const resip::Data& getTuplePrefix() const { return mTuplePrefix; }

   void setNotifyObj(RendEventNotifyIf *notObj) 
   {
      mNotifyObj = notObj;
   }

   unsigned getPidfSn(unsigned pubAcct, unsigned pubRpt) const;

   void setWatchable(unsigned pubAcctIdx, unsigned pubRptIdx, bool isWatchable);

   unsigned getNextPidfSn(unsigned pubAcct, unsigned pubRpt, 
                          RendTimeUs absPubTime, RendTimeUs absExpTime, 
                          int sessionIdx);

   void setSize(unsigned numAcct, unsigned numPubRpt, unsigned numSubRpt);

   /**
   * Get/Set {subAcctIdx/subAcctRpt} to be watching {pubAcctIdx}.
   */
   bool getWatching(unsigned pubAcctIdx, unsigned subAcctIdx, unsigned subRptIdx);
   void setWatching(RendTimeUs now, 
                    unsigned pubAcctIdx, unsigned subAcctIdx,
                    unsigned subRptIdx, bool isWatching,
                    bool notifyPub = true);
   /**
   * Count number of watchers of {pubAcctIdx}. Note that all repeats
   * of the pub have same number of watchers (since any sub watches
   * all pub repeats.
   */
   unsigned countWatchers(unsigned pubAcctIdx);

   /*
   * Record that a new PUBLISH has occured, and that we expect to 
   * receive new NOTIFYs for all subcriptions to that account.
   */
   void recordPublish(unsigned pubAcct, unsigned pubRpt);

   /*
   * Check to see if all expect NOTIFYs have been received for given
   * publish. Likely need to extend to return more info about
   * which subscriptions are not ready.
   * Returns true if all subscriptions ready to go.
   */
   bool checkPublish(unsigned pubAcct, unsigned pubRpt);

   int checkPublishDetail(RendTimeUs now, unsigned pubAcctIdx, unsigned pubRptIdx, RendTupDetail& detail);

   void recordNotifyTup(RendTimeUs now,
                        unsigned pubAcct, unsigned pubRpt,
                        unsigned subAcct, unsigned subRpt, 
                        unsigned pubPidfSn);

   void recordNotifyMask(RendTimeUs now,
                         unsigned pubAcct, UInt32 pubRptMask,
                         unsigned subAcct, unsigned subRpt);

   void dumpPub(unsigned pubAcctIdx, unsigned pubRptIdx) const;

   unsigned getPendingTuples(unsigned pubAcctIdx, unsigned pubRptIdx);
   unsigned countPendingTuplesCore(unsigned pubAcctIdx, unsigned pubRptIdx) const;
   unsigned countPendingTuples();

   // std::ostream& fmtTupDetail(std::ostream& os, const RendTupDetail& detail);

protected:
   resip::Data mEventName;
   resip::Mime mContentType;
   int mContentMode;

   resip::Data mTuplePrefix;

   unsigned mNumPubAccts;
   unsigned mNumPubRpts;
   unsigned mNumSubAccts;
   unsigned mNumSubRpts;
   RendEventPubPosture* mPosture;
   UInt32* mWatching;
   UInt32* mPidfReady;

   RendEventNotifyIf* mNotifyObj;

   void release();

   RendEventPubPosture* getPosture(unsigned pubAcctIdx, unsigned pubRptIdx) const;

};

RendPubSubEvent::~RendPubSubEvent()
{
   release();
}

void
RendPubSubEvent::release()
{
   if ( mPosture ) 
   {
      delete[] mPosture; mPosture = NULL;
   }
   if ( mWatching ) 
   {
      delete[] mWatching; mWatching = NULL;
   }
   if ( mPidfReady ) 
   {
      delete[] mPidfReady; mPidfReady = NULL;
   }
}

void
RendPubSubEvent::setSize(unsigned numAcct, unsigned numPubRpt, unsigned numSubRpt) 
{
   resip_assert( numAcct >= 1 );
   resip_assert( numPubRpt >= 1 );
   resip_assert( numSubRpt >= 1 );
   resip_assert( numSubRpt < 32 );

   mNumPubAccts = mNumSubAccts = numAcct;
   mNumPubRpts = numPubRpt;
   mNumSubRpts = numSubRpt;

   release();

   unsigned v;
   // posture[pubAcct][pubRpt] = ()
   mPosture = new RendEventPubPosture[mNumPubAccts*mNumPubRpts];
   // watch[pubAcct][subAcct][subRpt] = bool
   v = mNumPubAccts*mNumSubAccts;

   mWatching = new UInt32[v];
   memset(mWatching, 0, sizeof(UInt32)*v);
   // ready[pubAcct][pubRpt][subAcct][subRpt] = bool

   v = mNumPubAccts*mNumPubRpts*mNumSubAccts;
   mPidfReady = new UInt32[v];
   memset(mPidfReady, 0, sizeof(UInt32)*v);
}

inline RendEventPubPosture*
RendPubSubEvent::getPosture(unsigned pubAcctIdx, unsigned pubRptIdx) const
{
   resip_assert( pubAcctIdx < mNumPubAccts );
   resip_assert( pubRptIdx < mNumPubRpts );
   RendEventPubPosture *posture = &mPosture[pubAcctIdx*mNumPubRpts+pubRptIdx];
   return posture;
}

unsigned
RendPubSubEvent::getPidfSn(unsigned pubAcctIdx, unsigned pubRptIdx) const 
{
   resip_assert( pubAcctIdx < mNumPubAccts );
   resip_assert( pubRptIdx < mNumPubRpts );
   RendEventPubPosture *posture = &mPosture[pubAcctIdx*mNumPubRpts+pubRptIdx];
   // TBD: could check to see if expired
   return posture->mExpireAbsSecs==0 ? 0 : posture->mPidfSn;
}

void
RendPubSubEvent::setWatchable(unsigned pubAcctIdx, unsigned pubRptIdx,
                              bool isWatchable) 
{
   RendEventPubPosture *posture = getPosture(pubAcctIdx, pubRptIdx);
   resip_assert(isWatchable==false);
   posture->mWatchable = isWatchable;
   posture->mPendCnt = -1;	// not sure needed
}

unsigned
RendPubSubEvent::getNextPidfSn(unsigned pubAcctIdx, unsigned pubRptIdx, 
                               RendTimeUs absPubTime, RendTimeUs absExpTime, int sessionIdx) 
{
   RendEventPubPosture *posture  = getPosture(pubAcctIdx, pubRptIdx);
   posture->mWatchable = true;
   if ( sessionIdx >= 0 ) 
   {
      posture->mSessionIdx = sessionIdx;	// cb data
      posture->mAbsPubTime = absPubTime;
      if ( absExpTime == 0 ) 
      {
         // DONT zero the pidfSn, need to continue count later
         posture->mExpireAbsSecs = 0;
         return 0;
      }
      posture->mExpireAbsSecs = REND_US2S(absExpTime);
   }
   recordPublish(pubAcctIdx, pubRptIdx);
   return ++(posture->mPidfSn);
}


unsigned
RendPubSubEvent::countWatchers(unsigned pubAcctIdx) 
{
   // watch[pubAcct][subAcct][subRpt] = bool
   resip_assert( pubAcctIdx < mNumPubAccts );
   unsigned watchCnt = 0;
   unsigned subAcctIdx;
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];
   for (subAcctIdx=0; subAcctIdx < mNumSubAccts; subAcctIdx++) 
   {
      watchCnt += rendPopcount(watchBase[subAcctIdx]);
   }
   return watchCnt;
}

bool
RendPubSubEvent::getWatching(unsigned pubAcctIdx, unsigned subAcctIdx,
                             unsigned subRptIdx) 
{
   // watch[pubAcct][subAcct][subRpt] = bool
   resip_assert( pubAcctIdx < mNumPubAccts );
   resip_assert( subAcctIdx < mNumSubAccts );
   resip_assert( subRptIdx < 32 );
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts+subAcctIdx];
   return (*watchBase & (1<<subRptIdx)) ? true : false;
}

void
RendPubSubEvent::setWatching(RendTimeUs now, 
                             unsigned pubAcctIdx, unsigned subAcctIdx,
                             unsigned subRptIdx, bool isWatching, bool notifyPub) 
{
   // watch[pubAcct][subAcct][subRpt] = bool
   resip_assert( pubAcctIdx < mNumPubAccts );
   resip_assert( subAcctIdx < mNumSubAccts );
   resip_assert( subRptIdx < mNumSubRpts );
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts+subAcctIdx];
   UInt32 subMask = 1<<subRptIdx;
   if ( ((*watchBase)&subMask) ^ (isWatching?subMask:0) ) 
   {
      // need to make a change
      *watchBase ^= subMask;
      unsigned pubRptIdx;
      for ( pubRptIdx=0; pubRptIdx < mNumPubRpts; pubRptIdx++) 
      {
         RendEventPubPosture *posture = getPosture(pubAcctIdx, pubRptIdx);
         posture->mPendCnt = -1;
         if ( !isWatching && notifyPub && mNotifyObj && checkPublish(pubAcctIdx, pubRptIdx) ) 
         {
            // can only be ready if we moved watching
            mNotifyObj->handleNotifyDone(now, posture->mSessionIdx,
               posture->mAbsPubTime, /*isNotify*/false);
         }
      }
   }
}

/**
 * Strictly, there is question of what we should do we when unPUBLISH
 * or expire. How do we record our readyness for that? And how to we
 * check that a given PIDF is compatible? For now, punt because
 * we don't subscribe to unPUBLISHed accounts.
 */
void
RendPubSubEvent::recordPublish(unsigned pubAcctIdx, unsigned pubRptIdx) 
{
   RendEventPubPosture *posture = getPosture(pubAcctIdx, pubRptIdx);
   UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];
   unsigned watchCnt = 0;
   unsigned subIdx;
   for (subIdx=0; subIdx < mNumSubAccts; subIdx++) {
      readyBase[subIdx] = 0;
      watchCnt += rendPopcount(watchBase[subIdx]);
   }
   posture->mPendCnt = watchCnt;
}

bool
RendPubSubEvent::checkPublish(unsigned pubAcctIdx, unsigned pubRptIdx) 
{
   // ready[pubAcct][pubRpt][subAcct][subRpt] = bool
   // watch[pubAcct][subAcct][subRpt] = bool
   resip_assert( pubAcctIdx < mNumPubAccts );
   resip_assert( pubRptIdx < mNumPubRpts );
   UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];
   unsigned subIdx;
   for (subIdx=0; subIdx < mNumSubAccts; subIdx++) 
   {
      //InfoLog(<<"PubCheck pub="<<pubAcctIdx<<"."<<pubRptIdx
      //	<<std::hex
      //	<<" watch="<<watchBase[subIdx]
      //	<<" ready="<<readyBase[subIdx]);

      if ( (watchBase[subIdx] & ~readyBase[subIdx]) != 0 ) 
      {
         return false;
      }
   }
   return true;
}



EncodeStream&
RendTupDetail::fmt(EncodeStream& os) const 
{
   os<<"[P."<<mPubAcctIdx<<"."<<mPubRptIdx
      <<" SN="<<mPubPidfSn
      <<" E="<<mPubExpireSecs;
   os<<" #P="<<mPendCnt<<" of #W="<<mWatchCnt;
   os<<" S."<<mSubAcctIdx<<"."<<mSubRptIdx;
   os<<std::hex<<std::setfill('0')
      <<" WM=0x"<<std::setw(4)<<mWatchMask
      <<" RM=0x"<<std::setw(4)<<mReadyMask
      <<" PM=0x"<<std::setw(4)<<mPendMask;
   os<<std::dec<<"]";

   return os;
}

/**
    Returns detailed info about which tuples are missing
**/
int
RendPubSubEvent::checkPublishDetail(RendTimeUs now, 
                                    unsigned pubAcctIdx, unsigned pubRptIdx,
                                    RendTupDetail& detail) 
{
   const RendEventPubPosture *posture = getPosture(pubAcctIdx, pubRptIdx);
   detail.mPubAcctIdx = pubAcctIdx;
   detail.mPubRptIdx = pubRptIdx;
   detail.mPubExpireSecs = posture->mExpireAbsSecs 
      ? posture->mExpireAbsSecs - REND_US2S(now) : 0;
   detail.mPubPidfSn = posture->mPidfSn; 

   UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];
   unsigned watchCnt = 0;
   unsigned pendCnt = 0;
   unsigned subIdx;
   bool isFirst = true;
   for (subIdx=0; subIdx < mNumSubAccts; subIdx++) 
   {
      UInt32 watchMask = watchBase[subIdx];
      watchCnt += rendPopcount(watchMask);
      UInt32 readyMask = readyBase[subIdx];
      UInt32 pendMask = watchMask & ~readyMask;
      if ( pendMask ) 
      {
         unsigned pendRpts = rendPopcount(pendMask);
         pendCnt += pendRpts;
         if ( isFirst ) 
         {
            detail.mSubAcctIdx = subIdx;
            detail.mSubRptIdx = ffsl(pendMask)-1;
            detail.mWatchMask = watchMask;
            detail.mReadyMask = readyMask;
            detail.mPendMask = pendMask;
            isFirst = false;
         }
      }
   }
   detail.mWatchCnt = watchCnt;
   detail.mPendCnt = pendCnt;
   return pendCnt;
}


void
RendPubSubEvent::recordNotifyTup(RendTimeUs now, 
                                 unsigned pubAcctIdx, unsigned pubRptIdx,
                                 unsigned subAcctIdx, unsigned subRptIdx, 
                                 unsigned pubPidfSn) 
{
   RendEventPubPosture *posture = getPosture(pubAcctIdx,pubRptIdx);
   resip_assert( subAcctIdx < mNumSubAccts );
   resip_assert( subRptIdx < mNumSubRpts );
   // TBD: could check to see if expired
   if ( posture->mPidfSn == pubPidfSn && posture->mExpireAbsSecs > 0 ) 
   {
      UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];
      UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
      UInt32 subMask = 1<<subRptIdx;
      if ( (readyBase[subAcctIdx] & subMask)==0 ) 
      {
         /* not previously ready */
         if ( posture->mPendCnt >= 0 && (watchBase[subAcctIdx]&subMask)!=0 )
            posture->mPendCnt -= 1;
         // XXX: above, should really make sure watching?
         readyBase[subAcctIdx] |= subMask;
         if ( mNotifyObj && checkPublish(pubAcctIdx, pubRptIdx) ) 
         {
            mNotifyObj->handleNotifyDone(now, posture->mSessionIdx,
               posture->mAbsPubTime, /*isNotify*/true);
         }
      }
   }
}

/**
    Primary purpose of this is to record tuples that we did NOT get,
    and thus confirm unPUBs.
**/
void
RendPubSubEvent::recordNotifyMask(RendTimeUs now, 
                                  unsigned pubAcctIdx, UInt32 pubRptMask,
                                  unsigned subAcctIdx, unsigned subRptIdx)
{

   UInt32 subRptMask = 1 << subRptIdx;
   unsigned pubRptIdx;
   for ( pubRptIdx=0; pubRptIdx < mNumPubRpts; pubRptIdx++) 
   {
      if ( (pubRptMask & (1<<pubRptIdx))!=0 )
         continue; // got tup in NOTIFY, so unPUB didn't work yet

      RendEventPubPosture *posture = &mPosture[pubAcctIdx*mNumPubRpts+pubRptIdx];
      if ( posture->mExpireAbsSecs > 0 )
         continue; // PUB not expired

      UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
      if ( (readyBase[subAcctIdx] & subRptMask)!=0 )
         continue; // already ready

      readyBase[subAcctIdx] |= subRptMask;
      posture->mPendCnt = -1;
      if ( mNotifyObj && checkPublish(pubAcctIdx, pubRptIdx) )
      {
         mNotifyObj->handleNotifyDone(now, posture->mSessionIdx,
            posture->mAbsPubTime, /*isNotify*/true);
      }
   }
}

void
RendPubSubEvent::dumpPub(unsigned pubAcctIdx, unsigned pubRptIdx) const 
{
   UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];

   ErrLog(<<"EventDb: Dump of "<<pubAcctIdx<<"."<<pubRptIdx<<": ");
   unsigned watchCnt = 0;
   unsigned pendCnt = 0;

   unsigned subIdx;
   for (subIdx=0; subIdx < mNumSubAccts; subIdx++)
   {
      unsigned watchX = rendPopcount(watchBase[subIdx]);
      UInt32 pendBits = (watchBase[subIdx] & ~readyBase[subIdx]);
      unsigned pendX = rendPopcount(pendBits);
      if ( watchBase[subIdx] || readyBase[subIdx] )
      {
         ErrLog(<<"    Sub "<<subIdx<<" of "<<mNumSubAccts<<":"
                <<std::hex<<std::setw(4)<<std::setfill('0')
                <<" W="<<watchBase[subIdx]
                <<" R="<<readyBase[subIdx]
                <<" P="<<pendBits
                <<std::dec
                <<" #w="<<watchX
                <<" #p="<<pendX);
      }
      watchCnt += watchX;
      pendCnt += pendX;
   }
   ErrLog(<<"EventDb: Dump complete:"
          <<" #w="<<watchCnt
          <<" #p="<<pendCnt);
}

/**
Count number of tuples that are pending for give pubAcct/pubRpt
Pending means both watched (we are subscribed) and stale (not ready).
NOTE that this doesn't worry about the pub being expired or not:
caller must handle that.
**/
inline unsigned
RendPubSubEvent::countPendingTuplesCore(unsigned pubAcctIdx, unsigned pubRptIdx) const
{
   unsigned pendCnt = 0;
   UInt32 *readyBase = &mPidfReady[(pubAcctIdx*mNumPubRpts+pubRptIdx)*mNumSubAccts];
   UInt32 *watchBase = &mWatching[pubAcctIdx*mNumSubAccts];

   unsigned subIdx;
   for (subIdx=0; subIdx < mNumSubAccts; subIdx++)
   {
      UInt32 pendBits = (watchBase[subIdx] & ~readyBase[subIdx]);
      unsigned pendX = rendPopcount(pendBits);
      pendCnt += pendX;
   }
   return pendCnt;
}

unsigned
RendPubSubEvent::getPendingTuples(unsigned pubAcctIdx, unsigned pubRptIdx)
{
   // TBD: asserts on the args
   RendEventPubPosture *posture = &mPosture[pubAcctIdx*mNumPubRpts+pubRptIdx];
   if ( posture->mPendCnt < 0 )
      posture->mPendCnt = countPendingTuplesCore(pubAcctIdx, pubRptIdx);

   return posture->mPendCnt;
}

/**
Count number of tuples that are pending. This means the tuple
is both watched (we are subscribed) and stale (not ready).
**/
unsigned
RendPubSubEvent::countPendingTuples()
{
   unsigned pendCnt = 0;
   // ready[pubAcct][pubRpt][subAcct][subRpt] = bool
   // watch[pubAcct][subAcct][subRpt] = bool
   unsigned pubAcctIdx;
   for ( pubAcctIdx=0; pubAcctIdx < mNumPubAccts; pubAcctIdx++)
   {
      unsigned pubRptIdx;
      for ( pubRptIdx=0; pubRptIdx < mNumPubRpts; pubRptIdx++)
      {
         RendEventPubPosture *posture = &mPosture[pubAcctIdx*mNumPubRpts+pubRptIdx];
         
         if ( ! posture->mWatchable )
            continue;
         
         if ( posture->mExpireAbsSecs==0 )
            continue;

         int cachePend = posture->mPendCnt;
         if ( cachePend < 0 )
         {
            posture->mPendCnt = countPendingTuplesCore(pubAcctIdx, pubRptIdx);
         }
#if 0		// we haven't had issues for while, so probably ok now
         if ( cachePend >= 0 && cachePend != posture->mPendCnt )
         {
            ErrLog(<<"EventDb Consistency Error: pending count mismatch: "
                   <<" cachePend="<<cachePend
                   <<" countPend="<<posture->mPendCnt);
            dumpPub(pubAcctIdx, pubRptIdx);
            resip_assert(0);
         }
#endif
         pendCnt += posture->mPendCnt;
      }
   }
   return pendCnt;
}

/********************************************************************
*
* PUBLISH-specific classes
*
********************************************************************/
class RendPubDlg;

class RendPubTroop : public RendTroopBase, public RendEventNotifyIf
{
public:
   RendPubTroop(RendTimeUs now, RendTu& tu, RendCntMgr& cntMgr, RendPubSubEvent& evInfo)
      : RendTroopBase(now, tu, REND_DlgCat_Pub, cntMgr), mEventInfo(evInfo) 
   {}
   virtual RendTroopDlg*createNewDlg(const RendDlgAcctKey& key);
   virtual int startDlgCmd(RendTimeUs now, RendSession& sess,
                           RendTroopDlg *dlg, int expSecs, 
                           RendPendReason pendReason);
   virtual void handleNotifyDone(RendTimeUs now, unsigned sessionIdx,
                                 RendTimeUs absPubTime, bool isNotify);
   virtual RendSessionMood checkGoalMood(RendHandleMoodCxt& cxt);

   RendPubDlg* getPubDlg(const RendSession& sess) const;

   void updateWorkStats(unsigned subPerPub, bool clearB);

   RendPubSubEvent& mEventInfo;

   // Number of tuples in the wave queue, 
   // doesn't include PendReq or PendNot queues
   // int mWaveCurTupRemCnt;
   unsigned mPendTupCurCnt;
   unsigned mPendTupMostCnt;
   unsigned mRemainTupCurCnt;
   unsigned mRemainTupTotCnt;
};


class RendPubDlg : public RendTroopDlg 
{
public:
   RendPubDlg(RendPubTroop& troop, RendTu& tu, RendAcctIdx acctIdx, int rptIdx) 
      : RendTroopDlg(tu, acctIdx, acctIdx), mTroop(troop), mRepeatIdx(rptIdx)
   { 
      resip_assert(acctIdx>=0); 
      mFlags |= REND_DlgF_ETagDlg;
   }
   virtual RendTroopBase& getTroop() { return mTroop; }
   virtual resip::SipMessage* makeNextReq();
   virtual RendSessionMood getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood);

   RendPubTroop& mTroop;
   int mRepeatIdx;
   int mPidfSn;

protected:
};

inline RendPubDlg*
RendPubTroop::getPubDlg(const RendSession& sess) const 
{
   return static_cast<RendPubDlg*>(sess.getDlg());
}


RendSessionMood
RendPubTroop::checkGoalMood(RendHandleMoodCxt& cxt)
{
   RendSession& sess = *cxt.mSess;

   resip_assert(mOpenToIdxBase >= 0 && mOpenToIdxLen >= 0);
   int ndlgs = mToSlice.getDlgLen();
   resip_assert( mOpenToIdxBase < ndlgs );
   int idx = (cxt.mKey.mToIdx - mOpenToIdxBase + ndlgs) % ndlgs;
   bool doPub = idx >= 0 && idx < mOpenToIdxLen;

   if ( doPub && sess.mMood==REND_SM_Idle )
   {
      // below increments SN so that we know stale even though
      // we haven't sent PUB yet (so we don't mark ready prematurely)
      // mEventInfo.getNextPidfSn(key.mToIdx, key.mRepeatIdx, now, 0, -1);
      // mWaveCurTupRemCnt += mEventInfo.countWatchers(key.mToIdx);
      cxt.mNewPendReason = REND_PR_Open;
      return REND_SM_Wave;
   }

   if ( doPub && sess.mMood==REND_SM_Open )
   {
      // see comment above
      //mEventInfo.getNextPidfSn(key.mToIdx, key.mRepeatIdx, now, 0, -1);
      // mWaveCurTupRemCnt += mEventInfo.countWatchers(key.mToIdx);
      cxt.mNewPendReason = REND_PR_Modify;
      return REND_SM_Wave;
   }
#if 0
   // this is unreached unless we modify above to be more selective
   if ( doPub && sess.mMood==REND_SM_Open 
       && sess.mRenewAbsTime!=0 && now > sess.mRenewAbsTime ) 
   {
      pr = REND_PR_Renew;
      return REND_SM_Wave;
   }
#endif
   if ( doPub ) 
   {
      WarningLog(<<"Want to pub, but in strange mood="<<sess.mMood);
   }
   if ( !doPub && sess.mMood!=REND_SM_Idle ) 
   {
      // I don't think we need to bump the PIDF SN, since we're 
      // really looking for absense, not the SN. When we send
      // the actual unPUB we'll update the DB entry
      cxt.mNewPendReason = REND_PR_Close;
      return REND_SM_Wave;
   }
   return REND_SM_None;
}

void
RendPubTroop::updateWorkStats(unsigned subPerPub, bool clearB) 
{
   unsigned pubWave = getSessionMoodCnt(REND_SM_Wave);
   if ( clearB ) 
   {
      mPendTupCurCnt = mPendTupMostCnt = 0;
      mRemainTupTotCnt = pubWave * subPerPub;
   } 
   else 
   {
      int pubReq = getSessionMoodCnt(REND_SM_PendReq);
      int pubNot = getSessionMoodCnt(REND_SM_PendNotify);
      mPendTupCurCnt = (pubReq+pubNot==0) ? 0 : mEventInfo.countPendingTuples();

      if ( mPendTupCurCnt > mPendTupMostCnt )
         mPendTupMostCnt = mPendTupCurCnt;
   }
   mRemainTupCurCnt = pubWave * subPerPub + mPendTupCurCnt;

   // WATCHOUT: if we ever count pending tuples in EventDb, need
   // to be careful because we may have skipped/failed some PUBs
   // (and thus aren't waiting anymore) but we don't current record
   // that into the DB (yet!).
   RendTroopBase::updateWorkStats(clearB);
}


RendTroopDlg*
RendPubTroop::createNewDlg(const RendDlgAcctKey& key)
{
   // what about key.mRepeatIdx?
   return new RendPubDlg(*this, mTu, key.mToIdx, key.mRepeatIdx);
}

int
RendPubTroop::startDlgCmd(RendTimeUs now, RendSession& sess,
                          RendTroopDlg *dlg, int expSecs, RendPendReason pendReason) 
{
   RendPubDlg *pdlg = static_cast<RendPubDlg*>(dlg);
   if ( pendReason==REND_PR_Renew )
   {
      pdlg->mPidfSn = 0;
   } 
   else 
   {
      //InfoLog(<<"PubStartCmd pub="<<pdlg->mToAcctIdx<<"."<<pdlg->mRepeatIdx
      //		<<" exp="<<expSecs);
      RendTimeUs expTime = expSecs<=0 ? 0 : now+REND_S2US(expSecs);
      pdlg->mPidfSn = mEventInfo.getNextPidfSn(pdlg->mToAcctIdx, 
         pdlg->mRepeatIdx, now, expTime,
         pdlg->mTroopSessionIdx);
   }
   return RendTroopBase::startDlgCmd(now, sess, dlg, expSecs, pendReason);
}

/**
Would be "cleaner" if EventInfo provided us acctIdx&rptIdx (rather than
sessionIdx) but right now the mapping {acctKey}->{sessionIdx} is not
easily (or safely) inverted. Look into cleaning this up later.
For now, think of {sessionIdx} as callback data.
**/
void
RendPubTroop::handleNotifyDone(RendTimeUs now, unsigned sessionIdx,
                               RendTimeUs absPubTime, bool isNotify) 
{
   RendSession* sess = getSession(sessionIdx);
   resip_assert( sess );

   if ( sess->mMood != REND_SM_PendNotify )
      return; // we don't care any right now

   if ( isNotify )
      addMoodDur(sess->mMood, now - absPubTime);

   setSessionMood(now, *sess, REND_SM_GROUP_ByPendReason);
}

resip::SipMessage*
RendPubDlg::makeNextReq()
{
   resip::SipMessage *req = makeRequest(resip::PUBLISH);
   req->header(resip::h_Event).value() = mTroop.mEventInfo.getEventName();
   // req->header(h_Accepts).push_back( resip::Mime( "application", "pidf+xml") );

   if ( mPidfSn > 0 ) 
   {
      resip::Data entityAor = getAcctMgr().getAorNoPort(mFromAcctIdx);
      const resip::Data& tupPre = mTroop.mEventInfo.getTuplePrefix();
      const char *basicStatus = "open";
      char noteBuf[100];
#ifdef WIN32
      _snprintf(noteBuf, sizeof(noteBuf), "%010u.%05u.%010u", 
         mToAcctIdx+1, mRepeatIdx+1, mPidfSn+1);
#else
      snprintf(noteBuf, sizeof(noteBuf), "%010u.%05u.%010u", 
         mToAcctIdx+1, mRepeatIdx+1, mPidfSn+1);
#endif
      resip_assert( strlen(noteBuf)==27 );
      resip::Data body;
      {
         resip::DataStream strm(body);
         strm<<"<?xml version=\"1.0\"?>\r\n"
            << "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" entity=\"" 
            << entityAor << "\" >\r\n" 
            << "<tuple id=\""<<tupPre<<"."<<mRepeatIdx<<"\" >"
            << "<status><basic>" <<basicStatus<<"</basic></status>\r\n"
            << "<note>RENDNOTE."<<tupPre<<"."<<noteBuf<<".rendnote"<<"</note>"
            << "</tuple> </presence>\r\n";
      }

      resip::HeaderFieldValue bodyHfv(body.data(), body.size());
      req->setRawBody(bodyHfv);
      req->header(resip::h_ContentType) = mTroop.mEventInfo.getContentType();

#if 0
      resip::Pidf pidf;
      resip::Uri entity = getAcctMgr().getUriAor(mToAcctIdx);
      pidf.setEntity(entity);
      pidf.setSimpleId(mPidfSimple.mId);
      pidf.setSimpleStatus(mPidfSimple.mStatus, mPidfSimple.mNote);
      req->setContents(&pidf);
#endif
   }

   return req;
}


/**
Checking for new or modified PUBLISH is relatively strait-forward:
the pidf SN for that tuple is incremented, and we look for that in
the NOTIFY message. 
Checking for removed PUBLISH is harder, since we're
really looking for absense of info. I think for now we just look
to see that we got any message at all.
**/
RendSessionMood
RendPubDlg::getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood)
{
   resip_assert( sess.mMood == REND_SM_PendReq );
   resip_assert( sess.getPendReason() != 0 );
   if ( isGood ) 
   {
      if ( ! mTroop.mEventInfo.checkPublish(mToAcctIdx, mRepeatIdx) ) 
      {
         return REND_SM_PendNotify;
      }
      if ( sess.getPendReason()!=REND_PR_Close ) 
      {
         return REND_SM_Open;
      }
   }
   // If good close or any failure, unset watchable so we
   // don't wait for notifies.
   mTroop.mEventInfo.setWatchable(mToAcctIdx, mRepeatIdx, /*isWatchable*/false);
   return REND_SM_Recycle;
}


/********************************************************************
*
* SUBSCRIBE-specific classes
* (also handles receiving NOTIFY requests)
*
********************************************************************/

class RendSubDlg;

class RendSubTroop : public RendTroopBase
{
public:
   RendSubTroop(RendTimeUs now, RendTu& tu, RendCntMgr &cntMgr, RendPubSubEvent& evInfo) 
      : RendTroopBase(now, tu, REND_DlgCat_Sub, cntMgr),
        mEventInfo(evInfo) 
   {
      // disable level-seeking logic
      mTgtOpenDlgs = -1;
   }
   virtual RendTroopDlg* createNewDlg(const RendDlgAcctKey& key);
   virtual RendSessionMood checkGoalMood(RendHandleMoodCxt& cxt);

   RendSubDlg* getSubDlg(const RendSession& sess) const;

   RendPubSubEvent& mEventInfo;
};


class RendSubDlg : public RendTroopDlg 
{
public:
   RendSubDlg(RendSubTroop& troop, RendTu& tu, 
              RendAcctIdx fromIdx, RendAcctIdx toIdx,
              int rptIdx) 
      : RendTroopDlg(tu, fromIdx, toIdx), mTroop(troop), mRepeatIdx(rptIdx)
   { 
      resip_assert(toIdx>=0); 	// why is this here
      mFlags |= REND_DlgF_StdDlg|REND_DlgF_KeepAlive;
      mGotNotify = false;
      mTryRenew = false;
   }
   virtual RendTroopBase& getTroop() { return mTroop; }
   virtual resip::SipMessage* makeNextReq();

   virtual RendSessionMood getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood);

   virtual int handleRequest(RendReqCxt& cxt);
   int handleNotifyReq(RendReqCxt& cxt, RendSession& ses);

   void handleNotifyTuple(RendReqCxt& cxt, unsigned pubAcctIdx, unsigned pubRptIdx, unsigned pubPidfSn);

   RendSubTroop& mTroop;
   int mRepeatIdx;
   bool mGotNotify;
   bool mTryRenew;

protected:
};

inline RendSubDlg*
RendSubTroop::getSubDlg(const RendSession& sess) const 
{
   return static_cast<RendSubDlg*>(sess.getDlg());
}

/**
Callback for adjustAllDlgs(), call one at a time for every possible
session.
**/
RendSessionMood
RendSubTroop::checkGoalMood(RendHandleMoodCxt& cxt)
{
   RendSession& sess= *cxt.mSess;
   bool doWatch = mEventInfo.getWatching(/*pub*/cxt.mKey.mToIdx, 
      /*sub*/cxt.mKey.mFromIdx, cxt.mKey.mRepeatIdx);
   //InfoLog(<<"SubCheckGoal pub="<<key.mToIdx
   //	    <<" sub="<<key.mFromIdx<<"."<<key.mRepeatIdx
   //	    <<" watch="<<doWatch);
   // return doWatch ? REND_SM_Open : REND_SM_Idle;

   if ( doWatch && sess.mMood==REND_SM_Idle )
   {
      cxt.mNewPendReason = REND_PR_Open;
      return REND_SM_Wave;
   }
   if ( doWatch && sess.mMood==REND_SM_Open )
   {
      // renew up to 1min early so that expire tracker doesn't kick in
      if ( sess.mRenewAbsTime!=0 && cxt.mNow+60 > sess.mRenewAbsTime )
      {
         cxt.mNewPendReason = REND_PR_Renew;
         return REND_SM_Wave;
      }
      RendSubDlg *sdlg = getSubDlg(sess);
      resip_assert(sdlg);
      if ( sdlg->mTryRenew )
      {
         cxt.mNewPendReason = REND_PR_Renew;
         return REND_SM_Wave;
      }
   }
   if ( !doWatch && sess.mMood!=REND_SM_Idle )
   {
      cxt.mNewPendReason = REND_PR_Close;
      return REND_SM_Wave;
   }
   return REND_SM_None;
}

RendTroopDlg*
RendSubTroop::createNewDlg(const RendDlgAcctKey& key) 
{
   // what about key.mRepeatIdx?
   return new RendSubDlg(*this, mTu, key.mFromIdx, key.mToIdx, key.mRepeatIdx);
}


resip::SipMessage*
RendSubDlg::makeNextReq()
{
   resip::SipMessage *req = makeRequest(resip::SUBSCRIBE);
   req->header(resip::h_Event).value() = mTroop.mEventInfo.getEventName();
   // req->header(h_Accepts).push_back( resip::Mime( "application", "pidf+xml") );

   req->header(resip::h_ContentType) = mTroop.mEventInfo.getContentType();

   mGotNotify = false;
   mTryRenew = false;	// could do on response, but easier here

   return req;
}


RendSessionMood
RendSubDlg::getPostRspMood(RendTimeUs now, RendSession& sess, bool isGood)
{
   if ( isGood )
   {
      resip_assert( sess.mMood == REND_SM_PendReq );
      if ( sess.getPendReason()==REND_PR_Close ) 
      {
         // unset watchable so PUBs don't wait for NOTIFYs
         mTroop.mEventInfo.setWatching(now, /*pub*/mFromAcctIdx, /*sub*/mToAcctIdx, mRepeatIdx, /*doWatching*/false);
      }
      if ( ! mGotNotify )
         return REND_SM_PendNotify;

      return sess.getPendReason()==REND_PR_Close ? REND_SM_Recycle : REND_SM_Open;
   }

   mTroop.mEventInfo.setWatching(now, /*pub*/mFromAcctIdx, /*sub*/mToAcctIdx, mRepeatIdx, /*doWatching*/false);
   return REND_SM_Recycle;
}

void
RendSubDlg::handleNotifyTuple(RendReqCxt& cxt,
                              unsigned pubAcctIdx, unsigned pubRptIdx, 
                              unsigned pubPidfSn) 
{
   if ( pubAcctIdx != (unsigned)mToAcctIdx )
   {
      WarningLog(<<"Got wrong pub acct index.");
      return;
   }

   mTroop.mEventInfo.recordNotifyTup(cxt.mRxTimeUs,
      pubAcctIdx, pubRptIdx, mFromAcctIdx, mRepeatIdx, pubPidfSn);
}

int
RendSubDlg::handleNotifyReq(RendReqCxt& cxt, RendSession& sess)
{
   RendTimeUs now = cxt.mRxTimeUs;
   if ( cxt.mOutOfOrder )
      return 200;

   if ( ! cxt.mMsg->exists(resip::h_SubscriptionState) ) 
   {
      ErrLog(<<"NOTIFY missing subscription state");
      return 400;
   }
   const resip::Token& ss = cxt.mMsg->header(resip::h_SubscriptionState);
   const resip::Data& sss = ss.value(); // active, pending, terminated
   const resip::HeaderFieldValue& bodyHfv = cxt.mMsg->getRawBody();
   bool isPend = sess.isPendish();

   mGotNotify = true;

   if ( sss==resip::Symbols::Terminated ) 
   {
      const resip::Data& ssr = ss.param(resip::p_reason);
      // deactiviated, probation, rejected, timeout, giveup, noresource
      if ( isPend && sess.getPendReason()==REND_PR_Close )
      {
         // this is normal unSUB, already setWatchable false
         // NOTE: If SUB expired, then maybe no body, but if
         // unSUB, should have body (to support polling). We dont
         // process body in either case
         if ( sess.mMood==REND_SM_PendNotify )
         {
            mTroop.setSessionMood(cxt.mRxTimeUs, sess, REND_SM_Recycle,
               REND_PR_NotifyTermClose);
         }
         // else wait for response
         return 200;
      }
      RendSessionFmtr subFmtr(now, sess);
      ErrLog(<<"Got NOTIFY terminated reason="<<ssr
             <<" sub=["
             <<" f="<<mFromAcctIdx
             <<" t="<<mToAcctIdx<<"."<<mRepeatIdx
             <<" sess="<<subFmtr
             <<"]");

      // Could try sending reSUB or unSUB, but seems best just to drop it
      mTroop.mEventInfo.setWatching(now, /*pub*/mFromAcctIdx, /*sub*/mToAcctIdx, mRepeatIdx, /*doWatching*/false);
      mTroop.mCntMgr.inc( REND_CntCode_NotifyTerm, REND_DlgCat_Sub);
      mTroop.setSessionMood(cxt.mRxTimeUs, sess, REND_SM_Recycle, REND_PR_NotifyTermSuprise);
      return 200;
   }
   if ( bodyHfv.getLength()==0 ) 
   {
      WarningLog(<<"NOTIFY missing content");
      return 200;
   }
   if ( isPend ) 
   {
      // server could send us multiple NOTIFYs (e.g., while we are 
      // waiting for response). We count them all.
      mTroop.addMoodDur(REND_SM_PendNotify, cxt.mRxTimeUs - mReqSendTime);
   }
   //
   // do we want to check it at all?

   resip::Data tag("RENDNOTE");
   resip::Data body;
   bodyHfv.toShareData(body);
   const resip::Data& tupPre = mTroop.mEventInfo.getTuplePrefix();
   unsigned ofs = 0;
   const unsigned siglen = 8+1+tupPre.size()+1+10+1+5+1+10+1+8;
   UInt32 gotPubMask = 0;
   while ( (ofs = body.find(tag, ofs)) != resip::Data::npos )
   {
      // RENDNOTE.tupPrefix.pubAcctIdx.pubRptIdx.pidfSn.rendnote
      // 8       1   ?     1    10    1   5     1  10  1   8
      unsigned long pubAcctIdx, pubAcctRpt, pubPidfSn;
      unsigned signext = ofs + siglen;

      if ( signext > body.size() )
         break; // not error, just false positive

      ofs += 8+1; // skip lead tag

      if ( memcmp(body.data()+ofs, tupPre.data(), tupPre.size())!=0 )
         continue; // not error, probably stale

      ofs += tupPre.size()+1; // skip prefix tag
      pubAcctIdx = strtoul(body.data()+ofs, NULL, 10);
      ofs += 10+1; // skip acct
      pubAcctRpt = strtoul(body.data()+ofs, NULL, 10);
      ofs += 5+1; // skip rpt
      pubPidfSn = strtoul(body.data()+ofs, NULL, 10);
      ofs += 10+1+8; // skip SN & tail tag
      resip_assert( signext == ofs );

      if ( pubAcctIdx==0 || pubAcctRpt==0 || pubPidfSn==0 ) 
      {
         WarningLog(<<"NOTIFY malform PIDF note");
         continue;
      }
      handleNotifyTuple(cxt, pubAcctIdx-1, pubAcctRpt-1, pubPidfSn-1);
      gotPubMask |= 1<<(pubAcctRpt-1);
   }
   mTroop.mEventInfo.recordNotifyMask(cxt.mRxTimeUs, mToAcctIdx, gotPubMask, mFromAcctIdx, mRepeatIdx);

   if ( sess.mMood==REND_SM_PendNotify ) 
   {
      mTroop.setSessionMood(cxt.mRxTimeUs, sess, REND_SM_GROUP_ByPendReason);
   }

   return 200;
}


/**
Process in-dialog request from the wire.
**/
int
RendSubDlg::handleRequest(RendReqCxt& cxt)
{
   RendSession *sess = mTroop.getSession(mTroopSessionIdx);
   if ( cxt.mMethod == resip::NOTIFY )
   {
      if ( sess==NULL )
         return 481;
      return handleNotifyReq(cxt, *sess);
   }
   return 405;	// method not allowed
}

/********************************************************************
*
* sketch specific classes
*
********************************************************************/

#define REND_SKETCH_WS_LOOP_START 101

#define REND_SKETCH_WS_SUB_START 110
#define REND_SKETCH_WS_SUB_1     112
#define REND_SKETCH_WS_SUB_2     113
#define REND_SKETCH_WS_SUB_3     114
#define REND_SKETCH_WS_SUB_4     115
#define REND_SKETCH_WS_SUB_5     116
#define REND_SKETCH_WS_SUB_DONE  119

#define REND_SKETCH_WS_PUB_START 120
#define REND_SKETCH_WS_PUB_1     121
#define REND_SKETCH_WS_PUB_2     122
#define REND_SKETCH_WS_PUB_3     123
#define REND_SKETCH_WS_PUB_4     124
#define REND_SKETCH_WS_PUB_5     125
#define REND_SKETCH_WS_PUB_DONE  129

#define	REND_SKETCH_WS_LOOP_PAUSE 131

class RendPres1Sketch : public RendSketchBase
{
public:
   RendPres1Sketch(RendTu& tu, RendCntMgr& cntMgr, RendPubSubEvent& evInfo, RendPubTroop& pubTr, RendSubTroop& subTr) 
      : RendSketchBase(tu, cntMgr), mEventInfo(evInfo), mPubTroop(pubTr), mSubTroop(subTr) 
   {
      mWorkState = REND_SKETCH_WS_LOOP_START;
      mNextWorkState = 0;
      mTroopList.push_back(&pubTr);
      mTroopList.push_back(&subTr);
      mWaveCnt = 0;
      mChunkCnt = 0;
      mMaxSubPerPub = 0;
      mBigDurStart = 0;
      mBigDurWhich = 0;
      mWaveLastPubDur = 0;
      mWaveLastSubDur = 0;

      mCurWaveReqTotCnt = 0;
      mCurWaveTupTotCnt = 0;
   }
   virtual ~RendPres1Sketch();

   // virtual const char* getName() const { return "Pres1"; }
   // virtual void setWorkVolume(RendWorkVolume& vol);
   // virtual void cleanup();

   virtual void setWorkLevel(int level);
   // int doTroopWork(RendTimeUs now, int minWork, int maxWork);
   virtual int doWorkChunk(RendTimeUs now, int minWork, int maxWork);
   virtual void printStatus(const RendTroopReport& rpt);
   virtual void getTroopReportCore(RendTimeUs now, RendTroopReport& rpt);
   virtual void getPendCnts(int& pendReqCnt, int& pendNotCnt) const;
   virtual void checkStale(RendTimeUs now);

   void setSubShape(RendTimeUs now);

   RendSessionMood handleTooOld3(RendHandleMoodCxt& cxt);

   /**
   MaxSubPerPub is an app-configured limit. Set zero to allow full
   matrix.
   **/
   int mMaxSubPerPub;

   RendPubSubEvent& mEventInfo;
   RendPubTroop& mPubTroop;
   RendSubTroop& mSubTroop;

   int mWorkState;
   int mNextWorkState;
   int mWaveCnt;
   int mChunkCnt;
   RendTimeUs mWaveStateStartTime;

   RendTimeUs *mBigDurWhich;
   RendTimeUs mBigDurStart;
   RendTimeUs mWaveLastPubDur;
   RendTimeUs mWaveLastSubDur;

   int mCurWaveReqTotCnt;
   int mCurWaveTupTotCnt;

   /**
   MostSubPerPub is the current actual limit. It should never
   be larger than the Max value, but might be smaller.
   **/
   int mMostSubPerPub;
   // int mMostPubPendReq;
   // int mMostPubPendNot;
   // int mMostSubPendReq;
   // int mMostSubPendNot;
};

RendPres1Sketch::~RendPres1Sketch() 
{
   InfoLog(<<"Destructing Sketch Pres1...");
   deleteTroops();
   // mPubTroop and mSubTroop are now bad refs
   delete	&mEventInfo;
   InfoLog(<<"Destructed Sketch Pres1.");
}

// NOTE that below is internal function only; not mutex protected
void
RendPres1Sketch::setWorkLevel(int level) 
{
   mPubTroop.setOpenToIdxRange(0, level>=0 ? level : 0);
}

void
RendPres1Sketch::printStatus(const RendTroopReport& rpt) 
{
   int totgood = rpt.mOpenGoodCnt + rpt.mRenewGoodCnt + rpt.mCloseGoodCnt;
   int totfail = rpt.mOpenFailCnt + rpt.mRenewFailCnt + rpt.mCloseFailCnt;
   printf("tick wave=%d rs=%d ws=%d idle=%d open=%d preq=%d pnot=%d wave=%d totgood=%d totfail=%d\n",
      mWaveCnt,
      mRunState, mWorkState,
      rpt.mMoodIdleCnt,
      rpt.mMoodOpenCnt,
      rpt.mMoodPendReqCnt,
      rpt.mMoodPendNotifyCnt,
      rpt.mMoodWaveCnt,
      totgood, totfail);
}

void
RendPres1Sketch::getTroopReportCore(RendTimeUs now, RendTroopReport& rpt) 
{
   RendSketchBase::getTroopReportCore(now, rpt);
   rpt.mWaveCnt = mWaveCnt;
   rpt.mWaveStateSecs = REND_US2S(now - mWaveStateStartTime);

   const char *nm = "?";

   rpt.mWaveState = mWorkState;
   switch ( mWorkState ) 
   {
   case REND_SKETCH_WS_LOOP_START: nm = "top";	break;
   case REND_SKETCH_WS_SUB_START:  nm = "sub0";	break;
   case REND_SKETCH_WS_SUB_1:      nm = "sub1";	break;
   case REND_SKETCH_WS_SUB_2:      nm = "sub2";	break;
   case REND_SKETCH_WS_SUB_3:      nm = "sub3";	break;
   case REND_SKETCH_WS_SUB_4:      nm = "sub4";	break;
   case REND_SKETCH_WS_SUB_5:      nm = "sub5";	break;
   case REND_SKETCH_WS_SUB_DONE:   nm = "subX";	break;
   case REND_SKETCH_WS_PUB_START:  nm = "pub0";	break;
   case REND_SKETCH_WS_PUB_1:      nm = "pub1";	break;
   case REND_SKETCH_WS_PUB_2:      nm = "pub2";	break;
   case REND_SKETCH_WS_PUB_3:      nm = "pub3";	break;
   case REND_SKETCH_WS_PUB_4:      nm = "pub4";	break;
   case REND_SKETCH_WS_PUB_5:      nm = "pub5";	break;
   case REND_SKETCH_WS_PUB_DONE:   nm = "pubX";	break;
   case REND_SKETCH_WS_LOOP_PAUSE: nm = "pause";	break;
   default: 
      CritLog(<<"Bogus work state="<<mWorkState);
      resip_assert(0);
   }
   rpt.mWaveStateStr = nm;

   rpt.mWavePubAliveCnt = mPubTroop.getSessionMoodCnt(REND_SM_GROUP_Alive);
   rpt.mWaveSubAliveCnt = mSubTroop.getSessionMoodCnt(REND_SM_GROUP_Alive);

   rpt.mWaveLastPubDur = REND_US2S(mWaveLastPubDur);
   rpt.mWaveLastSubDur = REND_US2S(mWaveLastSubDur);

   rpt.mWaveCurReqTotCnt = mCurWaveReqTotCnt;
   rpt.mWaveCurReqRemCnt = 
      mPubTroop.getSessionMoodCnt(REND_SM_Wave)
      + mPubTroop.getSessionMoodCnt(REND_SM_PendReq)
      + mSubTroop.getSessionMoodCnt(REND_SM_Wave)
      + mSubTroop.getSessionMoodCnt(REND_SM_PendReq);

   rpt.mWaveCurTupTotCnt = mCurWaveTupTotCnt;

   if ( mWorkState==REND_SKETCH_WS_PUB_3 ) 
   {
      rpt.mWaveCurTupRemCnt = mPubTroop.mRemainTupCurCnt;
   } 
   else 
   {
      rpt.mWaveCurTupRemCnt = rpt.mWaveCurReqRemCnt + mSubTroop.getSessionMoodCnt(REND_SM_PendNotify);
   }

   rpt.mWaveCurTupPendCnt = mPubTroop.mPendTupCurCnt;
   // add sub pend tups into above?

   rpt.mWavePubReqAvgDur = REND_US2MS(mPubTroop.getMoodDur(REND_SM_PendReq).avg());
   rpt.mWavePubReqMaxDur = REND_US2MS(mPubTroop.getMoodDur(REND_SM_PendReq).mMax);
   rpt.mWavePubNotAvgDur = REND_US2MS(mPubTroop.getMoodDur(REND_SM_PendNotify).avg());
   rpt.mWavePubNotMaxDur = REND_US2MS(mPubTroop.getMoodDur(REND_SM_PendNotify).mMax);

   rpt.mWaveSubReqAvgDur = REND_US2MS(mSubTroop.getMoodDur(REND_SM_PendReq).avg());
   rpt.mWaveSubReqMaxDur = REND_US2MS(mSubTroop.getMoodDur(REND_SM_PendReq).mMax);
   rpt.mWaveSubNotAvgDur = REND_US2MS(mSubTroop.getMoodDur(REND_SM_PendNotify).avg());
   rpt.mWaveSubNotMaxDur = REND_US2MS(mSubTroop.getMoodDur(REND_SM_PendNotify).mMax);
}


/**
We interpret the work level as the number of active PUB accounts. 
Repeats are always active.

Notes:
1. See RendSubTroop::checkGoalMood() for how the decisions made here
get turned into actions.
2. Logic below isn't ideal wrt SUBs because leads to unneeded thrashing.
E.g., it always starts the matrix at the first start rather than
worrying only about the number of SUBs. Advantage is that it is
more deterministic.
3. Below could be optmized to use vector assignments, especially when
disabling SUBs.
**/
void
RendPres1Sketch::setSubShape(RendTimeUs now)
{
   unsigned pubIdx, subIdx, rptIdx;
   unsigned totSubCnt = 0;
   unsigned maxPerCnt = 0;
   for (pubIdx=0; pubIdx < mSubTroop.getNumToAccts(); pubIdx++)
   {
      // just look at first PUB repeat
      unsigned pidfSn = mEventInfo.getPidfSn(pubIdx, mPubTroop.getRepeatBase());
      unsigned subPerPubCnt = 0;
      for (subIdx=0; subIdx < mSubTroop.getNumFromAccts(); subIdx++) 
      {
         for (rptIdx=mSubTroop.getRepeatBase(); 
            rptIdx < mSubTroop.getRepeatMax(); rptIdx++) 
         {
            bool doWatch = pidfSn!=0;

            if ( mMaxSubPerPub > 0 && subPerPubCnt >= (unsigned)mMaxSubPerPub )
               doWatch = false;

            if ( mWorkVol.mWorkLevel >= 0 && subIdx >= (unsigned)mWorkVol.mWorkLevel )
               doWatch = false;

            //InfoLog(<<"SetSubShape pub="<<pubIdx
            // <<" sub="<<subIdx<<"."<<rptIdx
            // <<" watch="<<doWatch);
            mEventInfo.setWatching(now, pubIdx, subIdx, rptIdx, doWatch, /*notifyPub*/false);

            if ( doWatch ) 
            {
               ++subPerPubCnt;
               ++totSubCnt;
            }
         }
      }

      if ( subPerPubCnt > maxPerCnt )
         maxPerCnt = subPerPubCnt;
   }
   mMostSubPerPub = maxPerCnt;
   InfoLog(<<"SetSubShape done:"
           <<" workVol="<<mWorkVol.mWorkLevel
           <<" totSubs="<<totSubCnt
           <<" maxSubPerPubCnt="<<maxPerCnt);
}

void
RendPres1Sketch::getPendCnts(int& pendReqCnt, int& pendNotCnt) const 
{
   int pubReq = mPubTroop.getSessionMoodCnt(REND_SM_PendReq);
   int subReq = mSubTroop.getSessionMoodCnt(REND_SM_PendReq);
   pendReqCnt = pubReq + subReq;
   //int pubNot = mPubTroop.getSessionMoodCnt(REND_SM_PendNotify);
   // below is expensive, should only called if received stuff
   // since last call
   mPubTroop.updateWorkStats(mMostSubPerPub, /*clear*/false);
   int subNot = mSubTroop.getSessionMoodCnt(REND_SM_PendNotify);

   // WATCHOUT: if we ever count pending tuples in EventDb, need
   // to be careful because we may have skipped/failed some PUBs
   // (and thus aren't waiting anymore) but we don't current record
   // that into the DB.
   pendNotCnt = subReq + subNot;
   pendNotCnt += mPubTroop.mPendTupCurCnt;
}

RendSessionMood
RendPres1Sketch::handleTooOld3(RendHandleMoodCxt &cxt)
{
   RendSession& sess = *cxt.mSess;
   if (cxt.mCat==REND_DlgCat_Pub ) 
   {
      if (sess.mMood==REND_SM_PendNotify) 
      {
         RendPubDlg* pdlg = mPubTroop.getPubDlg(sess);
         resip_assert( pdlg );
         bool readyB = mEventInfo.checkPublish(pdlg->mToAcctIdx, pdlg->mRepeatIdx);
         if ( readyB ) 
         {
            // something went wrong: this shouldn't be pending!
            ErrLog(<<"PubCheckPend: Lost NOTIFY tracking: pending PUB shouldn't be pending:"<<cxt.mKey);
            resip_assert(0);
            return REND_SM_GROUP_ByPendReason;
         }
         RendTupDetail detail;
         mEventInfo.checkPublishDetail(cxt.mNow, pdlg->mToAcctIdx, pdlg->mRepeatIdx, detail);
         resip_assert( detail.mPendCnt > 0 );
         mCntMgr.inc(REND_CntCode_ReqStaleNot, REND_DlgCat_Pub);
         mCntMgr.add(REND_CntCode_TupStaleNot, REND_DlgCat_Pub, detail.mPendCnt);
         RendDlgAcctKey subKey = detail.toSubKey();
         RendSession* subSess = mSubTroop.getSession(subKey);
         if ( subSess==NULL ) 
         {
            CritLog(<<"Missing NOTIFY for PUB: No session:"
               <<" key="<<cxt.mKey
               <<" tup="<<detail
               <<" subkey="<<subKey);
            resip_assert(0);
            return REND_SM_GROUP_ByPendReason;
         }
         RendSessionFmtr pubFmtr(cxt.mNow, sess);
         RendSessionFmtr subFmtr(cxt.mNow, *subSess);
         WarningLog(<<"PubCheckPend: Stale PUB: never got NOTIFY:"
                    <<" key="<<cxt.mKey
                    <<" age="<<cxt.mAge
                    <<" pub="<<pubFmtr
                    <<" tup="<<detail
                    <<" sub="<<subFmtr);
         RendSubDlg* sdlg = mSubTroop.getSubDlg(*subSess);
         if ( sdlg ) 
         {
            WarningLog(<<"PubCheckPend: Stale PUB: has SUB dlg: "
                       <<" f="<<sdlg->mFromAcctIdx
                       <<" t="<<sdlg->mToAcctIdx<<"."<<sdlg->mRepeatIdx
                       <<" lastRspC="<<sdlg->mLastRspCode
                       <<" lastRspR="<<sdlg->mLastRspReason
                       <<" lastRspW="<<sdlg->mLastRspWarnings);
            sdlg->mTryRenew = true;
         } 
         else 
         {
            // while the dialog must have existed at one point,
            // it may have been released already. That said,
            // I cannot fine the code that actually releases the shared
            // pointer, so I don't know how this can be NULL, but can be.
            // Probably the dialog is never getting created. We
            // set watching=true in the setShape function, and then
            // for some reason we never create the dialog itself.
            // Still should have set watching=false at some point,
            // but clearly things not working as expected. For now,
            // just unset watching.
            WarningLog(<<"PubCheckPend: Stale PUB: missing SUB dlg: "
               <<" SUB dlg for key="<<subKey);

            // WATCHOUT
            // There is a dangerous interaction below: if disabling
            // watching kills the last tuple we are waiting for, the
            // notifyDone CB will be invoked, which will change the
            // state of the PUB, which may really mess up the
            // iterator used by checkAge(). Thus must disable this.
            mEventInfo.setWatching(cxt.mNow, 
                                   /*pub*/subKey.mFromIdx, /*sub*/subKey.mToIdx, 
                                   subKey.mRepeatIdx, /*doWatching*/false,
                                   /*notifyPub*/false);
            // assert(0);
            // we have SUB session, so maybe recycle it?
            return REND_SM_GROUP_ByPendReason;
         }
         // if below fails, means someone recycled without removing
         // watching (which is happening w/realtunnel)
         // assert( subSess->isOpenish() );
         return REND_SM_GROUP_ByPendReason;
      }

      resip_assert(sess.mMood==REND_SM_PendReq);
      mCntMgr.inc(REND_CntCode_ReqStaleRsp, REND_DlgCat_Pub);
      cxt.mNewPendReason = REND_PR_CheckAge;
      return REND_SM_Recycle;
   }

   if (cxt.mTroop->getDlgCat()==REND_DlgCat_Sub )
   {
      RendSubDlg* sdlg = mSubTroop.getSubDlg(sess);
      resip_assert(sdlg);
      WarningLog(<<"Stale SUB "
                 <<(sess.mMood==REND_SM_PendNotify?"NOTIFY":"request")
                 <<": giving up:"
                 <<" key="<<cxt.mKey
                 <<" f="<<sdlg->mFromAcctIdx
                 <<" t="<<sdlg->mToAcctIdx<<"."<<sdlg->mRepeatIdx);
      mEventInfo.setWatching(cxt.mNow, 
         /*pub*/sdlg->mFromAcctIdx, /*sub*/sdlg->mToAcctIdx, 
         sdlg->mRepeatIdx, /*doWatching*/false);
      // XXX: be more clever: mark resub next time around?
      mCntMgr.inc(sess.mMood==REND_SM_PendNotify?REND_CntCode_ReqStaleNot:REND_CntCode_ReqStaleRsp, REND_DlgCat_Sub);
      cxt.mNewPendReason = REND_PR_CheckAge;
      return REND_SM_Recycle;
   }
   resip_assert(0);
   return REND_SM_None;
}

void
RendPres1Sketch::checkStale(RendTimeUs now) 
{
   bool failAll = false;
   int maxAgePR=-1, maxAgePN=-1, maxAgeSR=-1, maxAgeSN=-1; // returned by checkAge()
   int numOldPR = mPubTroop.checkAge(now, boost::bind(&RendPres1Sketch::handleTooOld3, this, _1), 
                                     REND_SM_PendReq, mFailAge, failAll, maxAgePR);
   int numOldPN = mPubTroop.checkAge(now, boost::bind(&RendPres1Sketch::handleTooOld3, this, _1), 
                                     REND_SM_PendNotify, mFailAge, failAll, maxAgePN);
   int numOldSR = mSubTroop.checkAge(now, boost::bind(&RendPres1Sketch::handleTooOld3, this, _1), 
                                     REND_SM_PendReq, mFailAge, failAll, maxAgeSR);
   int numOldSN = mSubTroop.checkAge(now, boost::bind(&RendPres1Sketch::handleTooOld3, this, _1), 
                                     REND_SM_PendNotify, mFailAge, failAll, maxAgeSN);

   if ( numOldPR>0 || numOldPN>0 || numOldSR>0 || numOldSN>0 )
   {
      WarningLog(<<"Stale dialogs:"
                 <<" PubReq[cnt="<<numOldPR<<" age="<<maxAgePR<<"]"
                 <<" PubNot[cnt="<<numOldPN<<" age="<<maxAgePN<<"]"
                 <<" SubReq[cnt="<<numOldSR<<" age="<<maxAgeSR<<"]"
                 <<" SubNot[cnt="<<numOldSN<<" age="<<maxAgeSN<<"]");
   }
}

struct RendFmtMoodStat 
{
   RendFmtMoodStat(const RendTroopBase& tr, RendSessionMood mood) 
      : mTroop(tr), mMood(mood) { }
   const RendTroopBase& mTroop;
   RendSessionMood mMood;
};

std::ostream&
operator<<(std::ostream& os, const RendFmtMoodStat stat)
{
   const RendStatAcc& dur = stat.mTroop.getMoodDur(stat.mMood);
   os<<"[";
   if ( dur.mCnt==0 ) 
   {
      os << "0";
   } 
   else 
   {
      char buf1[50];
      os<<"Most="<<stat.mTroop.getMostMoodCnt(stat.mMood)
         <<" Dur="<<dur.fmt1(buf1, sizeof(buf1), .001, 3)
         <<"ms";
   }
   if ( stat.mMood==REND_SM_PendReq ) 
   {
      unsigned cntFail = stat.mTroop.mCntMgr.get(REND_CntPeriod_Current,REND_CntCode_ReqFailRsp,stat.mTroop.getDlgCat());
      if ( cntFail )
         os<<" Fail="<<cntFail;
   }

   unsigned cntStale = 0;
   if ( stat.mMood==REND_SM_PendReq ) 
   {
      cntStale = stat.mTroop.mCntMgr.get(REND_CntPeriod_Current,REND_CntCode_ReqStaleRsp,stat.mTroop.getDlgCat());
   } 
   else if ( stat.mMood==REND_SM_PendNotify ) 
   {
      cntStale = stat.mTroop.mCntMgr.get(REND_CntPeriod_Current,REND_CntCode_ReqStaleNot,stat.mTroop.getDlgCat());
   }

   if ( cntStale )
      os<<" Stale="<<cntStale;

   os<<"]";

   return os;
}

int
RendPres1Sketch::doWorkChunk(RendTimeUs now, int minWork, int maxWork) 
{
   //int origMinWork = minWork;
   //int origMaxWork = maxWork;
   int totWork = 0;
   // char buf1[50], buf2[50];

   ++mChunkCnt;
   if ( mRunState==REND_SKETCH_RS_STOPPING ) 
   {
      mPubTroop.setTgtOpenDlgs(0);
      mSubTroop.setTgtOpenDlgs(0);
      return doTroopWork(now, minWork, maxWork);
   }
   for (;;) 
   {
      int numWork = 0;
      int nextState = 0;
      int doneB = false;
      // bool doneB = false;

      if ( mNextWorkState ) 
      {
         resip_assert( mWorkState != mNextWorkState );
         int curPend = getSessionMoodCnt(REND_SM_GROUP_Pend);
         if ( curPend > 0 ) 
         {
            mStallWhy = "stfini";
            break;
         }
         if ( mBigDurWhich && mBigDurStart!=0 ) 
         {
            *mBigDurWhich = now - mBigDurStart;
         }
         mBigDurWhich = 0;
         mWaveStateStartTime = now;
         mWorkState = mNextWorkState;
         mNextWorkState = 0;
      }
      if ( maxWork <= 0 ) 
      {
         break;
      }
      int curState = mWorkState;
      switch (mWorkState) 
      {
      case REND_SKETCH_WS_LOOP_START:
         // nothing special to do here
         // XXX: increment counter? soft exit?
         ++mWaveCnt;
         nextState = REND_SKETCH_WS_SUB_START;
         break;

      case REND_SKETCH_WS_SUB_START:
         mBigDurStart = now;
         setSubShape(now);
         // mMostSubPendReq = 0;
         // mMostSubPendNot = 0;
         mSubTroop.updateWorkStats(/*clearB*/true);
         mSubTroop.adjustAllDlgs(0, 0, 0); // restart adjust loop
         nextState = REND_SKETCH_WS_SUB_1;
         break;

      case REND_SKETCH_WS_SUB_1:
         if ((numWork=mSubTroop.adjustAllDlgs(now, 
                                              boost::bind(&RendSubTroop::checkGoalMood, &mSubTroop, _1), 
                                              maxWork)) == 0) 
         {
            nextState = REND_SKETCH_WS_SUB_2;
         }
         break;

      case REND_SKETCH_WS_SUB_2:
         mCurWaveReqTotCnt = mSubTroop.getSessionMoodCnt(REND_SM_Wave);
         mCurWaveTupTotCnt = mCurWaveReqTotCnt;
         nextState = REND_SKETCH_WS_SUB_3;
         break;

      case REND_SKETCH_WS_SUB_3:
         {
            // every SUB (new, renew or close) triggers rsp and notify
            int doDlgs = (maxWork+1) / 2;
            numWork = 2 * mSubTroop.changeDlgCnt( now, doDlgs, REND_SM_Wave, REND_SM_GROUP_ByPendReason);
            if ( numWork==0 ) 
            {
               nextState = REND_SKETCH_WS_SUB_4;
            }
         }
         break;

      case REND_SKETCH_WS_SUB_4:
         if ( (numWork=mSubTroop.renewDlgCnt(now, maxWork)) == 0) 
         {
            nextState = REND_SKETCH_WS_SUB_5;
         } 
         else 
         {
            // Renews should have happened in the adjust phase. But
            // if adjust takes long time, some might not have been
            // ready for the adjust phase but now are.
            WarningLog(<<"Sub expire tracker triggered -- shouldn't happen"
               <<" num="<<numWork);
         }
         break;

      case REND_SKETCH_WS_SUB_5:
         // triggers stall for pending & stats
         mBigDurWhich = &mWaveLastSubDur;
         nextState = REND_SKETCH_WS_SUB_DONE;
         break;

      case REND_SKETCH_WS_SUB_DONE:
         {
            RendFmtMoodStat fmtPendReq(mSubTroop,REND_SM_PendReq);
            RendFmtMoodStat fmtPendNot(mSubTroop,REND_SM_PendNotify);
            InfoLog(<<"SubComplete: SN="<<mWaveCnt
               <<" TotSub="<<mSubTroop.getSessionMoodCnt(REND_SM_Open)
               <<" TotReq="<<mCurWaveReqTotCnt
               <<" DurSecs="<<REND_US2S(mWaveLastSubDur)
               <<" MostSubPerPub="<<mMostSubPerPub
               <<" SubPendReq="<<fmtPendReq
               <<" SubPendNot="<<fmtPendNot
               );
         }
         nextState = REND_SKETCH_WS_PUB_START;
         break;

      case REND_SKETCH_WS_PUB_START:
         mBigDurStart = now;
         // mPubTroop.mWaveCurTupRemCnt = 0;
         mPubTroop.adjustAllDlgs(0, 0, 0); // restart adjust loop
         nextState = REND_SKETCH_WS_PUB_1;
         break;

      case REND_SKETCH_WS_PUB_1:
         // With current PubTroop, this should never do work
         // (It move stuff to Wave mood, not doesn't start any dlgs)
         if ( (numWork=mPubTroop.adjustAllDlgs(now, 
            boost::bind(&RendPubTroop::checkGoalMood, &mPubTroop, _1), 
            maxWork)) == 0) 
         {
            nextState = REND_SKETCH_WS_PUB_2;
         }
         break;

      case REND_SKETCH_WS_PUB_2:
         mCurWaveReqTotCnt = mPubTroop.getSessionMoodCnt(REND_SM_Wave);
         mCurWaveTupTotCnt = mCurWaveReqTotCnt * mMostSubPerPub;
         //mCurWaveTupTotCnt = mPubTroop.mWaveCurTupRemCnt;
         // mMostPubPendReq = 0;
         // mMostPubPendNot = 0;
         mPubTroop.updateWorkStats(mMostSubPerPub, /*clear*/true);
         nextState = REND_SKETCH_WS_PUB_3;
         break;

      case REND_SKETCH_WS_PUB_3:
         {
            int spp = mMostSubPerPub > 1 ? mMostSubPerPub : 1;
            int doDlgs = (maxWork+spp-1) / spp;
            numWork = spp * mPubTroop.changeDlgCnt( now, doDlgs, 
               REND_SM_Wave, REND_SM_GROUP_ByPendReason);

            if ( numWork==0 ) 
            {
               nextState = REND_SKETCH_WS_PUB_4;
            }
         }
         break;

      case REND_SKETCH_WS_PUB_4:
         if ( (numWork=mPubTroop.renewDlgCnt(now, maxWork)) == 0) 
         {
            nextState = REND_SKETCH_WS_PUB_5;
         } 
         else 
         {
            WarningLog(<<"Pub expire tracker triggered -- shouldn't happen");
         }
         break;

      case REND_SKETCH_WS_PUB_5:
         mBigDurWhich = &mWaveLastPubDur;
         nextState = REND_SKETCH_WS_PUB_DONE;
         break;

      case REND_SKETCH_WS_PUB_DONE:
         {
            RendFmtMoodStat fmtPendReq(mPubTroop,REND_SM_PendReq);
            RendFmtMoodStat fmtPendNot(mPubTroop,REND_SM_PendNotify);
            InfoLog(<<"PubComplete:  SN="<<mWaveCnt
               <<" TotPubs="<<mCurWaveReqTotCnt
               <<" TotTups="<<mCurWaveTupTotCnt
               <<" TotSecs="<<REND_US2S(mWaveLastPubDur)
               <<" Most/MaxSubPerPub="<<mMostSubPerPub<<"/"<<mMaxSubPerPub
               <<" PubPendReq="<<fmtPendReq
               <<" PubPendNot="<<fmtPendNot
               <<" PubPendTupMost="<<mPubTroop.mPendTupMostCnt
               <<" TupStale="<<mCntMgr.get(REND_CntPeriod_Current,REND_CntCode_TupStaleNot,REND_DlgCat_Pub)
               );
         }
         nextState = REND_SKETCH_WS_LOOP_PAUSE;
         break;

      case REND_SKETCH_WS_LOOP_PAUSE:
         if ( REND_US2S(now-mWaveStateStartTime) >= 1 ) 
         {
            mCntMgr.endwave();
            nextState = REND_SKETCH_WS_LOOP_START;
         }
         doneB = true;
         break;

      default:
         resip_assert(0);
      }
      resip_assert( curState == mWorkState );

      minWork -= numWork; // not used?
      maxWork -= numWork;
      totWork += numWork;

      //InfoLog(<<"ChunkWork: cc="<<mChunkCnt
      // <<" last="<<mWorkState
      // <<" cnts=("<<numWork<<":"<<origMinWork<<"/"<<origMaxWork
      // <<".."<<totWork<<".."<<minWork<<"/"<<maxWork
      // <<") next="<<nextState);

      if ( nextState!=0 && nextState != mWorkState ) 
      {
         mNextWorkState = nextState;
         if ( ! doneB )
            continue;
      }
      break;
   }
   mPubTroop.updateWorkStats(mMostSubPerPub, /*clear*/false);
   mSubTroop.updateWorkStats();
   return totWork;
}


extern struct poptOption ThePres1OptTbl[];
// above, not really extern but otherwise compiler wants to know size

class RendPres1Options : public RendOptsBase 
{
public:
   RendPres1Options() : mWorkVol(1, 0.0, 1.0, 10) 
   {
      // mAcctBase = 0;
      // mAcctLen = 0;	// default to all available in AcctMgr
      // mAcctStride = 1;
      mPubRepeatBase = 5;
      mPubRepeatLen = 1;
      mPubExpireSecs = 10*60;

      mSubRepeatBase = 15;
      mSubRepeatLen = 1;
      mSubExpireSecs = 20*60;

      // mWorkPendCntMax = 100;
      mWorkPendAgeMax = 8;

      mMaxSubPerPub = 0;

      mFailAge = 35;
   }
   virtual struct poptOption* getPoptTbl() { return ThePres1OptTbl; }
   virtual const char*		getPoptDesc() { return "Pres1Options"; };

   //int mAcctBase;
   //int mAcctLen;
   //int mAcctStride;
   int mPubRepeatBase;
   int mPubRepeatLen;
   int mPubExpireSecs;

   int mSubRepeatBase;
   int mSubRepeatLen;
   int mSubExpireSecs;

   //int mWorkPendCntMax;
   int mWorkPendAgeMax;	// seconds

   int mMaxSubPerPub;

   int mFailAge;

   /*
   * Below here can be changed dynamically
   **/
   RendWorkVolume	mWorkVol;
};


class RendPres1SketchFactory : public RendSketchFactory 
{
public:
   RendPres1SketchFactory() { };
   virtual const char* getName() const { return "Pres1"; }
   virtual RendOptsBase* getOpts() { return &mOpts; }
   virtual RendSketchBase* makeSketch(RendTu& tu, RendCntMgr& cntMgr);

   // virtual void setWorkVolume(RendWorkVolume& vol);
   // void setWorkLevel(int level);

   RendPres1Options mOpts;
};

static RendPres1SketchFactory ThePres1SketchFactory;
RendSketchFactory* TheRendPres1SketchFacPtr = &ThePres1SketchFactory;

RendSketchBase* 
RendPres1SketchFactory::makeSketch(RendTu& tu, RendCntMgr& cntMgr) 
{ 
   RendTimeUs now = RendGetTimeUsRel();

   RendPubSubEvent* evInfo = new RendPubSubEvent( resip::Data("presence"), resip::Mime( "application", "pidf+xml") );

   // int numDlgs = mOpts.mAcctLen>0 ? mOpts.mAcctLen : tu.getAcctMgr().getNumAccts();
   int numDlgs = tu.getAcctMgr().getNumAccts();

   RendPubTroop* pubTr = new RendPubTroop(now, tu, cntMgr, *evInfo);
   pubTr->setVectorDlgs(numDlgs, /*base*/0, numDlgs, /*stride*/1, mOpts.mPubRepeatBase, mOpts.mPubRepeatLen);
   pubTr->setExpires(mOpts.mPubExpireSecs);

   RendSubTroop* subTr = new RendSubTroop(now, tu, cntMgr, *evInfo);
   subTr->setMatrixDlgs(numDlgs, numDlgs, mOpts.mSubRepeatBase, mOpts.mSubRepeatLen);
   subTr->setExpires(mOpts.mSubExpireSecs);

   evInfo->setSize(numDlgs, mOpts.mPubRepeatBase+mOpts.mPubRepeatLen, mOpts.mSubRepeatBase+mOpts.mSubRepeatLen);
   evInfo->setNotifyObj(pubTr);

   RendPres1Sketch* sketch = new RendPres1Sketch(tu, cntMgr, *evInfo, *pubTr, *subTr);
   sketch->setWorkVolume(mOpts.mWorkVol);
   //sketch->mWorkPendCntMax = mOpts.mWorkPendCntMax;
   sketch->mWorkPendAgeMax = mOpts.mWorkPendAgeMax;
   sketch->mMaxSubPerPub = mOpts.mMaxSubPerPub;
   sketch->mFailAge = mOpts.mFailAge;
   return sketch;
}


#define OPTOBJ (ThePres1SketchFactory.mOpts)
struct poptOption ThePres1OptTbl[] = 
{
#if 0
   { "regacctbase", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mAcctBase, 0,  "First account index" },
   { "regacctlen", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mAcctLen, 0, "Number of accounts" },
   { "regrepeatbase", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mRegRepeatLen, 0, "Repeat base"},
#endif
   { "pubrepeat", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mPubRepeatLen, 0,  "PUBLISH repeat factor"},
   { "subrepeat", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mSubRepeatLen, 0, "SUBSCRIBE repeat factor"},
   { "level", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkLevel, 0, "Target number of open PUBLISH dialogs"},
   { "pubexpire", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mPubExpireSecs, 0, "PUBLISH Expires"},
   { "subexpire", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mSubExpireSecs, 0, "SUBSCRIBE Expires"},
   { "minrate", 'r', POPT_ARG_FLOAT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkRateMin, 0, "Target minimum change rate"},
   { "maxrate", 'R', POPT_ARG_FLOAT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkRateMax, 0, "Target maximum change rate"},
   { "maxpend", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkPendCntMax, 0, "Maximum count of pending work"},
   { "maxage", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkPendAgeMax, 0, "Maximum age (secs) of pending work"},
   { "failage", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mFailAge, 0, "Age (secs) at which to give up on request/notify"},
   { "maxsubperpub", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mMaxSubPerPub, 0, "Maximum SUB per presentity"},
   { NULL, 0, 0, NULL, 0 }
};

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
