#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"

using namespace resip;

AppDialogSet::AppDialogSet(DialogUsageManager& dum) : 
   Handled(dum),
   mDum(dum),
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

void
AppDialogSet::destroy()
{
   delete this;
}

void 
AppDialogSet::cancel()
{
   mDum.cancel(mDialogSetId);   
}

AppDialog* 
AppDialogSet::createAppDialog(const SipMessage&)
{
   return new AppDialog(mDum);
}

const DialogSetId& 
AppDialogSet::getDialogSetId()
{
   return mDialogSetId;   
}

