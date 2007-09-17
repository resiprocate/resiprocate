#ifndef FDSET_H
#define FDSET_H

#include <cassert>
#include <errno.h>
#include <algorithm>
#include "Socket.h"

namespace p2p
{
  inline int getErrno() { return errno; }
  class FdSet
  {
  public:
  FdSet():size(0), numReady(0)
      {
	FD_ZERO(&read);
	FD_ZERO(&write);
	FD_ZERO(&except);
	FD_ZERO(&masterRead);
	FD_ZERO(&masterWrite);
	FD_ZERO(&masterExcept);
      }
    
    int select(struct timeval& tv)
    {
      read = masterRead;
      write = masterWrite;
      except = masterExcept;
      return numReady = ::select(size, &read, &write, &except, &tv);
    }
    int select(){
      read = masterRead;
      write = masterWrite;
      except = masterExcept;
      return numReady = ::select(size, &read, &write, &except, NULL);
    }
    
    int selectMilliSeconds(unsigned long ms)
    {
      struct timeval tv;
      tv.tv_sec = (ms/1000);
      tv.tv_usec = (ms%1000)*1000;
      return select(tv);
    }
    
    int selectMicroSeconds(unsigned long ms)
    {
      struct timeval tv;
      tv.tv_sec = (ms/1000000);
      tv.tv_usec = (ms%1000000);
 
      return select(tv);
    }
    
    bool readyToRead(Socket fd)
    {
      return ( FD_ISSET(fd, &read) != 0);
    }
    
    bool readyToWrite(Socket fd)
    {
      return ( FD_ISSET(fd, &write) != 0);
    }
    
    bool hasException(Socket fd)
    {
      return (FD_ISSET(fd,&except) != 0);
    }
    
    void setRead(Socket fd)
    {
      FD_SET(fd, &masterRead);
      size = ( int(fd+1) > size) ? int(fd+1) : size;
    }
    
    void setWrite(Socket fd)
    {
      FD_SET(fd, &masterWrite);
      size = ( int(fd+1) > size) ? int(fd+1) : size;
    }
    
    void setExcept(Socket fd)
    {
      FD_SET(fd,&masterExcept);
      size = ( int(fd+1) > size) ? int(fd+1) : size;
    }
    
    void clear(Socket fd)
    {
      FD_CLR(fd, &read);
      FD_CLR(fd, &write);
      FD_CLR(fd, &except);
      FD_CLR(fd, &masterRead);
      FD_CLR(fd, &masterWrite);
      FD_CLR(fd, &masterExcept);
    }
    
    void clearRead(Socket fd)
    {
      FD_CLR(fd, &read);
      FD_CLR(fd, &masterRead);
    }

    void clearWrite(Socket fd)
    {
      FD_CLR(fd, &write);
      FD_CLR(fd, &masterWrite);
    }
    
    void reset()
    {
      size = 0;
      numReady = 0;
      FD_ZERO(&read);
      FD_ZERO(&write);
      FD_ZERO(&except);
      FD_ZERO(&masterRead);
      FD_ZERO(&masterWrite);
      FD_ZERO(&masterExcept);
    }
    
    // Make this stuff public for async dns/ares to use
    fd_set read;
    fd_set write;
    fd_set except;
    fd_set masterRead;
    fd_set masterWrite;
    fd_set masterExcept;
    int size;
    int numReady;  // set after each select call
  };
}
#endif
