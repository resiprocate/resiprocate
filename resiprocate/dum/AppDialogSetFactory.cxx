#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;

AppDialogSet* 
AppDialogSetFactory::createAppDialogSet(DialogUsageManager& dum, const SipMessage&)
{
   //!dcm! -- inefficient, but state may creep in and the handles add trickyness
   return new AppDialogSet(dum); 
}

