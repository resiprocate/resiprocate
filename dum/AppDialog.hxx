#if !defined(RESIP_APPDIALOG_HXX)
#define RESIP_APPDIALOG_HXX

#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/dum/DialogId.hxx"


#include <vector>

namespace resip
{

class HandleManager;
class Dialog;
class Data;

class AppDialog : public Handled
{
   public:
      AppDialog(HandleManager& ham);
      virtual ~AppDialog();

      AppDialogHandle getHandle();
      DialogId getDialogId() const;

      //?dcm? -- further evidence that this should possbily be a dialog
      //subclass(cancel gets tricky). List vs vector?(here and in Dialog)
      std::vector<ClientSubscriptionHandle> getClientSubscriptions();
      std::vector<ClientSubscriptionHandle> findClientSubscriptions(const Data& event);

      std::vector<ServerSubscriptionHandle> getServerSubscriptions();
      std::vector<ServerSubscriptionHandle> findServerSubscriptions(const Data& event);

      //returns an invalid handle if there is no session
      InviteSessionHandle getInviteSession();
   private:
      friend class DialogSet;      
      Dialog* mDialog;      
};

}

#endif
