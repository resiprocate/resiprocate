#include "resiprocate/dum/AppDialog.hxx"

using namespace resip;

AppDialog::AppDialog(HandleManager& ham) : Handled(ham)
{
}

AppDialog::~AppDialog()
{
}

AppDialogHandle 
AppDialog::getHandle()
{
   return AppDialogHandle(mHam, mId);
}
