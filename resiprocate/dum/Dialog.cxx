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
    std::list<BaseUsage*>::iterator it  =  mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if (dynamic_cast<ClientInviteSession*>(usage) != null)
        {
            return *usage;
        }
    }
    // FixMe: is it possible not to have an invite session?
    assert(0);
}

UsageSet 
Dialog::findSubscriptions()
{
    std::list<BaseUsage*>::iterator it  =  mUsages.begin();
    BaseUsage *usage;
    UsageSet usageSet;
    while (it != mUsages.end())
    {
        usage = it.next();
        if (dynamic_cast<ClientSubscription*>(usage) != null)
        {
            usageSet.push_back(*usage);
        }
    }
    return usageSet:
}

BaseUsage&
Dialog::findRegistration()
{
    std::list<BaseUsage*>::iterator it  =  mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if (dynamic_cast<CientRegistration*>(usage) != null)
        {
            return *usage;
        }
    }
    // FixMe: is it possible not to have a registration?
    assert(0);
}


BaseUsage&
Dialog::findPublication()
{
    std::list<BaseUsage*>::iterator it  =  mUsages.begin();
    BaseUsage *usage;
    while (it != mUsages.end())
    {
        usage = it.next();
        if (dynamic_cast<CientPublication*>(usage) != null)
        {
            return *usage;ppppppppp
        }
    }
    // FixMe: is it possible not to have a publication?
    assert(0);
}

UsageSet 
Dialog::findOutOfDialogs()
{
    std::list<BaseUsage*>::iterator it  =  mUsages.begin();
    BaseUsage *usage;
    UsageSet usageSet;
    while (it != mUsages.end())
    {
        usage = it.next();
        if (dynamic_cast<ClientOutOfDialogReq*>(usage) != null)
        {
            usageSet.push_back(*usage);
        }
    }
    return usageSet:
}
