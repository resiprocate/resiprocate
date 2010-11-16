#if !defined(RESIP_FDPOLL_HXX)
#define RESIP_FDPOLL_HXX

// One of the following macros must be defined:
//#define RESIP_POLL_IMPL_POLL
//#define RESIP_POLL_IMPL_SELECT
#define RESIP_POLL_IMPL_EPOLL  // Only one currently implemented

/*
  Define this macro to support applications that must call system call "poll"
  or its ilk themselves rather than calling method "Poll::wait".
*/
// #define RESIP_POLL_EXTERN

#include <vector>

#ifdef RESIP_POLL_IMPL_EPOLL
#  include <sys/epoll.h>
#endif

#ifdef RESIP_POLL_IMPL_POLL
#  include <sys/poll.h>
#endif

#ifdef RESIP_POLL_IMPL_SELECT
# ifdef WIN32
#  include <winsock2.h>
# else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
# endif // WIN32
#endif // RESIP_POLL_IMPL_SELECT

#ifdef RESIP_POLL_EXTERN
# include <map>
#endif // RESI

#include "rutil/Socket.hxx"

namespace resip {


typedef unsigned short FdPollEventMask;
#define FPEM_Read	0x0001	// POLLIN
#define FPEM_Write	0x0002	// POLLOUT
#define FPEM_Error	0x0004	// POLLERR	(select exception)
#define FPEM_Edge	0x4000	// EPOLLET

class FdPollGrp;


class FdPollItemIf {
  //friend class FdPollGrp;
  public:
    FdPollItemIf() { };
    virtual ~FdPollItemIf();

    virtual Socket	getPollSocket() const = 0;

    /**
        Called by PollGrp when activity is possible
    **/
    virtual void	processPollEvent(FdPollEventMask mask) = 0;
};

class FdPollItemBase : public FdPollItemIf {
  //friend class FdPollGrp;
  public:
    FdPollItemBase(FdPollGrp *grp, Socket fd, FdPollEventMask mask);
    virtual ~FdPollItemBase();

    virtual Socket	getPollSocket() const { return mPollSocket; }

 protected:

    FdPollGrp*		mPollGrp;
    Socket		mPollSocket;
    FdPollEventMask	mPollMask;		// events we want
};

class FdPollGrp {
  public:
    FdPollGrp();
    ~FdPollGrp();


    void			addPollItem(FdPollItemIf *item,
    				  FdPollEventMask newMask);
    void			modPollItem(const FdPollItemIf *item, 
    				  FdPollEventMask newMask);
    void			delPollItem(FdPollItemIf *item);

    /// get the epoll-fd (epoll_create()) -- it is int, not Socket
    int				getEPollFd() const { return mEPollFd; }
    /// Add our epoll-fd into the fdSet (for hierarchical selects)
    void			buildFdSet(FdSet& fdSet) const;
    void			buildFdSet(fd_set& readfds) const;
    /// process epoll queue if epoll-fd is readable in fdset
    void			processFdSet(FdSet& fdset);
    void			processFdSet(fd_set& readfds);

    void			process();


    FdPollItemIf*		getItemByFd(Socket fd);
    FdPollItemIf*		modifyEventMaskByFd(FdPollEventMask mask, 
    				  Socket fd);

  protected:
    void			processItem(FdPollItemIf *item,
    				  FdPollEventMask mask);
    void			killCache(Socket fd);

    std::vector<FdPollItemIf*>	mItems;	// indexed by fd
    int				mEPollFd;	// from epoll_create()

    /*
     * This is temporary cache of poll events. It is a member (and
     * not on stack) for two reasons: (1) simpler memory management,
     * and (2) so delPollItem() can traverse it and clean up.
     */
    std::vector<struct epoll_event> mEvCache;
    int				mEvCacheCur;
    int				mEvCacheLen;

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
 */
