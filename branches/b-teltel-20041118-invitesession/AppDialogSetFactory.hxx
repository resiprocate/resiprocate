#if !defined(RESIP_APPDIALOGSETFACTORY_HXX)
#define RESIP_APPDIALOGSETFACTORY_HXX


namespace resip
{

class AppDialogSet;
class DialogUsageManager;
class SipMessage;

class AppDialogSetFactory
{
   public:
      virtual AppDialogSet* createAppDialogSet(DialogUsageManager&, const SipMessage&);      
      virtual ~AppDialogSetFactory() {}
};

}

#endif
