#if !defined(RESIP_NONDIALOGUSAGE_HXX)
#define RESIP_NONDIALOGUSAGE_HXX

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DialogUsageManager;
class DialogSet;
class DumTimeout;
class SipMessage;
class NameAddr;

class NonDialogUsage : public BaseUsage
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,const Data& file,int line);
            virtual const char* name() const;
      };

      AppDialogSetHandle getAppDialogSet();
   protected:
      NonDialogUsage(DialogUsageManager& dum, DialogSet& dialogSet);
      virtual ~NonDialogUsage();
      
      DialogSet& mDialogSet;
};
 
}

#endif
