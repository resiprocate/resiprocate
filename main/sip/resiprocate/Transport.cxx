#include <errno.h>

#include <sipstack/Transport.hxx>
#include <sipstack/SipMessage.hxx>

using namespace Vocal2;


Transport::TransportException::TransportException(const Data& msg, const Data& file, const int line)
{
}

const char* 
Transport::TransportException::what() const throw()
{
   return "TransportException";
}


Transport::Transport(int portNum, Fifo<SipMessage>& rxFifo) :
   mPort(portNum), 
   mRxFifo(rxFifo),
   mShutdown(false)
{
}


void
Transport::run()
{
   while(!mShutdown)
   {
      fd_set fdSet; 
      int fdSetSize;

      FD_ZERO(&fdSet); 
      fdSetSize=0;
      FD_SET(mFd,&fdSet); 
      fdSetSize = std::max( mFd+1, fdSetSize );

      int  err = select(fdSetSize, &fdSet, 0, 0, 0);
      int e = errno;
      if (e == 0)
      {
         process();
      }
      else
      {
         assert(0);
      }
   }
}

void
Transport::shutdown()
{
   mShutdown = true;
}

Transport::~Transport()
{
}


