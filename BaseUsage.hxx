#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

namespace resip
{
class BaseUsage
{
   public:
      BaseUsage(SAManager& sam);
      
      SAManager& sam();
      Dialog& dialog();
      
   private:
      SAManager& mSAM;
      DialogImpl& mDialog;
};
 
}

#endif
