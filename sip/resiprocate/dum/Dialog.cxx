#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/os/Logger.hxx"

#include "BaseCreator.hxx"
#include "Dialog.hxx"
#include "DialogUsageManager.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

class ServerInviteSession;

Dialog::Dialog(DialogUsageManager& dum, const SipMessage& msg) 
   : mId(msg),
     mDum(dum),
     mClientSubscriptions(),
     mServerSubscription(0),
     mInviteSession(0),
     mClientRegistration(0),
     mServerRegistration(0),
     mClientPublication(0),
     mServerPublication(0),
     mClientOutOfDialogReq(0),
     mServerOutOfDialogReq(0),
     mType(Fake),
     mLocalTag(),
     mRemoteTag(),
     mCallId(msg.header(h_CallID)),
     mRouteSet(),
     mLocalContact(),
     mLocalCSeq(0),
     mRemoteCSeq(0),
     mRemoteTarget()
{
   assert(msg.isExternal());

   if (msg.isRequest()) // UAS
   {
      const SipMessage& request = msg;

      switch (request.header(h_CSeq).method())
      {
         case INVITE:
            mType = Invitation;
            break;
            
         case SUBSCRIBE:
         case REFER:
         case NOTIFY:
            mType = Subscription;
            break;

         default:
            mType = Fake;
      }
     
      mRouteSet = request.header(h_RecordRoutes); // !jf! is this right order

      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSCRIBE:
         case REFER:
            if (request.exists(h_Contacts) && request.header(h_Contacts).size() == 1)
            {
               const NameAddr& contact = request.header(h_Contacts).front();
               if (isEqualNoCase(contact.uri().scheme(), Symbols::Sips) ||
                   isEqualNoCase(contact.uri().scheme(), Symbols::Sip))
               {
                  mLocalContact = NameAddr(request.header(h_RequestLine).uri()); // update later when send a request 
                  mRemoteTarget = contact;
               }
               else
               {
                  InfoLog(<< "Got an INVITE or SUBSCRIBE with invalid scheme");
                  DebugLog(<< request);
                  throw Exception("Invalid scheme in request", __FILE__, __LINE__);
               }
            }
            else
            {
               InfoLog (<< "Got an INVITE or SUBSCRIBE that doesn't have exactly one contact");
               DebugLog (<< request);
               throw Exception("Too many (or no contact) contacts in request", __FILE__, __LINE__);
            }
            break;
         default:
            break;
      }
      
      mRemoteCSeq = request.header(h_CSeq).sequence();
      mLocalCSeq = 1;
      
      if (request.header(h_From).exists(p_tag) ) // 2543 compat
      {
         mRemoteTag = request.header(h_From).param(p_tag);  
      }
      if ( request.header(h_To).exists(p_tag) )  // 2543 compat
      {
         mLocalTag = request.header(h_To).param(p_tag); 
      }
   }
   else if (msg.isResponse())
   {
      const SipMessage& response = msg;

      switch (msg.header(h_CSeq).method())
      {
         case INVITE:
            mType = Invitation;
            break;
            
         case SUBSCRIBE:
         case REFER:
            mType = Subscription;
            break;

         default:
            mType = Fake;
      }

      if (response.exists(h_RecordRoutes))
      {
         mRouteSet = response.header(h_RecordRoutes).reverse();
      }

      switch (response.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSCRIBE:
         case REFER:
            if (response.exists(h_Contacts) && response.header(h_Contacts).size() == 1)
            {
               const NameAddr& contact = response.header(h_Contacts).front();
               if (isEqualNoCase(contact.uri().scheme(), Symbols::Sips) ||
                   isEqualNoCase(contact.uri().scheme(), Symbols::Sip))
               {
                  BaseCreator& creator = mDum.findCreator(mId);
                  mLocalContact = creator.getLastRequest().header(h_Contacts).front();
                  mRemoteTarget = contact;
               }
               else
               {
                  InfoLog (<< "Got an INVITE or SUBSCRIBE with invalid scheme");
                  DebugLog (<< response);
                  throw Exception("Bad scheme in contact in response", __FILE__, __LINE__);
               }
            }
            else
            {
               InfoLog (<< "Got an INVITE or SUBSCRIBE that doesn't have exactly one contact");
               DebugLog (<< response);
               throw Exception("Too many contacts (or no contact) in response", __FILE__, __LINE__);
            }
            break;
         default:
            break;
      }

      mLocalCSeq = response.header(h_CSeq).sequence();
      mRemoteCSeq = 0;
      
      if ( response.header(h_From).exists(p_tag) ) // 2543 compat
      {
         mLocalTag = response.header(h_From).param(p_tag);  
      }
      if ( response.header(h_To).exists(p_tag) )  // 2543 compat
      {
         mRemoteTag = response.header(h_To).param(p_tag); 
      }
   }
}

DialogId
Dialog::getId() const
{
   return mId;
}

// !dlb! merge this stub in
#if 0
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
#endif

void
Dialog::dispatch(const SipMessage& msg)
{
   BaseUsage* usage = findUsage(msg);
   if (usage)
   {
      usage->dispatch(msg);
   }
   else // no matching usage
   {
      if (msg.isRequest())
      {
         switch (msg.header(h_CSeq).method())
         {
            case INVITE:  // new INVITE
               mInviteSession = mDum.makeServerInviteSession(*this, msg);
               break;
               
            case ACK:
            case CANCEL:
               InfoLog (<< "Drop stray ACK or CANCEL in dialog on the floor");
               DebugLog (<< msg);
               break;

            case SUBSCRIBE:
            case REFER: //!jf! does this create a server subs?
               mServerSubscription = mDum.makeServerSubscription(*this, msg);
               break;
               
            case NOTIFY:
               mClientSubscriptions.push_back(mDum.makeClientSubscription(*this, msg));
               break;
               
            case PUBLISH:
               mServerPublication = mDum.makeServerPublication(*this, msg);
               break;
               
            case REGISTER:
               mServerRegistration = mDum.makeServerRegistration(*this, msg);
               break;
               
            default:
               mServerOutOfDialogReq = mDum.makeServerOutOfDialog(*this, msg);
               break;
         }
      }
      else if (msg.isResponse())
      {
         // !jf! should this only be for 2xx responses?
         switch (msg.header(h_CSeq).method())
         {
            case INVITE:  
               mInviteSession = mDum.makeClientInviteSession(*this, msg);
               break;
               
            case ACK:
            case CANCEL:
               // Drop on the floor
               break;
               
            case SUBSCRIBE:
            case REFER: //!jf! does this create a server subs?
               mClientSubscriptions.push_back(mDum.makeClientSubscription(*this, msg));
               break;
               
            case NOTIFY:
               mClientSubscriptions.push_back(mDum.makeClientSubscription(*this, msg));
               break;
               
            case PUBLISH:
               mClientPublication = mDum.makeClientPublication(*this, msg);
               break;
               
            case REGISTER:
               mClientRegistration = mDum.makeClientRegistration(*this, msg);
               break;
               
            default:
               mClientOutOfDialogReq = mDum.makeClientOutOfDialog(*this, msg);
               break;
         }
      }
   }
}

BaseUsage* 
Dialog::findUsage(const SipMessage& msg)
{
   switch (msg.header(h_CSeq).method())
   {
      case INVITE:  // new INVITE
      case CANCEL:
      case ACK:
         return mInviteSession;

      case SUBSCRIBE:
      case REFER: 
      case NOTIFY: 
         for (std::vector<ClientSubscription*>::iterator i = mClientSubscriptions.begin(); 
              i != mClientSubscriptions.end(); ++i)
         {
            if ((*i)->matches(msg))
            {
               return *i;
            }
         }
         break;
      case REGISTER:
         InfoLog (<< "Received REGISTER inside an existing dialog. This is not supported. ");
         DebugLog (<< msg);
         break;
         
      case PUBLISH:
         if (msg.isRequest())
         {
            return mServerPublication;
         }
         else if (msg.isRequest())
         {
            return mClientPublication;
         }
         break;

      default:
         if (msg.isRequest())
         {
            return mServerOutOfDialogReq;
         }
         else if (msg.isRequest())
         {
            return mClientOutOfDialogReq;
         }
         break;
   }
   return 0;
}

InviteSession::Handle
Dialog::findInviteSession()
{
   if (mInviteSession)
   {
      return mInviteSession->getSessionHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such invite session",
                                 __FILE__, __LINE__);
   }
}

std::vector<ClientSubscription::Handle> 
Dialog::findClientSubscriptions()
{
   std::vector<ClientSubscription::Handle> handles;
   
   for (std::vector<ClientSubscription*>::const_iterator i = mClientSubscriptions.begin();
        i != mClientSubscriptions.end(); ++i)
   {
      handles.push_back((*i)->getHandle());
   }

   return handles;
}

ClientRegistration::Handle 
Dialog::findClientRegistration()
{
   if (mClientRegistration)
   {
      return mClientRegistration->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such client registration",
                                 __FILE__, __LINE__);
   }
}

ServerRegistration::Handle 
Dialog::findServerRegistration()
{
   if (mServerRegistration)
   {
      return mServerRegistration->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such server registration",
                                 __FILE__, __LINE__);
   }
}

ClientPublication::Handle 
Dialog::findClientPublication()
{
   if (mClientPublication)
   {
      return mClientPublication->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such client publication",
                                 __FILE__, __LINE__);
   }
}

ServerPublication::Handle 
Dialog::findServerPublication()
{
   if (mServerPublication)
   {
      return mServerPublication->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such server publication",
                                 __FILE__, __LINE__);
   }
}

ClientOutOfDialogReq::Handle 
Dialog::findClientOutOfDialog()
{
   if (mClientOutOfDialogReq)
   {
      return mClientOutOfDialogReq->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such client out of dialog",
                                 __FILE__, __LINE__);
   }
}

ServerOutOfDialogReq::Handle
Dialog::findServerOutOfDialog()
{
   if (mServerOutOfDialogReq)
   {
      return mServerOutOfDialogReq->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such server out of dialog",
                                 __FILE__, __LINE__);
   }
}

#if 0
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
#endif

Dialog::Exception::Exception(const Data& msg, const Data& file, int line)
   : BaseException(msg, file, line)
{}

