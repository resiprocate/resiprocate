#if !defined(RESIP_BASECREATOR_HXX)
#define RESIP_BASECREATOR_HXX

namespace resip
{

class BaseCreator
{
   public:
      BaseCreator(DialogUsageManager& dum);
      
   private:
      SipMessage mInitialRequest;
      DialogUsageManager& mDUM;
};

}

#endif
