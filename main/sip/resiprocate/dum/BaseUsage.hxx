#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

namespace resip
{
class BaseUsage
{
   public:
      BaseUsage(DialogUsageManager& dum);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
   private:
      DialogUsageManager& mDUM;
      DialogImpl& mDialog;
};
 
}

#endif
