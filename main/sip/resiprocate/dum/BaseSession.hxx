#if !defined(RESIP_BASESESSION_HXX)
#define RESIP_BASESESSION_HXX

namespace resip
{
class BaseSession
{
   public:
      BaseSession(SAManager& sam);
      
      SAManager& sam();
      Dialog& dialog();
      
   private:
      SAManager& mSAM;
      DialogImpl& mDialog;
};
 
}

#endif
