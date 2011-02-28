#include <assert.h>
#include <string.h>

#include "rutil/FdPoll.hxx"
#include "rutil/Logger.hxx"
#include "rutil/BaseException.hxx"

#include <vector>

#ifdef RESIP_POLL_IMPL_EPOLL
#  include <sys/epoll.h>
#endif

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

/*****************************************************************
 *
 * FdPollItemIf and FdPollItemBase impl
 *
 *****************************************************************/

FdPollItemIf::~FdPollItemIf()
{
}

FdPollItemBase::FdPollItemBase(FdPollGrp *grp, Socket fd, FdPollEventMask mask) :
  mPollGrp(grp), mPollSocket(fd)
{
    mPollHandle = mPollGrp->addPollItem(fd, mask, this);
}

FdPollItemBase::~FdPollItemBase()
{
    mPollGrp->delPollItem(mPollHandle);
}

/*****************************************************************
 *
 * FdPollGrp
 *
 * Implementation for some of the base class methods.
 * While some of these are epoll-specific, we can (and do) implement
 * them at this level.
 * For now we use delegation for the impl data structures
 * rather than inhieritance. Long term, not sure which will
 * be cleaner.
 *
 *****************************************************************/

FdPollGrp::FdPollGrp()
{
}

FdPollGrp::~FdPollGrp()
{
}


void
FdPollGrp::buildFdSet(FdSet& fdset) const
{
   int fd = getEPollFd();
   if (fd != -1)
   {
      fdset.setRead(fd);
   }
}

void
FdPollGrp::buildFdSet(fd_set& readfds) const
{
   int fd = getEPollFd();
   if (fd != -1)
   {
      FD_SET(fd, &readfds);
   }
}

void
FdPollGrp::processFdSet(FdSet& fdset)
{
   int fd = getEPollFd();
   if (fd !=- 1 && fdset.readyToRead(fd))
   {
      waitAndProcess();
   }
}

void
FdPollGrp::processFdSet(fd_set& readfds)
{
   int fd = getEPollFd();
   if (fd !=- 1 && FD_ISSET(fd, &readfds))
   {
      waitAndProcess();
   }
}

void
FdPollGrp::processItem(FdPollItemIf *item, FdPollEventMask mask)
{
   try
   {
      item->processPollEvent( mask );
   }
   catch(BaseException& e)
   {
           // kill it or something?
       ErrLog(<<"Exception thrown for FdPollItem: " << e);
   }
   item = NULL; // WATCHOUT: item may have been deleted
   /*
    * If FPEM_Error was reported, should really make sure it was deleted
    * or disabled from polling. Otherwise were in stuck in an infinite loop.
    * But difficult to do that checking robustly until we serials the items.
    */
}

int
FdPollGrp::getEPollFd() const
{
   return -1;
}

/*****************************************************************
 *
 * FdPollImplFdSet
 *
 *****************************************************************/

/**
  This is an implemention built around FdSet, which in turn is built
  around select(). As such, it should work on all platforms. The
  number of concurrent fds is limited by your platform's select call.
**/

namespace resip
{

class FdPollItemFdSetInfo
{
   public:
      FdPollItemFdSetInfo()
         : mSocketFd(INVALID_SOCKET), mItemObj(0), mEvMask(0), mNextIdx(-1)
      {
      }

      Socket mSocketFd; // socket
      FdPollItemIf* mItemObj; // callback object
      FdPollEventMask mEvMask; // events the application wants
      int mNextIdx;             // next link for live or free list
};



class FdPollImplFdSet : public FdPollGrp
{
   public:
      FdPollImplFdSet();
      ~FdPollImplFdSet();

      virtual const char*       getImplName() const { return "fdset"; }

      virtual FdPollItemHandle  addPollItem(Socket fd,
                                  FdPollEventMask newMask, FdPollItemIf *item);
      virtual void              modPollItem(FdPollItemHandle handle,
                                  FdPollEventMask newMask);
      virtual void              delPollItem(FdPollItemHandle handle);

      virtual bool              waitAndProcess(int ms=0);

   protected:
      void                      killCache(Socket fd);

      std::vector<FdPollItemFdSetInfo>  mItems;

      /*
       * The ItemInfos are stored in a vector (above) that grows as needed.
       * Every Info is in one single-linked list, either the "Live" list
       * or the "Free" list. This is somewhat like using
       * boost::intrusive::slist, except we use indices not pointers
       * since the vector may reallocate and move around.
       */
      int                       mLiveHeadIdx;
      int                       mFreeHeadIdx;

      /*
       * This is temporary cache of poll events. It is a member (and
       * not on stack) for two reasons: (1) simpler memory management,
       * and (2) so delPollItem() can traverse it and clean up.
       */
      FdSet                     mSelectSet;
};

};      // namespace

// NOTE: shift by one so that idx=0 doesn't have NULL handle
#define IMPL_FDSET_IdxToHandle(idx) ((FdPollItemHandle)( ((char*)0) + ((idx)+1) ))
#define IMPL_FDSET_HandleToIdx(handle) ( ((char*)(handle)) - ((char*)0) - 1)

FdPollImplFdSet::FdPollImplFdSet()
   : mLiveHeadIdx(-1), mFreeHeadIdx(-1)
{
}

FdPollImplFdSet::~FdPollImplFdSet()
{
   // assert( mEvCacheLen == 0 );  // poll not active
   unsigned itemIdx;
   for (itemIdx=0; itemIdx < mItems.size(); itemIdx++)
   {
      FdPollItemFdSetInfo& info = mItems[itemIdx];
      if (info.mItemObj)
      {
         CritLog(<<"FdPollItem idx="<<itemIdx
               <<" not deleted prior to destruction");
      }
   }
}

FdPollItemHandle
FdPollImplFdSet::addPollItem(Socket fd, FdPollEventMask newMask, FdPollItemIf *item)
{
   // if this isn't true then the linked lists will get messed up
   assert(item);
   assert(fd!=INVALID_SOCKET);

   unsigned useIdx;
   if ( mFreeHeadIdx >= 0 )
   {
      useIdx = mFreeHeadIdx;
      mFreeHeadIdx = mItems[useIdx].mNextIdx;
   }
   else
   {
      useIdx = mItems.size();
      unsigned newsz = 10+useIdx + useIdx/3; // plus 30% margin
      // WATCHOUT: below may trigger re-allocation, invalidating any iters
      // We don't use iters (only indices), but need to watchout for
      // cached pointers
      mItems.resize(newsz);
      // push new items onto the free list
      unsigned itemIdx;
      for (itemIdx=useIdx+1; itemIdx < newsz; itemIdx++)
      {
         mItems[itemIdx].mNextIdx = mFreeHeadIdx;
         mFreeHeadIdx = itemIdx;
      }
   }
   FdPollItemFdSetInfo& info = mItems[useIdx];
   info.mItemObj = item;
   info.mSocketFd = fd;
   info.mEvMask = newMask;
   info.mNextIdx = mLiveHeadIdx;
   mLiveHeadIdx = useIdx;
   return IMPL_FDSET_IdxToHandle(useIdx);
}

void
FdPollImplFdSet::modPollItem(const FdPollItemHandle handle, FdPollEventMask newMask)
{
   int useIdx = IMPL_FDSET_HandleToIdx(handle);
   assert(useIdx>=0 && ((unsigned)useIdx) < mItems.size());
   FdPollItemFdSetInfo& info = mItems[useIdx];
   assert(info.mSocketFd!=INVALID_SOCKET);
   assert(info.mItemObj);
   info.mEvMask = newMask;
}

void
FdPollImplFdSet::delPollItem(FdPollItemHandle handle)
{
   int useIdx = IMPL_FDSET_HandleToIdx(handle);
   //DebugLog(<<"deleting epoll item fd="<<fd);
   assert(useIdx>=0 && ((unsigned)useIdx) < mItems.size());
   FdPollItemFdSetInfo& info = mItems[useIdx];
   assert(info.mSocketFd!=INVALID_SOCKET);
   assert(info.mItemObj);
   killCache(info.mSocketFd);
   // we don't change the lists here since the select loop might
   // be iterating. Just mark it as dead and gc it later.
   info.mSocketFd = INVALID_SOCKET;
   info.mItemObj = NULL;
   info.mEvMask = 0;
}


/**
    There is a boundary case:
    1. fdA and fdB are added to epoll
    2. events occur on fdA and fdB
    2. waitAndProcess() select and gets events for fdA and fdB
    3. handler for fdA deletes fdB (closing fd)
    5. handler (same or differnt) opens new fd, gets fd as fdB, and adds
       it to us but under different object
    6. cache processes "old" fdB event masks but binds it to the new
       (wrong) object

    For read or write events it would be relatively harmless to
    pass these events to the new object (all objects should be prepared
    to get EAGAIN). But passing an error event could incorrectly kill
    the wrong object.

    To prevent this, we kill the events in the mSelectSet. In POSIX,
    I'm pretty sure this is always safe. In Windows, I don't know what
    happens if the fd isn't already in the FdSet.
**/
void
FdPollImplFdSet::killCache(Socket fd)
{
   mSelectSet.clear(fd);
}


bool
FdPollImplFdSet::waitAndProcess(int ms)
{
   bool didsomething = false;
   int itemIdx;
   int* prevIdxRef;
   int loopCnt;

   // Step 1: build a new FdSet from the Items vector
   mSelectSet.reset();
   prevIdxRef = &mLiveHeadIdx;
   loopCnt = 0;
   while ( (itemIdx = *prevIdxRef) != -1 )
   {
      assert( ++loopCnt < 99123123 );
      FdPollItemFdSetInfo& info = mItems[itemIdx];
      if ( info.mItemObj==0 )
      {
         // item was deleted, need to garbage collect
         assert( info.mEvMask==0 );
         // unlink from live list
         *prevIdxRef = info.mNextIdx;
         // link into free list
         info.mNextIdx = mFreeHeadIdx;
         mFreeHeadIdx = itemIdx;
         continue;
      }
      if ( info.mEvMask!=0 )
      {
         assert(info.mSocketFd!=INVALID_SOCKET);
         if ( info.mEvMask & FPEM_Read )
            mSelectSet.setRead(info.mSocketFd);
         if ( info.mEvMask & FPEM_Write )
            mSelectSet.setWrite(info.mSocketFd);
         if ( info.mEvMask & FPEM_Error )
            mSelectSet.setExcept(info.mSocketFd);
      }
      prevIdxRef = &info.mNextIdx;
   }

   // Step 2: Select on our built FdSet
   if ( ms < 0 )
   {
      // On Linux, passing a NULL timeout ptr to select() will wait
      // forever, but I don't want to trust that on all platforms.
      // So use 60sec as approximation of "forever".
      // Use 60sec b/c fits in short.
      ms = 60*1000;
   }
   int numReady = mSelectSet.selectMilliSeconds(ms);
   if ( numReady < 0 )
   {
      int err = getErrno();
      if ( err!=EINTR )
      {
         CritLog(<<"select() failed: "<<strerror(err));
         assert(0);     // .kw. not sure correct behavior...
      }
      return false;
   }
   if ( numReady==0 )
      return false;     // timer expired

   // Step 3: Invoke callbacks
   // Could take advantage of early via numReady, but book keeping
   // seems tedious especially if items are deleted during walk
   prevIdxRef = &mLiveHeadIdx;
   loopCnt = 0;
   while ( (itemIdx = *prevIdxRef) != -1 )
   {
      FdPollItemFdSetInfo& info = mItems[itemIdx];
      assert( ++loopCnt < 99123123 );
      if ( info.mEvMask!=0 && info.mItemObj!=0 )
      {
         FdPollEventMask usrMask = 0;
         assert(info.mSocketFd!=INVALID_SOCKET);
         if ( mSelectSet.readyToRead(info.mSocketFd) )
            usrMask = FPEM_Read;
         if ( mSelectSet.readyToWrite(info.mSocketFd) )
            usrMask = FPEM_Write;
         if ( mSelectSet.hasException(info.mSocketFd) )
            usrMask = FPEM_Error;

         // items's mask may have changed since select occured, so mask it again
         usrMask &= info.mEvMask;
         if ( usrMask )
         {
            processItem(info.mItemObj, usrMask);
            didsomething = true;
         }
      }
      // WATCHOUT: {info} may have moved due to add during processItem()
      // set pointer using index, not {info}
      prevIdxRef = &mItems[itemIdx].mNextIdx;
   }
   return didsomething;
}

// end of ImplFdSet


/*****************************************************************
 *
 * FdPollImplEpoll
 *
 *****************************************************************/

#ifdef RESIP_POLL_IMPL_EPOLL

namespace resip
{



class FdPollImplEpoll : public FdPollGrp
{
   public:
      FdPollImplEpoll();
      ~FdPollImplEpoll();

      virtual const char*       getImplName() const { return "epoll"; }

      virtual FdPollItemHandle  addPollItem(Socket fd,
                                  FdPollEventMask newMask, FdPollItemIf *item);
      virtual void              modPollItem(FdPollItemHandle handle,
                                  FdPollEventMask newMask);
      virtual void              delPollItem(FdPollItemHandle handle);

      virtual bool              waitAndProcess(int ms=0);

      /// See baseclass. This is integer fd, not Socket
      virtual int               getEPollFd() const { return mEPollFd; }

   protected:
      void                      killCache(Socket fd);

      std::vector<FdPollItemIf*>  mItems; // indexed by fd
      int                       mEPollFd;       // from epoll_create()

      /*
       * This is temporary cache of poll events. It is a member (and
       * not on stack) for two reasons: (1) simpler memory management,
       * and (2) so delPollItem() can traverse it and clean up.
       */
      std::vector<struct epoll_event> mEvCache;
      int                       mEvCacheCur;
      int                       mEvCacheLen;
};

};      // namespace

// NOTE: shift by one so that fd=0 doesn't have NULL handle
#define IMPL_EPOLL_FdToHandle(fd) ((FdPollItemHandle)( ((char*)0) + ((fd)+1) ))
#define IMPL_EPOLL_HandleToFd(handle) ( ((char*)(handle)) - ((char*)0) - 1)

FdPollImplEpoll::FdPollImplEpoll() :
  mEPollFd(-1)
{
   int sz = 200;        // ignored
   if ( (mEPollFd = epoll_create(sz)) < 0 )
   {
      CritLog(<<"epoll_create() failed: "<<strerror(errno));
      abort();
   }
   mEvCache.resize(sz);
   mEvCacheCur = mEvCacheLen = 0;
}

FdPollImplEpoll::~FdPollImplEpoll()
{
   assert( mEvCacheLen == 0 );  // poll not active
   unsigned itemIdx;
   for (itemIdx=0; itemIdx < mItems.size(); itemIdx++)
   {
      FdPollItemIf *item = mItems[itemIdx];
      if (item)
      {
         CritLog(<<"FdPollItem idx="<<itemIdx
               <<" not deleted prior to destruction");
      }
   }
   if (mEPollFd != -1)
   {
      close(mEPollFd);
   }
}

static inline unsigned short
CvtSysToUsrMask(unsigned long sysMask)
{
   unsigned usrMask = 0;
   if ( sysMask & EPOLLIN )
       usrMask |= FPEM_Read;
   if ( sysMask & EPOLLOUT )
       usrMask |= FPEM_Write;
   if ( sysMask & EPOLLERR )
       usrMask |= FPEM_Error|FPEM_Read|FPEM_Write;
   // NOTE: above, fake read and write if error to encourage
   // apps to actually do something about it
   return usrMask;
}

static inline unsigned long
CvtUsrToSysMask(unsigned short usrMask)
{
   unsigned long sysMask = 0;
   if ( usrMask & FPEM_Read )
       sysMask |= EPOLLIN;
   if ( usrMask & FPEM_Write )
       sysMask |= EPOLLOUT;
   if ( usrMask & FPEM_Edge )
       sysMask |= EPOLLET;
   return sysMask;
}

FdPollItemHandle
FdPollImplEpoll::addPollItem(Socket fd, FdPollEventMask newMask, FdPollItemIf *item)
{
   assert(fd>=0);
   //DebugLog(<<"adding epoll item fd="<<fd);
   if (mItems.size() <= (unsigned)fd)
   {
      unsigned newsz = fd+1;
      newsz += newsz/3; // plus 30% margin
      // WATCHOUT: below may trigger re-allocation, invalidating any iters
      // Currently only iterator is destructor, so should be safe
      mItems.resize(newsz);
   }
   FdPollItemIf *olditem = mItems[fd];
   assert(olditem == NULL);     // what is right thing to do?
   mItems[fd] = item;
   struct epoll_event ev;
   memset(&ev, 0, sizeof(ev));  // make valgrind happy
   ev.events = CvtUsrToSysMask(newMask);
   ev.data.fd = fd;
   if (epoll_ctl(mEPollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
   {
      CritLog(<<"epoll_ctl(ADD) failed: " << strerror(errno));
      abort();
   }
   return IMPL_EPOLL_FdToHandle(fd);
}

void
FdPollImplEpoll::modPollItem(const FdPollItemHandle handle, FdPollEventMask newMask)
{
   int fd = IMPL_EPOLL_HandleToFd(handle);
   assert(fd>=0 && ((unsigned)fd) < mItems.size());
   assert(mItems[fd] != NULL);

   struct epoll_event ev;
   memset(&ev, 0, sizeof(ev));  // make valgrind happy
   ev.events = CvtUsrToSysMask(newMask);
   ev.data.fd = fd;
   if (epoll_ctl(mEPollFd, EPOLL_CTL_MOD, fd, &ev) < 0)
   {
      CritLog(<<"epoll_ctl(MOD) failed: "<<strerror(errno));
      abort();
   }
}

void
FdPollImplEpoll::delPollItem(FdPollItemHandle handle)
{
   int fd = IMPL_EPOLL_HandleToFd(handle);
   //DebugLog(<<"deleting epoll item fd="<<fd);
   assert(fd>=0 && ((unsigned)fd) < mItems.size());
   assert( mItems[fd] != NULL );
   mItems[fd] = NULL;
   if (epoll_ctl(mEPollFd, EPOLL_CTL_DEL, fd, NULL) < 0)
   {
       CritLog(<<"epoll_ctl(DEL) fd="<<fd<<" failed: " << strerror(errno));
           abort();
   }
   killCache(fd);
}


/**
    There is a boundary case:
    1. fdA and fdB are added to epoll
    2. events occur on fdA and fdB
    2. waitAndProcess() reads queue for fdA and fdB into its cache
    3. handler for fdA deletes fdB (closing fd)
    5. handler (same or differnt) opens new fd, gets fd as fdB, and adds
       it to epoll but under different object
    6. cache processes "old" fdB but binds it to the new (wrong) object

    For read or write events it would be relatively harmless to
    pass these events to the new object (all objects should be prepared
    to get EAGAIN). But passing an error event could incorrectly kill
    the wrong object.

    To prevent this, we walk the cache and kill any events for our fd.
    In theory, the kernel does the same.

    An alternative approach would be use a serial number counter,
    as a lifetime indicator for each fd, and store both a 32-bit serial
    and 32-bit fd into the epoll event in the kernel. We could then
    recognize stale events.
**/
void
FdPollImplEpoll::killCache(int fd)
{
   int ne;
   for (ne=mEvCacheCur; ne < mEvCacheLen; ne++)
   {
      if ( mEvCache[ne].data.fd == fd )
      {
         mEvCache[ne].data.fd = INVALID_SOCKET;
      }
   }
}


bool
FdPollImplEpoll::waitAndProcess(int ms)
{
   bool didSomething = false;
   bool maybeMore;
   int waitMs = ms;
   assert( mEvCache.size() > 0 );
   do
   {
      int nfds = epoll_wait(mEPollFd, &mEvCache.front(), mEvCache.size(), waitMs);
      if (nfds < 0)
      {
	 if (errno==EINTR)
	 {
	    // signal handler (like alarm) broke loop. generally ok
            DebugLog(<<"epoll_wait() broken by EINTR");
	    nfds = 0;	// clean-up and return. could add return code
			// to indicate this, but not needed by us
	 }
	 else
	 {
            CritLog(<<"epoll_wait() failed: " << strerror(errno));
            abort();	// TBD: just throw instead?
	 }
      }
      waitMs = 0;             // don't wait anymore
      mEvCacheLen = nfds;     // for killCache()
      maybeMore = ( ((unsigned)nfds)==mEvCache.size()) ? 1 : 0;
      int ne;
      for (ne=0; ne < nfds; ne++)
      {
         int fd = mEvCache[ne].data.fd;
         if (fd == INVALID_SOCKET)
         {
            continue;      // was killed by killCache()
         }
         int sysEvtMask = mEvCache[ne].events;
         assert(fd>=0 && fd < (int)mItems.size());
         FdPollItemIf *item = mItems[fd];
         if (item == NULL)
         {
            /* this can happen if item was deleted after
             * event was generated in kernel, etc. */
            continue;
         }
         mEvCacheCur = ne;  // for killCache()
         processItem(item, CvtSysToUsrMask(sysEvtMask));
         item = NULL; // WATCHOUT: item may not exist anymore
         didSomething = true;
      }
      mEvCacheLen = 0;
   } while (maybeMore);
   return didSomething;
}

#endif // RESIP_POLL_IMPL_EPOLL

/*****************************************************************
 *
 * Factory
 *
 *****************************************************************/

/*static*/FdPollGrp*
FdPollGrp::create(const char *implName)
{
   if ( implName==0 || implName[0]==0 || strcmp(implName,"event")==0 )
      implName = 0;     // pick the first (best) one supported
#ifdef RESIP_POLL_IMPL_EPOLL
   if ( implName==0 || strcmp(implName,"epoll")==0 )
   {
      return new FdPollImplEpoll();
   }
#endif
   if ( implName==0 || strcmp(implName,"fdset")==0 )
   {
      return new FdPollImplFdSet();
   }
   assert(0);
   return NULL;
}

/*static*/const char*
FdPollGrp::getImplList()
{
   // .kw. this isn't really scalable approach if we get a lot of impls
   // but it works for now
#ifdef RESIP_POLL_IMPL_EPOLL
   return "event|epoll|fdset";
#else
   return "event|fdset";
#endif
}

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000-2005 Jacob Butcher
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
 * vi: set shiftwidth=3 expandtab:
 */
