#if !defined(RESIP_APPDIALOGSETFACTORY_HXX)
#define RESIP_APPDIALOGSETFACTORY_HXX


namespace resip
{

class AppDialogSet;

class AppDialogSetFactory
{
   public:
      virtual AppDialogSet* createAppDialogSet(const DialogUsageManager&, const SipMessage&);      
};

}

#endif
