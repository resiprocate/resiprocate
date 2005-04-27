#ifndef RESIP_SelectInterruptor_HXX
#define RESIP_SelectInterruptor_HXX

#include "resiprocate/os/AsyncProcessHandler.hxx"
#include "resiprocate/os/Socket.hxx"

#if 0
#if defined(WIN32)
#include <Ws2tcpip.h>
#else
#include <netinet/in.h>
#endif
#endif

namespace resip
{

class SelectInterruptor : public AsyncProcessHandler
{
   public:
      SelectInterruptor();
      virtual ~SelectInterruptor();
      
      virtual void handleProcessNotification(); 
      
      void interrupt();      
      void buildFdSet(FdSet& fdset);
      void process(FdSet& fdset);
   private:
#ifndef WIN32
      int mPipe[2];
#else
      Socket mSocket;
      sockaddr mWakeupAddr;
#endif
         
};

}

#endif
