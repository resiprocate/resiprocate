#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"

using namespace resip;

AppDialogSet::AppDialogSet(DialogUsageManager& dum) : 
   Handled(dum),
   mDum(&dum),
   mDialogSetId(Data::Empty, Data::Empty)
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
