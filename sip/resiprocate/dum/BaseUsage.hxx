#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

namespace resip
{
class BaseUsage
{
   public:
      class Exception : BaseException
      {
      };

      BaseUsage(DialogUsageManager& dum);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
      virtual void end();

   private:
      DialogUsageManager& mDUM;
      DialogImpl& mDialog;
};
 
}

#endif
