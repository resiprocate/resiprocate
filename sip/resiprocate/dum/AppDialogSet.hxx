#if !defined(RESIP_APPDIALOGSET_HXX)
#define RESIP_APPDIALOGSET_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/Handled.hxx"

namespace resip
{

class HandleManager;

class AppDialogSet : public Handled
{
   public:
      AppDialogSet(HandleManager& ham);
      virtual ~AppDialogSet();
      
      AppDialogSetHandle getHandle();
};

}

#endif
