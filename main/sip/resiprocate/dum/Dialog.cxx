#include "resiprocate/sam/nnDialog.hxx"

using namespace resip;
using namespace std;

DialogId Dialog;:getId() const
{
    return mId;
}

BaseUsage&
Dialog::findInvSession()
{
    std::list<BaseUsage*>::iterator it = mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if ((dynamic_cast<ClientInviteSession*>(usage) != null) ||
            (dynamic_cast<ServerInviteSession*>(usage) != null))
        {
            return *usage;
        }
    }
    return  BaseUsage::empty();
}

UsageSet 
Dialog::findSubscriptions()
{
    std::list<BaseUsage*>::iterator it = mUsages.begin();
    BaseUsage *usage;
    UsageSet usageSet;
    while (it != mUsages.end())
    {
        usage = it.next();
        if ((dynamic_cast<ClientSubscription*>(usage) != null) ||
            (dynamic_cast<ServerSubscription*>(usage) != null))
        {
            usageSet.push_back(*usage);
        }
    }
    return usageSet:
}

BaseUsage&
Dialog::findRegistration()
{
    std::list<BaseUsage*>::iterator it = mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if ((dynamic_cast<CientRegistration*>(usage) != null) ||
            (dynamic_cast<ServerRegistration*>(usage) != null))
        {
            return *usage;
        }
    }
    return  BaseUsage::empty();
}


BaseUsage&
Dialog::findPublication()
{
    std::list<BaseUsage*>::iterator it = mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if ((dynamic_cast<CientPublication*>(usage) != null) ||
            (dynamic_cast<ServerPublication*>(usage) != null))
        {
            return *usage;
        }
    }
    return  BaseUsage::empty();
}

UsageSet 
Dialog::findOutOfDialogs()
{
    std::list<BaseUsage*>::iterator it = mUsages.begin();
    BaseUsage *usage;
    UsageSet usageSet;
    while (it != mUsages.end())
    {
        usage = it.next();
        if ((dynamic_cast<ClientOutOfDialogReq*>(usage) != null) ||
            (dynamic_cast<ServerOutOfDialogReq*>(usage) != null))
        {
            usageSet.push_back(*usage);
        }
    }
    return usageSet:
}

void
Dialog::processNotify(const SipMessage& notify)
{
   if (notify.isRequest())
   {
      if (findSubscriptions().empty())
      {
         SubscriptionCreator* creator = dynamic_cast<SubscriptionCreator*>(DialogSetId(notify).getCreator());
         if (creator)
         {
            creator->makeNewSubscription(notify);
         }
      }
      else
      {
         for (std::list<BaseUsage*>::iterator i=mUsages.begin(); i!=mUsages.end(); i++)
         {
            ClientSubscription* sub = dynamic_cast<ClientSubscription*>(*i);
            if (sub && sub->matches(notify))
            {
               sub->process(notify);
               break;
            }
         }
      }
   }
}
