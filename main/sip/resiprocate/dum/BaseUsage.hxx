#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DialogUsageManager;
class Dialog;
class DumTimeout;
class SipMessage;
class NameAddr;

class BaseUsage : public Handled
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,const Data& file,int line);
            virtual const char* name() const;
      };

      AppDialogSetHandle getAppDialogSet();
      AppDialogHandle getAppDialog();
     
      virtual void send(SipMessage& request);

   protected:
      BaseUsage(DialogUsageManager& dum, Dialog& dialog);
      virtual ~BaseUsage();

      virtual void dispatch(const SipMessage& msg) = 0;
      virtual void dispatch(const DumTimeout& timer) = 0;
      
      BaseUsageHandle getBaseHandle();

      DialogUsageManager& mDum;
      Dialog& mDialog;

   private:
      BaseUsageHandle mHandle;

      friend class DialogUsageManager;
};
 
}

#endif
