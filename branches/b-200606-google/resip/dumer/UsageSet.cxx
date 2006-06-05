#include "UsageSet.hxx"

UsageSet::UsageSet(SharedPtr<UserProfile> userProfile) 
   : Prd(userProfile)
{}

UsageSet::~UsageSet()
{
}

void
UsageSet::dispatch(const SipMessage& message) 
{
   if (!dispatchFilter(message)) {
      DebugLog(<< "discard stray response, no dialog" << endl << msg);
   }
}

bool
UsageSet::dispatchFilter(const SipMessage& message) 
{
   // child usage want the message?
   for (std::list< SharedPtr<DialogUsage> >::const_iterator i = mChildUsages.begin();
        i != mChildUsages.end(); ++i) {
      if (i->isForMe(msg)) {
         DebugLog(<< "found child to handle message " << *i << endl << msg);
         i->dispatch(msg);
         return true;
      }
   }

   // dialog wants the request?
   for (std::list< WeakdPtr<Dialog> >::const_iterator i = mDialogs.begin();
        i != mDialogss.end(); ++i) 
   {
      SharedPtr<Dialog> dialog(*i);
      if (dialog.get()
          && dialog->!isDestroying()
          && dialog->isForMe(msg)) 
      {
         if (msg->isRequest()) 
         {
            DebugLog(<< "dialog message " << *dialog << endl << msg);
            std::auto_ptr<DialogUsage> child(mPrdManager.createDialogUsage(msg));
            if (child.get()) 
            {
               child->setDialog(dialog);
               addChild(SharedPtr<DialogUsage>(child.release()));
               child->dispatch(msg);
               return true;
            }
            else 
            {
               DebugLog(<< "did not create usage");
            }
         }
         else 
         {
            DebugLog(<< "discard stray response " << *dialog << endl << msg);
         }
         break;
      }
   }
   return false;
}
