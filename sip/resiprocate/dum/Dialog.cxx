#include "resiprocate/Contents.hxx"
//#include "resiprocate/OctetContents.hxx"
//#include "resiprocate/HeaderFieldValueList.hxx"
#include "resiprocate/sam/Dialog.hxx"
#include "resiprocate/sam/ClientInviteSession.hxx"

using namespace resip;
using namespace std;

class ServerInviteSession;

Dialog::Dialog(DialogUsageManager& dum, const SipMessage& msg) 
   : mId(msg),
     mDum(dum),
     mLocalCSeq(0),
     mRemoteCSeq(0),
     mCallId(msg.header(h_CallID))
{
   assert(msg.isFromWire());

   if (msg.isRequest()) // UAS
   {
      const SipMessage& request = msg;
      mRouteSet = request.header(h_RecordRoutes);

      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSRIBE:
            if (request.exists(h_Contacts) && request.header(h_Contacts).size() == 1)
            {
               NameAddr& contact = request.header(h_Contacts).front();
               if (isEqualNoCase(contact.uri().scheme(), Symbols::Sips) ||
                   isEqualNoCase(contact.uri().scheme(), Symbols::Sip))
               {
                  mRemoteTarget = contact;
               }
               else
               {
                  InfoLog (<< "Got an INVITE or SUBSCRIBE with invalid scheme");
                  DebugLog (<< request);
                  throw Exception("Invalid dialog", __FILE__, __LINE__);
               }
            }
            else
            {
               InfoLog (<< "Got an INVITE or SUBSCRIBE that doesn't have exactly one contact");
               DebugLog (<< request);
               throw Exception("Invalid dialog", __FILE__, __LINE__);
            }
            break;
      }
      
      mRemoteCSeq = request.header(h_CSeq).sequence();
      mLocalCSeq = 0;
      
      if ( response.header(h_From).exists(p_tag) ) // 2543 compat
      {
         mRemoteTag = response.header(h_From).param(p_tag);  
      }
      if ( response.header(h_To).exists(p_tag) )  // 2543 compat
      {
         mLocalTag = response.header(h_To).param(p_tag); 
      }
      mMe = response.header(h_To);

      //mDialogId = mCallId;
      //mDialogId.param(p_toTag) = mLocalTag;
      //mDialogId.param(p_fromTag) = mRemoteTag;
   }
   else if (msg.isResponse())
   {
      const SipMessage& response = msg;
      if (response.exists(h_RecordRoutes))
      {
         mRouteSet = response.header(h_RecordRoutes).reverse();
      }

      switch (response.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSRIBE:
            if (response.exists(h_Contacts) && response.header(h_Contacts).size() == 1)
            {
               NameAddr& contact = response.header(h_Contacts).front();
               if (isEqualNoCase(contact.uri().scheme(), Symbols::Sips) ||
                   isEqualNoCase(contact.uri().scheme(), Symbols::Sip))
               {
                  mRemoteTarget = contact;
               }
               else
               {
                  InfoLog (<< "Got an INVITE or SUBSCRIBE with invalid scheme");
                  DebugLog (<< response);
                  throw Exception("Invalid dialog", __FILE__, __LINE__);
               }
            }
            else
            {
               InfoLog (<< "Got an INVITE or SUBSCRIBE that doesn't have exactly one contact");
               DebugLog (<< response);
               throw Exception("Invalid dialog", __FILE__, __LINE__);
            }
            break;
      }

      mLocalCSeq = response.header(h_CSeq).sequence();

      if ( response.header(h_From).exists(p_tag) ) // 2543 compat
      {
         mLocalTag = response.header(h_From).param(p_tag);  
      }
      if ( response.header(h_To).exists(p_tag) )  // 2543 compat
      {
         mRemoteTag = response.header(h_To).param(p_tag); 
      }
      mMe = response.header(h_From);

      //mDialogId = mCallId;
      //mDialogId.param(p_toTag) = mLocalTag;
      //mDialogId.param(p_fromTag) = mRemoteTag;

      BaseUsage* usage = mCreator->makeUsage(response);
      assert(usage);
      mUsages.push_back(usage);
   }
}

void
Dialog::dispatch(const SipMessage& msg)
{
   BaseUsage* usage = findUsage(msg);
   if (usage)
   {
      usage->dispatch(msg);
   }
   else
   {
      if (msg.isRequest())
      {
         switch (msg.header(h_CSeq).method())
         {
            case INVITE:  // new INVITE
               usage = mDum.createServerInviteSession(msg);
               break;
               
            case ACK:
            case CANCEL:
               InfoLog (<< "Drop stray ACK or CANCEL in dialog on the floor");
               DebugLog (<< msg);
               break;

            case SUBSCRIBE:
               
               
         }
      }
      else if (msg.isResponse())
      {
      }
      
      BaseUsage* usage = mCreator->makeUsage(msg);
      assert(usage);
      mUsages.push_back(usage);
   }
}

BaseUsage* 
Dialog::findUsage(const SipMessage& msg)
{
   switch (msg.header(h_CSeq).method())
   {
      case INVITE:  // new INVITE
         return mInviteSession;
      case SUBSCRIBE:
      case REFER: 
      case NOTIFY: 
         break;
      case REGISTER:
         assert(0);
      case PUBLISH:
         break;                       
      case MESSAGE :
      case OPTIONS :
      case INFO :   
         break;
   }
}

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
Dialog::dispatch(const SipMessage& msg)
{
   if (msg.isRequest())
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
   else if (msg.isResponse())
   {
      
   }
   else
   {
      assert(0);
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
