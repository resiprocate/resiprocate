#include <stdlib.h>
#include "rutil/Poll.hxx"

using namespace resip;
using namespace std;

#if !defined(RESIP_POLL_IMPL_POLL) && !defined(RESIP_POLL_IMPL_SELECT) && !defined(RESIP_POLL_EPOLL)
#error "Define one of RESIP_POLL_IMPL_POLL, RESIP_POLL_IMPL_SELECT, or RESIP_POLL_EPOLL."
#endif

///////////////////////////////////////////////////////////////////////////////

//constructor
Poll::FDEntry::FDEntry(Poll* poll,
                       bool isTransport,
                       int/*FD*/  fd) :
   _poll(poll),
   _fd(fd),
   _stateBitMask(isTransport ? Poll::FDEntry::rsbmIsTransport : 0),
   _index(poll->_fdEntryVector.size())
{
   _poll->_fdEntryVector.push_back(this);
#ifdef RESIP_POLL_IMPL_POLL
   pollfd pollFD;
   pollFD.fd = fd;
   pollFD.events = POLLIN;
   _poll->_pollFDVector.push_back(pollFD);
#endif
#ifdef RESIP_POLL_IMPL_SELECT
   if (_fd >= _poll->_maxFDPlus1) 
   {
      _poll->_maxFDPlus1 = _fd + 1;
   }
   FD_SET(_fd, &(_poll->_readFDSet));
#endif
#ifdef RESIP_POLL_EXTERN
   _poll->_fdEntryMap.insert(std::pair<int/*FD*/, Poll::FDEntry *>(_fd, this));
#endif
}

//destructor
Poll::FDEntry::~FDEntry()
{
   vector<Poll::FDEntry *> &fdEntryVector = _poll->_fdEntryVector;
   Poll::FDEntry *lastFDEntry = fdEntryVector[fdEntryVector.size() - 1];
   lastFDEntry->_index = _index;
   fdEntryVector[_index] = lastFDEntry;
   fdEntryVector.pop_back();
#ifdef RESIP_POLL_IMPL_POLL
   vector<pollfd> &pollFDVector = _poll->_pollFDVector;
   pollFDVector[_index] = pollFDVector[pollFDVector.size() - 1];
   pollFDVector.pop_back();
#endif
#ifdef RESIP_POLL_IMPL_SELECT
   FD_CLR(_fd, &(_poll->_readFDSet));
   FD_CLR(_fd, &(_poll->_writeFDSet));
#endif
#ifdef RESIP_POLL_EXTERN
   _poll->_fdEntryMap.erase(_fd);
#endif
}

void
Poll::FDEntry::clearFDState()
{
   _stateBitMask &= ~Poll::FDEntry::fdsbmAll;
}

void
Poll::FDEntry::setIsWritePending(bool isWritePending)
{
   if (isWritePending) 
   {
      _stateBitMask |= Poll::FDEntry::rsbmIsWritePending;
#ifdef RESIP_POLL_IMPL_POLL
      _poll->_pollFDVector[_index].events |= Poll::FDEntry::fdsbmWritable;
#endif
#ifdef RESIP_POLL_IMPL_SELECT
      FD_SET(_fd, &(_poll->_writeFDSet));
#endif
   } else 
   {
      _stateBitMask &= ~Poll::FDEntry::rsbmIsWritePending;
#ifdef RESIP_POLL_IMPL_POLL
      _poll->_pollFDVector[_index].events &= ~Poll::FDEntry::fdsbmWritable;
#endif
#ifdef RESIP_POLL_IMPL_SELECT
      FD_CLR(_fd, &(_poll->_writeFDSet));
#endif
   }
}

//static
int
Poll::FDEntry::compare(const Poll::FDEntry * leftFDEntry,
                       const Poll::FDEntry * rightFDEntry)
{
   return (leftFDEntry->getFD() - rightFDEntry->getFD());
}

///////////////////////////////////////////////////////////////////////////////

//static
int
Poll::findFDInWaitResult(int/*FD*/                        fd,
                         const vector<Poll::FDEntry *> &  waitResult)
{
   unsigned int lowIndex = 0;
   unsigned int highIndex = waitResult.size();
   while (lowIndex + 1 < highIndex) 
   {
      // The goal fd is in waitResult in the range [lowIndex, highIndex[.
      unsigned int midIndex = (lowIndex + highIndex) / 2;
      if (waitResult[midIndex]->_fd > fd) 
      {
         highIndex = midIndex - 1;
      } 
      else 
      {
         lowIndex = midIndex;
      }
   }//while
   return lowIndex;
}

//constructor
Poll::Poll()
#ifdef RESIP_POLL_IMPL_SELECT
   :
_maxFDPlus1(0) //!jacob! Can select handle an empty poll set?
#endif
{
#ifdef RESIP_POLL_IMPL_SELECT
   FD_ZERO(&_readFDSet);
   FD_ZERO(&_writeFDSet);
#endif
}

//destructor
Poll::~Poll()
{
}

#ifdef RESIP_POLL_EXTERN  // {

const vector<Poll::FDEntry *> &
Poll::beforeExternWait()
{
   _waitResult.clear();
   return _fdEntryVector;
}

bool
Poll::setEntryFDStateForExternWait(int/*FD*/                    fd,
                                   Poll::FDEntry::StateBitMask  fdStateBitMask)
{
   map<int/*FD*/, Poll::FDEntry *>::const_iterator fdEntryIterator =
      _fdEntryMap.find(fd);
   bool isFDInPoll = (fdEntryIterator != _fdEntryMap.end());
   if (isFDInPoll) 
   {
      Poll::FDEntry *fdEntry = fdEntryIterator->second;
      fdEntry->_stateBitMask |= fdStateBitMask & Poll::FDEntry::fdsbmAll;
      _waitResult.push_back(fdEntry);
   }
   return isFDInPoll;
}

const vector<Poll::FDEntry *> &
Poll::afterExternWait()
{
   return _waitResult;
}

#else //!defined(RESIP_POLL_EXTERN)  } {

const vector<Poll::FDEntry *> &
Poll::wait(int timeoutMilliSeconds)
{
   _waitResult.clear();
#ifdef RESIP_POLL_IMPL_POLL
   pollfd *pollFDArray = &(_pollFDVector.front());
   int numReadyFDs = poll(pollFDArray,
                          _pollFDVector.size(),
                          timeoutMilliSeconds);
   if (numReadyFDs < 0) 
   {
      //!jacob! poll failed.
   }
   for (unsigned short index = 0; numReadyFDs > 0; ++index) 
   {
      int revents = pollFDArray[index].revents;
      if (revents) 
      {
         Poll::FDEntry *fdEntry = _fdEntryVector[index];
         fdEntry->_stateBitMask |= revents;
         _waitResult.push_back(fdEntry);
         --numReadyFDs;
      }
   }//for
   qsort(&(_waitResult.front()),
         _waitResult.size(),
         sizeof(Poll::FDEntry *),
         reinterpret_cast<int(*)(const void *,
                                 const void *)>(&Poll::FDEntry::compare));
#endif
#ifdef RESIP_POLL_IMPL_SELECT
   fd_set readableFDSet = _readFDSet;
   fd_set writableFDSet = _writeFDSet;
   fd_set errorFDSet = _readFDSet;
   timeval timeoutTimeVal;
   timeoutTimeVal.tv_sec = timeoutMilliSeconds / 1000;
   timeoutTimeVal.tv_usec = 1000 * (timeoutMilliSeconds % 1000);
   int numReadyFDs = select(_maxFDPlus1,
                            &readableFDSet,
                            &writableFDSet,
                            &errorFDSet,
                            &timeoutTimeVal);
   if (numReadyFDs < 0) 
   {
      //!jacob! select failed.
   }
   for (unsigned short index = 0; numReadyFDs > 0; ++index) 
   {
      Poll::FDEntry *fdEntry = _fdEntryVector[index];
      bool isFDReadable = FD_ISSET(fdEntry->_fd, &readableFDSet);
      bool isFDWritable = FD_ISSET(fdEntry->_fd, &writableFDSet);
      bool isFDError = FD_ISSET(fdEntry->_fd, &errorFDSet);
      if (isFDReadable || isFDWritable || isFDError) 
      {
         if (isFDReadable) 
         {
            fdEntry->_stateBitMask |= Poll::FDEntry::fdsbmReadable;
         }
         if (isFDWritable) 
         {
            fdEntry->_stateBitMask |= Poll::FDEntry::fdsbmWritable;
         }
         if (isFDError) 
         {
            fdEntry->_stateBitMask |= Poll::FDEntry::fdsbmError;
         }
         _waitResult.push_back(fdEntry);
         --numReadyFDs;
      }
   }//for
#endif
   return _waitResult;
}

#endif //!defined(RESIP_POLL_EXTERN)  }

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
