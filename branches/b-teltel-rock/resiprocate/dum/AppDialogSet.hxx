#if !defined(RESIP_APPDIALOGSET_HXX)
#define RESIP_APPDIALOGSET_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/DialogSetId.hxx"
#include "resiprocate/dum/UserProfile.hxx"

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

      virtual UserProfile* getUserProfile();

      virtual AppDialog* createAppDialog(const SipMessage&);

      AppDialogSetHandle getHandle();
      DialogSetId getDialogSetId();

      virtual std::ostream& dump(std::ostream& strm) const;

   protected:
      DialogUsageManager& mDum;      
      virtual ~AppDialogSet();
      // This is called by the DialogUsageManager to select an userProfile to assign to a UAS DialogSet.
      // The application should not call this directly, but should override it, in order to assign 
      // an userProfile other than the MasterProfile
      virtual UserProfile* selectUASUserProfile(const SipMessage&); 

   private:
      friend class DialogUsageManager;
      DialogSet* mDialogSet;
};

}

#endif
