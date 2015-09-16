#include "rutil/Logger.hxx"

//#include "resip/stack/SipStack.hxx"
#include "resip/stack/KeepAliveMessage.hxx"

#include "RendKa.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

/***********************************************************************
 *
 * KeepAlive module
 *
 * This is the KeepAlive module. Its goal is to periodically send
 * packets to our server in order to keep the NAT pinhole open.
 * It also (can) serve to detect network connection failures.
 *
 * This module is is highly optimized for specific needs of test
 * this test application -- it is not general purpose tool. Specifically:
 * (1) All connections share one keep-alive interval, and this is
 *     established early in the constructor. This allows for single
 *     linear FIFO timer queue. No messy searches & sorts.
 * (2) We support multiple, parallel connections to the same Target.
 *     This requires tracking flow keys (not just Target) so that
 *     we can keep all of them alive.
 *
 ***********************************************************************/

struct RendKaAssoc 
{
public:
   RendKaAssoc(const resip::Tuple& target)
      : mLocalKey(0), mRefCnt(0), mTarget(target), 
      mTmrExpireUs(0), mTmrNext(NULL) 
   {
      // Only keep *this* flow alive
      mTarget.onlyUseExistingConnection = true;
   }
   RendLocalKey	mLocalKey;	// SN within all keep-alive flows
   int			mRefCnt;
   resip::Tuple	mTarget;
   RendTimeUs		mTmrExpireUs;
   RendKaAssoc*	mTmrNext;
};

typedef RendKaAssoc* RendKaAssocPtr;

bool operator<(const RendKaAssoc& a, const RendKaAssoc& b) 
{
   if ( a.mTarget.mFlowKey != b.mTarget.mFlowKey )
   {
      return a.mTarget.mFlowKey < b.mTarget.mFlowKey;
   }

   if ( a.mTarget.mTransportKey != b.mTarget.mTransportKey ) 
   {
      if ( a.mTarget.mTransportKey == 0 )
      {
         return true;
      }
      // directly compare pointers, not content! Hope this works!
      return a.mTarget.mTransportKey < b.mTarget.mTransportKey;
   }
   return a.mTarget < b.mTarget; // compares just proto&ipaddr
}

struct RendKaConn 
{
   bool operator() (const RendKaAssocPtr& a, const RendKaAssocPtr& b) 
   {
      return *a < *b;
   }
};

typedef std::set<RendKaAssocPtr, RendKaConn> RendKaConns;
typedef std::map<RendLocalKey,RendKaAssocPtr> RendKaKeys;

class RendKaMgr : public RendKaMgrIf 
{
public:
   RendKaMgr(resip::SipStack& stack, int ivalSecs) 
      : mStack(stack), mKaIvalSecs(ivalSecs), 
      mTmrHead(NULL), mTmrTail(NULL),
      mKaKeyCtr(1), mKaMsg(NULL) 
   {
   }
   virtual ~RendKaMgr();


   virtual RendLocalKey addConn(RendTimeUs now, const resip::Tuple& target);
   virtual void delConn(RendLocalKey key);
   virtual RendTimeUs processTimers(RendTimeUs now);
   virtual void getStats(RendKaStats& stats);


protected:
   resip::SipStack& mStack;
   int mKaIvalSecs;

   void tmrQueueAppend(RendTimeUs now, RendKaAssoc* kanet);
   RendKaAssoc* tmrQueuePop(RendTimeUs now, RendTimeUs *nextTime);
   RendKaAssoc* mTmrHead;
   RendKaAssoc* mTmrTail;

   RendKaConns mKaConns;	/* (src,dst) -> Assoc* */
   RendKaKeys mKaKeys;	/* LocalKey->Assoc* */
   int mKaKeyCtr;

   void sendKaMsg(RendKaAssoc *kanet);
   resip::SipMessage*  mKaMsg;
   RendKaStats mStats;
};


RendKaMgrIf*
RendKaMgrIf::createMgr(resip::SipStack& stack, int ivalSecs) 
{
    return new RendKaMgr(stack, ivalSecs);
}

RendKaMgr::~RendKaMgr()
{
   if ( mKaMsg ) 
   {
      delete mKaMsg;
      mKaMsg = NULL;
   }

   unsigned numConns = (unsigned)mKaConns.size();
   unsigned numKeys = (unsigned)mKaKeys.size();
   if ( numConns>0 || numKeys>0 ) 
   {
      CritLog(<<"KeepAlive destruction while still active conns="<<numConns
         <<" keys="<<numKeys);
   }
   resip_assert( numConns==0 && numKeys==0 );
   RendKaAssoc* kanet;
   while ( (kanet=tmrQueuePop(0, NULL)) != NULL ) 
   {
      // verify connection already removed
      resip_assert( kanet->mRefCnt == 0 );
      delete kanet;
   }
   resip_assert( mTmrHead==NULL );
}

void
RendKaMgr::tmrQueueAppend(RendTimeUs now, RendKaAssoc* kanet) 
{
   resip_assert( kanet->mTmrNext==NULL );
   resip_assert( mKaIvalSecs >= 1 );
   kanet->mTmrExpireUs = now + 1000000*mKaIvalSecs;
   if ( mTmrTail==NULL ) 
   {
      resip_assert( mTmrHead == NULL );
      mTmrHead = mTmrTail = kanet;
   } 
   else 
   {
      resip_assert( mTmrHead != NULL );
      mTmrTail->mTmrNext = kanet;
      mTmrTail = kanet;
   }
}

/**
 * WATCHOUT: If {nextTime} is NULL, it tells us we don't care
 * about times and we disable the Expire check. This feature
 * is used to drain the queue at destructor time.
 */
RendKaAssoc*
RendKaMgr::tmrQueuePop(RendTimeUs now, RendTimeUs *nextTime) 
{
   RendKaAssoc *kanet = NULL;
   if ( mTmrHead && (nextTime==NULL || mTmrHead->mTmrExpireUs <= now) ) 
   {
      kanet = mTmrHead;
      if ( kanet == mTmrTail ) 
      {
         resip_assert( kanet->mTmrNext == NULL );
         mTmrHead = mTmrTail = NULL;
      } 
      else 
      {
         mTmrHead = kanet->mTmrNext;
         kanet->mTmrNext = NULL;
      }
   }
   if ( nextTime ) 
   {
      // use 30sec as our infinity
      *nextTime = now + (mTmrHead?mTmrHead->mTmrExpireUs:1000000*30);
   }
   return kanet;
}


/**
    Each KaAssoc has multiple allocations associated with it:
    * The Assoc object itself. Each Assoc has the network info (source
      and dest), plus its assigned LocalKey.
    * Mapping from connection info (source & dest) to the Assoc object.
      This is used to consolidate all "users" of the same connection
      into a single keep-alive stream. This is why the Assoc object
      is ref counted.
    * Mapping from assoc local key to assoc object.

    The KaConns and KaKeys maps are only used when adding and deleting
    a connection (relatively infrequently). The timer queue mgmt
    (which happen frequently whenver timer fires) is handled thru
    internal single-link-list and doesn't require any memory allocation.

    While complicated, the first implementation (pure STL) required 5 allocs.
**/
RendLocalKey
RendKaMgr::addConn(RendTimeUs now, const resip::Tuple& target) 
{
   if ( mKaIvalSecs==0 )
      return 0;

   resip_assert( target.mFlowKey );
   RendKaAssoc kanet1(target);

   RendKaConns::iterator itc = mKaConns.find(&kanet1);

   RendKaAssoc *kanet;
   if ( itc == mKaConns.end() ) 
   {
      kanet = new RendKaAssoc(target);
      kanet->mLocalKey = mKaKeyCtr++;
      mKaConns.insert( kanet);
      mKaKeys.insert( std::pair<RendLocalKey,RendKaAssoc*>
         (kanet->mLocalKey,kanet));
      tmrQueueAppend(now, kanet);
   } 
   else 
   {
      kanet = *itc;
   }
   ++(kanet->mRefCnt);
   ++(mStats.mSessionCnt);
   ++(mStats.mNumCurSession);
#if 0
   std::cout << "Add Ka ival="
      <<mKaIvalSecs 
      <<" id="<<kanet->mLocalKey<<" refcnt="<<kanet->mRefCnt
      << std::endl;
#endif
   return kanet->mLocalKey;
}

void
RendKaMgr::delConn(RendLocalKey key) 
{
   resip_assert(key);
   RendKaKeys::iterator itk = mKaKeys.find(key);
   resip_assert ( itk != mKaKeys.end() );
   RendKaAssoc *kanet = itk->second;
   resip_assert( kanet );
   resip_assert( kanet->mLocalKey == key );
   resip_assert( kanet->mRefCnt > 0 );
   --(mStats.mNumCurSession);
   if ( --(kanet->mRefCnt) == 0 ) 
   {
      mKaKeys.erase( itk);
      RendKaConns::iterator itc = mKaConns.find(kanet);
      resip_assert( itc != mKaConns.end() );
      resip_assert( *itc == kanet );
      mKaConns.erase( itc);
      // NO: delete kanet; -- will delete it in tmrPop!
   }
}


void
RendKaMgr::sendKaMsg(RendKaAssoc *kanet) 
{
   if ( mKaMsg == NULL ) 
   {
      mKaMsg = new resip::KeepAliveMessage();
      // the method doesn't matter (the encode() method wins), just need something
      mKaMsg->header(resip::h_RequestLine).method() = resip::OPTIONS;
      // same thing, must exist
      resip::Via via;
      mKaMsg->header(resip::h_Vias).push_back(via);
   }
   resip_assert( kanet->mTarget.mTransportKey != 0 );
   resip_assert( kanet->mTarget.mFlowKey != 0 );
   // below makes clone of message
   mStack.sendTo(*mKaMsg, kanet->mTarget);
   ++(mStats.mSentMsgCnt);
}

RendTimeUs
RendKaMgr::processTimers(RendTimeUs now) 
{
   RendTimeUs nextTime;
   RendKaAssoc* kanet;
   //VasiLocalKey lastKey = 0;	// help check integrity of list
   int cnt = 0;		// help find infinite loops
   while ( (kanet=tmrQueuePop(now, &nextTime)) != NULL ) 
   {
      // assert( kanet->mLocalKey != lastKey ); lastKey = kanet->mLocalKey;
      resip_assert( cnt < 50000 ); ++cnt;
      resip_assert( kanet->mRefCnt >= 0 );
      if ( kanet->mRefCnt==0 ) 
      {
         delete kanet;
         continue;
      }
      sendKaMsg(kanet);
      tmrQueueAppend(now, kanet);
   }
   return nextTime;
}

void
RendKaMgr::getStats(RendKaStats& stats) 
{
   stats = mStats;
   stats.mNumCurConn = (unsigned int)mKaConns.size();
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
