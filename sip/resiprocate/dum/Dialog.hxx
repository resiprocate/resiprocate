#if !defined(RESIP_CLIENTDIALOG_HXX)
#define RESIP_CLIENTDIALOG_HXX

namespace resip
{

/** @file Dialog.hxx
 *   @todo This file is empty
 */

class Dialog 
{
   public:

      DialogId getId() const;
      
      BaseUsage& findInvSession();
      UsageSet   findSubscriptions();
      BaseUsage& findRegistration();
      BaseUsage& findPublication();
      UsageSet   findOutOfDialogs();

   private:
      std::list<BaseUsage*> mUsages;
};
 
}


#endif
