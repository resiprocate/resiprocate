#if !defined(RESIP_BASECREATOR_HXX)
#define RESIP_BASECREATOR_HXX

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/UserProfile.hxx"

namespace resip
{

class DialogUsageManager;

class BaseCreator
{
   public:
      BaseCreator(DialogUsageManager& dum, UserProfile& userProfile);
      virtual ~BaseCreator();
      SipMessage& getLastRequest();
      UserProfile& getUserProfile();
      const SipMessage& getLastRequest() const;
      
   protected:
      void makeInitialRequest(const NameAddr& target, MethodTypes method);
      
      // this will get updated when an initial request is challenged. where we
      // store the credentials and last cseq
      SipMessage mLastRequest;
      DialogUsageManager& mDum;
      UserProfile& mUserProfile;
};

}

#endif
