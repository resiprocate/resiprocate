#if !defined(RESIP_ASYNC_PROCESS_NOTIFIER_DNS_HXX)
#define RESIP_ASYNC_PROCESS_NOTIFIER_DNS_HXX

namespace resip
{
class ProcessNofificationHandler;

class ProcessNotifier
{
   public:
      class Handler
      {
         public:
            virtual void handleProcessNotification() = 0; 
      };
         
      ProcessNotifier(ProcessNotifier::Handler* handler) : mHandler(handler) {}

      void setHandler(ProcessNotifier::Handler* handler)
      {
          mHandler = handler;
      }
   protected:
      void notify()
      {
         mHandler->handleProcessNotification();
      }
   private:
      ProcessNotifier::Handler* mHandler;
};

}

#endif
