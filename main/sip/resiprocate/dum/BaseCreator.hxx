#if !defined(RESIP_BASECREATOR_HXX)
#define RESIP_BASECREATOR_HXX

namespace resip
{

class BaseCreator
{
   public:
      BaseCreator(DialogUsageManager& dum);
      SipMessage& getLastRequest();
      void dispatch(SipMessage& msg)=0;
      
   protected:
      void makeInitialRequest(const NameAddr& target, MethodTypes method);
      
      // this will get updated when an initial request is challenged. where we
      // store the credentials and last cseq
      SipMessage mLastRequest;
      DialogUsageManager& mDum;
};

}

#endif
