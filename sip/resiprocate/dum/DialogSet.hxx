#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogSetId.hxx"

namespace resip
{

class BaseCreator;
class DialogUsageManager;

/** @file DialogSet.hxx
 * 
 */

class DialogSet
{
   public:
      DialogSet(BaseCreator* creator, DialogUsageManager& dum);
      DialogSet(const SipMessage& request, DialogUsageManager& dum);
      virtual ~DialogSet();
      
      DialogSetId getId();
      
      void addDialog(Dialog*);
      void removeDialog(const Dialog*);
      
//!dcm! -- kill      DialogIdSet getDialogs() const;
      bool empty() const;

//!dcm! -- kill
//      Dialog* findDialog(const DialogId id);
//      Dialog* findDialog(const Data& otherTag);
      
      BaseCreator* getCreator();

      void cancel(const SipMessage& cancelMsg);
      void dispatch(const SipMessage& msg);

      bool mergeRequest(const SipMessage& request);

   private:
      Dialog* findDialog(const SipMessage& msg);
      Dialog* findDialog(const DialogId id);

      std::list<Dialog*> mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
      DialogUsageManager& mDum;
};
 
}

#endif
