#if !defined(RESIP_APPDIALOGSET_HXX)
#define RESIP_APPDIALOGSET_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/dum/DialogSetId.hxx"

namespace resip
{

class SipMessage;
class DialogUsageManager;

class AppDialogSet : public Handled
{
   public:
      AppDialogSet(DialogUsageManager& dum);

      virtual ~AppDialogSet();
      virtual void cancel();
      virtual AppDialog* createAppDialog(const SipMessage&);
      AppDialogSetHandle getHandle();
   protected:
      DialogUsageManager& mDum;      
  private:
      friend class DialogUsageManager;
      DialogSetId mDialogSetId;
};

}

#endif
