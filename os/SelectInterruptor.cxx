#include "resiprocate/os/SelectInterruptor.hxx"

#include <cassert>
#include "resiprocate/os/Logger.hxx"

#ifndef WIN32
#include <unistd.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

SelectInterruptor::SelectInterruptor()
{
#ifdef WIN32
   mSocket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   sockaddr_in loopback;
   memset(&loopback, 0, sizeof(loopback));
   loopback.sin_family = AF_INET;
   loopback.sin_port = 0;
   loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   
   ::bind( mSocket, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback));
   memset(&mWakeupAddr, 0, sizeof(mWakeupAddr));   
   int len = sizeof(mWakeupAddr);
   int error = getsockname(mSocket, (sockaddr *)&mWakeupAddr, &len);
   assert(error == 0);
   error= connect(mSocket, &mWakeupAddr, sizeof(mWakeupAddr)); 
   assert(error == 0);
#else
   pipe(mPipe);
#endif
}

SelectInterruptor::~SelectInterruptor()
{
#ifdef WIN32
   closesocket(mSocket);
#else
   close(mPipe[0]);
   close(mPipe[1]);
#endif
}   

void 
SelectInterruptor::handleProcessNotification()
{
   interrupt();   
}

void 
SelectInterruptor::buildFdSet(FdSet& fdset)
{
#ifdef WIN32
	fdset.setRead(mSocket);
#else
   fdset.setRead(mPipe[0]);
#endif
}

void 
SelectInterruptor::process(FdSet& fdset)
{      
#ifdef WIN32
   if ( fdset.readyToRead(mSocket))
   {
      char rdBuf[16];
      size_t res = recv(mSocket, rdBuf, sizeof(rdBuf), 0);
      assert(res >= 1);
   }
#else
   if ( fdset.readyToRead(mPipe[0]))
   {
      char rdBuf[16];
      size_t res = read(mPipe[0], rdBuf, sizeof(rdBuf));
      assert(res >= 1);
   }
#endif
}

void 
SelectInterruptor::interrupt()
{
   static char* wakeUp = "w";
#ifdef WIN32
   int count = send(mSocket, wakeUp, sizeof(wakeUp), 0);
   assert(count = 1);
#else
   size_t res = write(mPipe[1], wakeUp, sizeof(wakeUp));
   assert(res == sizeof(wakeUp));   
#endif
}
