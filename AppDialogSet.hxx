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

      // by default, calls the destructor. application can override this if it
      // wants to manage memory on its own. 
      virtual void destroy();

      virtual void cancel();
      virtual AppDialog* createAppDialog(const SipMessage&);
      AppDialogSetHandle getHandle();
      const DialogSetId& getDialogSetId();

   protected:
      DialogUsageManager& mDum;      
      virtual ~AppDialogSet();

   private:
      friend class DialogUsageManager;
      DialogSetId mDialogSetId;
};

}

#endif
