#if !defined(RESIP_APPDIALOG_HXX)
#define RESIP_APPDIALOG_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/Handled.hxx"

namespace resip
{

class HandleManager;

class AppDialog : public Handled
{
   public:
      AppDialog(HandleManager& ham);
      virtual ~AppDialog();

      AppDialogHandle getHandle();
};

}

#endif
