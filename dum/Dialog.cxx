#include "resiprocate/Contents.hxx"
//#include "resiprocate/OctetContents.hxx"
//#include "resiprocate/HeaderFieldValueList.hxx"
#include "resiprocate/sam/Dialog.hxx"
#include "resiprocate/sam/ClientInviteSession.hxx"

using namespace resip;
using namespace std;

class ServerInviteSession;

DialogId Dialog::getId() const
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
        usage = it->next();
        if ((dynamic_cast<ClientInviteSession*>(usage) != NULL) ||
            (dynamic_cast<ServerInviteSession*>(usage) != NULL))
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
Dialog::process(const SipMessage& msg)
{
   switch (request.header(h_RequestLine).getMethod())
   {
      // a NOTIFY is the only request that can
      // create a full dialog (when the 2xx from the
      // SUBSCRIBE arrives *after* the NOTIFY
      case NOTIFY : 
         processNotify(msg);
         break;

      case REFER: 
         // !jf! wierdo
         // affects an InvSession and a ServerSubscription 
         break;

      case SUBSCRIBE:
         processSubscribe(msg);
         break;

      case CANCEL: 
         // should only occur when canceling a re-INVITE
      case INVITE:  
         // should only occur for a re-INVITE
      case ACK:
      case PRACK:
      case BYE:
      case UPDATE:
      case INFO: 
         processInviteRelated(msg);
         break;
         
      case REGISTER:
      {
         assert(0); // already handled
         break;
      }
      
      case PUBLISH:
         assert(0);
         break;                       

      case MESSAGE:
      case OPTIONS:
         assert(0);
         break;
         
      default:
         assert(0);
         break;
      }
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

void
Dialog::processSubscribe(const SipMessage& subscribe)
{
   for (std::list<BaseUsage*>::iterator i=mUsages.begin(); i!=mUsages.end(); i++)
   {
      ServerSubscription* sub = dynamic_cast<ServerSubscription*>(*i);
      if (sub && sub->matches(subscribe))
      {
         sub->process(subscribe); // a resubscribe or unsubscribe
         return;
      }
   }
   
   // a subscribe on an existing dialog with a different BaseUsage
   ServerSubscription* sub = new ServerSubscription(mDum, subscribe);
}

void
Dialog::processInviteRelated(const SipMessage& msg)
{
   for (std::list<BaseUsage*>::iterator i=mUsages.begin(); i!=mUsages.end(); i++)
   {
      ServerInvSession* server = dynamic_cast<ServerInvSession*>(*i);
      ClientInvSession* client = dynamic_cast<ClientInvSession*>(*i);
      if (server) 
      {
         server->process(msg);
         break;
      }
      else if (client)
      {
         client->process(msg);
         break;
      }
   }
}
