#include "resiprocate/dum/AppDialogSet.hxx"

using namespace resip;

AppDialogSet::AppDialogSet(HandleManager& ham) : Handled(ham)
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
