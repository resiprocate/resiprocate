#if !defined(RESIP_BASECREATOR_HXX)
#define RESIP_BASECREATOR_HXX

namespace resip
{

class BaseCreator
{
   public:
      BaseCreator(DialogUsageManager& dum);
      SipMessage& getLastRequest();
      
   private:
      // this will get updated when an initial request is challenged. where we
      // store the credentials and last cseq
      SipMessage mLastRequest;
      DialogUsageManager& mDUM;
};

}

#endif
