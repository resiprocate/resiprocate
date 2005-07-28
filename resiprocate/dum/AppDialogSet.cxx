#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;

AppDialogSet::AppDialogSet(DialogUsageManager& dum) : 
   Handled(dum),
   mDum(dum),
   mDialogSet(0)
{
}

AppDialogSet::~AppDialogSet()
{
}

AppDialogSetHandle 
AppDialogSet::getHandle()
{
   return AppDialogSetHandle(mHam, mId);
}

void
AppDialogSet::destroy()
{
   delete this;
}

void 
AppDialogSet::cancel()
{
   if(mDialogSet)
   {
      mDialogSet->cancel();
   }
}

UserProfile*  
AppDialogSet::selectUASUserProfile(const SipMessage&)
{
   // Default is Master Profile - override this method to select a different userProfile for UAS DialogSets
   return mDum.getMasterProfile();
}

UserProfile* 
AppDialogSet::getUserProfile()
{
   if(mDialogSet)
   {
      return mDialogSet->getUserProfile();
   }
   else
   {
      return 0;
   }
}

AppDialog* 
AppDialogSet::createAppDialog(const SipMessage&)
{
   return new AppDialog(mDum);
}

DialogSetId 
AppDialogSet::getDialogSetId()
{
   if(mDialogSet)
   {
       return mDialogSet->getId();
   }
   else
   {
       return DialogSetId(Data::Empty, Data::Empty);
   }
}


std::ostream& 
AppDialogSet::dump(std::ostream& strm) const
{
   strm << "AppDialogSet " << mId;
   return strm;
}
