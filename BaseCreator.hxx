#if !defined(RESIP_BASECREATOR_HXX)
#define RESIP_BASECREATOR_HXX

namespace resip
{

class BaseCreator
{
   public:
      BaseCreator(SAManager& sam);
      
   private:
      SipMessage mInitialRequest;
      SAManager& mSAM;
};

}

#endif
