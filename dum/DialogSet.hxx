#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include <map>

#include "resiprocate/dum/DialogId.hxx"
#include "resiprocate/dum/DialogSetId.hxx"
#include "resiprocate/dum/MergedRequestKey.hxx"

namespace resip
{

class BaseCreator;
class Dialog;
class DialogUsageManager;
class AppDialogSet;

class DialogSet
{
   public:
      DialogSet(BaseCreator* creator, DialogUsageManager& dum);
      DialogSet(const SipMessage& request, DialogUsageManager& dum);
      virtual ~DialogSet();
      
      DialogSetId getId();
      void addDialog(Dialog*);
      bool empty() const;
      BaseCreator* getCreator();

      void cancel();
      void dispatch(const SipMessage& msg);
      
   private:
      friend class Dialog;
      friend class BaseUsage;
      
      Dialog* findDialog(const SipMessage& msg);
      Dialog* findDialog(const DialogId id);

      MergedRequestKey mMergeKey;
      typedef std::map<DialogId,Dialog*> DialogMap;
      DialogMap mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
      DialogUsageManager& mDum;
      bool mCancelled;
      AppDialogSet* mAppDialogSet;
};
 
}

#endif
