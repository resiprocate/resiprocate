#ifndef RESIP_AsyncProcessHandler_HXX
#define RESIP_AsyncProcessHandler_HXX

namespace resip
{

//called when an event from outside the process() lookp of the sipstack occurs
//which requires the sipstack to all process()
class AsyncProcessHandler
{
   public:
      virtual void handleProcessNotification() = 0; 
};

}

#endif
