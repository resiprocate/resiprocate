#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/MethodTypes.hxx"
#include <cassert>
namespace resip
{

class DialogUsageManager;
class Dialog;
class DumTimer;
class SipMessage;
class NameAddr;

class BaseUsage
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      int line)
               : BaseException(msg, file, line)
            {}
            
            virtual const char* name() const {return "BaseUsage::Exception";}
      };

      class Handle
      {
         public:
            typedef UInt64 Id;
         protected:
            Handle(DialogUsageManager& dum);
            // throws if not found
            BaseUsage* get();
         private:
            DialogUsageManager* mDum;
            Id mId;
            friend class DialogUsageManager;
            static UInt64 getNext();
      };

      BaseUsage(DialogUsageManager& dum, Dialog& dialog);
      virtual ~BaseUsage();

#if 0
      // For creating requests within a dialog
      SipMessage& makeInvite();
      SipMessage& makeRegister();
      SipMessage& makeSubscribe();
      SipMessage& makeBye();
      SipMessage& makeRefer(const NameAddr& referTo);
      SipMessage& makeNotify();
      SipMessage& makeOptions();
      SipMessage& makePublish();
      SipMessage& makeAck(); // needs to have the corresponding INVITE 
      SipMessage& makeCancel(); // needs to have the corresponding INVITE 
      SipMessage& makeRequest(MethodTypes method);
      SipMessage& makeResponse(int code);
#endif
      
      // to send a request on an existing dialog (made from make... methods above)
      void send(const SipMessage& request);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
      virtual void end()=0;
      virtual void dispatch(const SipMessage& msg) = 0;
      virtual void dispatch(const DumTimer& timer) = 0;
      virtual BaseUsage::Handle getBaseHandle() = 0;

   protected:
      DialogUsageManager& mDum;
      Dialog& mDialog;
};
 
}

#endif
