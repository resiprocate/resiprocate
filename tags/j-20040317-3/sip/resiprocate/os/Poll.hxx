#ifndef RESIP_POLL_HXX
#define RESIP_POLL_HXX

// One of the following macros must be defined:
//#define RESIP_POLL_IMPL_POLL
#define RESIP_POLL_IMPL_SELECT
//#define RESIP_POLL_IMPL_EPOLL  // Not yet implemented.

/*
Define this macro to support applications that must call system call "poll"
or its ilk themselves rather than calling method "Poll::wait".
*/
#define RESIP_POLL_EXTERN

#include <vector>
#ifdef RESIP_POLL_IMPL_POLL
#include <sys/poll.h>
#endif
#ifdef RESIP_POLL_IMPL_SELECT
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#ifdef RESIP_POLL_EXTERN
#include <map>
#endif

namespace resip {

///////////////////////////////////////////////////////////////////////////////

/*
This class abstracts the Unix system call "poll".
It offers implementations in terms of "poll" itself, "epoll", and "select".
(Respectively #ifdef'd by RESIP_POLL_IMPL_POLL, RESIP_POLL_IMPL_SELECT, and
RESIP_POLL_IMPL_EPOLL.)
File descriptors are wrapped by inner class "Poll::FDEntry".
The actual "poll" call is represented by method "wait", or if an application
wants to control the system call directly, it can instead use methods
"beforeExternWait", "setEntryFDStateForExternWait", and "afterExternWait".
(The latter are selected by #define-ing RESIP_POLL_EXTERN.)
*/
class Poll {

 public:

  class FDEntry {

    friend class Poll;

   public:

    typedef unsigned short StateBitMask; // FDStateBitMask | ResipStateBitMask.
    enum FDStateBitMaskEnum {
      fdsbmReadable       = 0x0001, // (POLLIN)
      fdsbmWritable       = 0x0004, // (POLLOUT)
      fdsbmError          = 0x0008, // (POLLERR)
      fdsbmAll            = Poll::FDEntry::fdsbmReadable |
                            Poll::FDEntry::fdsbmWritable | 
                            Poll::FDEntry::fdsbmError
    };
    typedef StateBitMask FDStateBitMask;
    enum ResipStateBitMaskEnum {
      rsbmIsTransport    = 0x0040,
      rsbmIsWritePending = 0x0080
    };
    typedef StateBitMask ResipStateBitMask;

   private:

    static
    int
    compare(const Poll::FDEntry * leftFDEntry,
            const Poll::FDEntry * rightFDEntry);
        
    // Fields:
    Poll *                       _poll;
    int/*FD*/                    _fd;
    Poll::FDEntry::StateBitMask  _stateBitMask;
    unsigned short               _index; // Within "_poll->_fdEntryVector".

   public:

    //constructor
    FDEntry(Poll *     poll,
            bool       isTransport,
            int/*FD*/  fd);

    //destructor
    ~FDEntry();

    inline
    int//FD
    getFD() const
    {
      return _fd;
    }

    inline
    Poll::FDEntry::StateBitMask
    getStateBitMask() const
    {
      return _stateBitMask;
    }

    void
    clearFDState();

   protected:

    void
    setIsWritePending(bool isWritePending);

   private:

    // Copy constructor: declared but not defined
    FDEntry(const Poll::FDEntry &  from);

    // Assignment: declared but not defined
    Poll::FDEntry &
    operator=(const Poll::FDEntry &  from);

  };
  friend class Poll::FDEntry;

 private:

  // Fields:
  vector<Poll::FDEntry *>          _fdEntryVector;
#ifdef RESIP_POLL_IMPL_POLL
  vector<pollfd>                   _pollFDVector;
#endif
#ifdef RESIP_POLL_IMPL_SELECT
  int/*FD*/                        _maxFDPlus1;
  fd_set                           _readFDSet;
  fd_set                           _writeFDSet;
#endif
#ifdef RESIP_POLL_EXTERN
  map<int/*FD*/, Poll::FDEntry *>  _fdEntryMap;
#endif
  vector<Poll::FDEntry *>          _waitResult;

 public:

  /*
  "waitResult" is the result of method "wait" or "afterExternWait".
  Returns the index of the entry with the smallest file descriptor >= "fd" in
  "waitResult", or "waitResult.size()" if none exists.
  */
  static
  int
  findFDInWaitResult(int/*FD*/                        fd,
                     const vector<Poll::FDEntry *> &  waitResult);

  //constructor
  Poll();

  //destructor
  ~Poll();

#ifdef RESIP_POLL_EXTERN  // {

  // The result is the set of entries in this poll object.
  const vector<Poll::FDEntry *> &
  beforeExternWait();

  /*
  If "fd" has no entry in this poll object, ignore it and return false.
  Otherwise, set the entry's FD state from "fdStateBitMask" and return true.
  If the same "fd" is passed in multiple times, the "fdStateBitMask" are
  or-ed together.
  */
  bool
  setEntryFDStateForExternWait(int/*FD*/                    fd,
                               Poll::FDEntry::StateBitMask  fdStateBitMask);

  // The result is identical to that of method "wait".
  const vector<Poll::FDEntry *> &
  afterExternWait();

#else //!defined(RESIP_POLL_EXTERN)  } {

  /*
  Wait for I/O on any of this poll object's entry's file descriptors.
  More precisely, wait for the file descriptor of any entry with a pending
  write to be writable or for any entry's file descriptor to have readable
  input or an error.
  If "timeoutMilliSeconds" is negative, it means wait indefinitely, otherwise
  it indicates the maximum period to wait.
  The result is an array of the entries for the file descriptors that are
  readable, writable, or error, as specified by each entry's FD state.
  The result contains no duplicates and is sorted by file descriptor.
  (To service file descriptors with round-robin prioritization, remember the
  last file descriptor serviced and use method "Poll::findFDInWaitResult" to
  find the index in the result array of the highest priority entry.)
  The result is valid until the next call to this method, method
  "beforeExternWait", or the destructor.
  At some point before then, for each entry in the result, method
  "Poll::FDEntry::clearFDState" must be called.
  */
  const vector<Poll::FDEntry *> &
  wait(int timeoutMilliSeconds);

#endif //!defined(RESIP_POLL_EXTERN)  }

 private:

  // Copy constructor: declared but not defined
  Poll(const Poll &  from);

  // Assignment: declared but not defined
  Poll &
  operator=(const Poll &  from);

};

///////////////////////////////////////////////////////////////////////////////

} // namespace resip

#endif //!defined(RESIP_POLL_HXX)
