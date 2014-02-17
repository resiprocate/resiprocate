#if !defined(RESIP_FDPOLL_HXX)
#define RESIP_FDPOLL_HXX

#include "rutil/Socket.hxx"

/* The Makefile system may define the following:
 * HAVE_EPOLL: system call epoll() is available
 *
 * An implementation based upon FdSet (and select()) is always available.
 *
 * This file and class is somewhat misnamed. It should really be
 * called "SocketEvent" or such. The name "FdPoll" originated
 * from an epoll-specific implementation.
 */

#if defined(HAVE_EPOLL)
#define RESIP_POLL_IMPL_EPOLL
#endif

#if defined(HAVE_POLL) || (_WIN32_WINNT >= 0x0600)
#define RESIP_POLL_IMPL_POLL
#endif

namespace resip {


typedef unsigned short FdPollEventMask;
#define FPEM_Read       0x0001  // POLLIN
#define FPEM_Write      0x0002  // POLLOUT
#define FPEM_Error      0x0004  // POLLERR      (select exception)
#define FPEM_Edge       0x4000  // EPOLLET

class FdPollGrp;

/**
 * This is opaque type used to identify a particular Item. It is assigned
 * when Item is allocated, and then used to modify or destroy the Item.
 * NOTE: FdPollItemFake doesn't exist: it is fictious, thus this type
 * can never be deferenced.
 */
typedef struct FdPollItemFake* FdPollItemHandle;

class FdPollItemIf
{
   //friend class FdPollGrp;
   public:
      FdPollItemIf() { };
      virtual ~FdPollItemIf();

      /**
        Called by PollGrp when activity is possible
      **/
      virtual void processPollEvent(FdPollEventMask mask) = 0;
};

class FdPollItemBase : public FdPollItemIf
{
   //friend class FdPollGrp;
   public:
      FdPollItemBase(FdPollGrp *grp, Socket fd, FdPollEventMask mask);
      virtual ~FdPollItemBase();

   protected:

      FdPollGrp*        mPollGrp;
      Socket            mPollSocket;
      FdPollItemHandle  mPollHandle;
};

class FdSetIOObserver;

class FdPollGrp
{
   public:
      FdPollGrp();
      virtual ~FdPollGrp();

      typedef enum {FdSetImpl = 0, PollImpl, EPollImpl } ImplType;

      /// factory
      static FdPollGrp* create(const char *implName=NULL);
      /// Return candidate impl names with vertical bar (|) between them
      /// Intended for help messages
      static const char* getImplList();

      virtual const char* getImplName() const = 0;
      virtual ImplType getImplType() const = 0;

      virtual FdPollItemHandle addPollItem(Socket sock, FdPollEventMask newMask, FdPollItemIf *item) = 0;
      virtual void modPollItem(FdPollItemHandle handle, FdPollEventMask newMask) = 0;
      virtual void delPollItem(FdPollItemHandle handle) = 0;

      virtual void registerFdSetIOObserver(FdSetIOObserver& observer) = 0;
      virtual void unregisterFdSetIOObserver(FdSetIOObserver& observer) = 0;

      /// Wait at most {ms} milliseconds. If any file activity has
      /// already occurs or occurs before {ms} expires, then
      /// FdPollItem will be informed (via cb method) and this method will
      /// return. Returns true iff any file activity occured.
      /// ms<0: wait forever, ms=0: don't wait, ms>0: wait this long
      /// NOTE: "forever" may be a little as 60sec or as much as forever
      virtual bool waitAndProcess(int ms=0) = 0;

      /// get the epoll-fd (epoll_create())
      /// This is fd (type int), not Socket. It may be -1 if epoll
      /// is not enabled.
      virtual int getEPollFd() const;

      // Legacy API's (deprecated) - use waitAndProcess instead
      virtual void buildFdSet(FdSet& fdSet)=0;
      virtual bool processFdSet(FdSet& fdset)=0;

   protected:
      void processItem(FdPollItemIf *item, FdPollEventMask mask);
};



///////////////////////////////////////////////////////////////////////////////

} // namespace resip

#endif //!defined(RESIP_FDPOLL_HXX)

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
