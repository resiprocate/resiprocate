#if !defined(RESIP_DIALOGUSAGE_HXX)
#define RESIP_DIALOGUSAGE_HXX

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DialogUsageManager;
class Dialog;
class DumTimeout;
class SipMessage;
class NameAddr;

class DialogUsage : public BaseUsage
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
   protected:
      friend class DialogSet;
      friend class DialogUsageManager;
      
      DialogUsage(DialogUsageManager& dum, Dialog& dialog);
      virtual ~DialogUsage();
      
      Dialog& mDialog;
};
 
}

#endif
