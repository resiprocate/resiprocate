#include "resiprocate/dum/AppDialogSetFactory.hxx"

AppDialogSet* 
AppDialogSetFactory::createAppDialogSet(const DialogUsageManager& dum, const SipMessage&)
{
   //!dcm! -- inefficient, but state may creep in and the handles add trickyness
   return new AppDialogSet();   
}

