#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include <map>
#include <list>

#include "resiprocate/dum/DialogId.hxx"
#include "resiprocate/dum/DialogSetId.hxx"
#include "resiprocate/dum/MergedRequestKey.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class BaseCreator;
class Dialog;
class DialogUsageManager;
class AppDialogSet;
class ClientOutOfDialogReq;
class UserProfile;

class DialogSet
{
   public:
      DialogSet(BaseCreator* creator, DialogUsageManager& dum);
      DialogSet(const SipMessage& request, DialogUsageManager& dum);
      virtual ~DialogSet();
      
      DialogSetId getId();
      void addDialog(Dialog*);
      bool empty() const;
      BaseCreator* getCreator();

      UserProfile* getUserProfile();
      void setUserProfile(UserProfile *userProfile);

      void cancel();
      void dispatch(const SipMessage& msg);
      
      ClientRegistrationHandle getClientRegistration();
      ServerRegistrationHandle getServerRegistration();
      ClientPublicationHandle getClientPublication();
      ClientOutOfDialogReqHandle getClientOutOfDialog();
      ServerOutOfDialogReqHandle getServerOutOfDialog();

   private:
      friend class Dialog;
      friend class DialogUsage;
      friend class ClientInviteSession;
      friend class NonDialogUsage;
      friend class DialogUsageManager;      
      friend class ClientRegistration;
      friend class ServerRegistration;
      friend class ClientOutOfDialogReq;
      friend class ServerOutOfDialogReq;
      friend class ClientPublication;
      friend class RedirectManager;
      friend class ClientPagerMessage;
      friend class ServerPagerMessage;
      
      void possiblyDie();

      Dialog* findDialog(const SipMessage& msg);
      Dialog* findDialog(const DialogId id);

      ClientOutOfDialogReq* findMatchingClientOutOfDialogReq(const SipMessage& msg);

      ClientRegistration* makeClientRegistration(const SipMessage& msg);
      ClientPublication* makeClientPublication( const SipMessage& msg);
      ClientOutOfDialogReq* makeClientOutOfDialogReq(const SipMessage& msg);

      ServerRegistration* makeServerRegistration(const SipMessage& msg);
      ServerOutOfDialogReq* makeServerOutOfDialog(const SipMessage& msg);
      
      ServerPagerMessage* makeServerPagerMessage(const SipMessage& request);      

      MergedRequestKey mMergeKey;
      typedef std::map<DialogId,Dialog*> DialogMap;
      DialogMap mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
      DialogUsageManager& mDum;
      AppDialogSet* mAppDialogSet;
      bool mCancelled;
      bool mReceivedProvisional;      

      //inelegant, but destruction can happen both automatically and forced by
      //the user.  Extremely single threaded.
      bool mDestroying;

      ClientRegistration* mClientRegistration;
      ServerRegistration* mServerRegistration;
      ClientPublication* mClientPublication;
      std::list<ClientOutOfDialogReq*> mClientOutOfDialogRequests;
      ServerOutOfDialogReq* mServerOutOfDialogRequest;

      ClientPagerMessage* mClientPagerMessage;
      ServerPagerMessage* mServerPagerMessage;
      UserProfile* mUserProfile;
};
 
}

#endif
