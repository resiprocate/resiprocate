#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/Dialog.hxx"

using namespace resip;
using namespace std;

AppDialog::AppDialog(HandleManager& ham) : 
   Handled(ham),
   mDialog(0)
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

vector<ClientSubscriptionHandle> 
AppDialog::getClientSubscriptions()
{
   return mDialog->getClientSubscriptions();
}

vector<ClientSubscriptionHandle> 
AppDialog::findClientSubscriptions(const Data& event)
{
   return mDialog->findClientSubscriptions(event);
}

vector<ServerSubscriptionHandle> 
AppDialog::getServerSubscriptions()
{
   return mDialog->getServerSubscriptions();
}


vector<ServerSubscriptionHandle> 
AppDialog::findServerSubscriptions(const Data& event)
{
   return mDialog->findServerSubscriptions(event);
}

InviteSessionHandle 
AppDialog::getInviteSession()
{
   return mDialog->getInviteSession();
}

DialogId 
AppDialog::getDialogId() const
{
   return mDialog->getId();   
}

std::ostream& 
AppDialog::dump(std::ostream& strm) const
{
   strm << "AppDialog " << mId;
   return strm;
}
