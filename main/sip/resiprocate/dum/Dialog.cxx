#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"

#include "BaseCreator.hxx"
#include "Dialog.hxx"
#include "DialogUsageManager.hxx"
#include "ClientOutOfDialogReq.hxx"

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
     mClientOutOfDialogRequests(),
     mServerOutOfDialogRequest(0),
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
                  BaseCreator* creator = mDum.findCreator(mId);
                  assert(creator);// !jf! throw or something here
                  
                  mLocalContact = creator->getLastRequest().header(h_Contacts).front();
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

void
Dialog::dispatch(const SipMessage& msg)
{
   if (msg.isRequest())
   {
      const SipMessage& request = msg;
      switch (request.header(h_CSeq).method())
      {
         case INVITE:  // new INVITE
            if (mInviteSession == 0)
            {
               mInviteSession = mDum.makeServerInviteSession(*this, request);
            }
            mInviteSession->dispatch(request);
            break;
               
         case ACK:
         case CANCEL:
            if (mInviteSession == 0)
            {
               InfoLog (<< "Drop stray ACK or CANCEL in dialog on the floor");
               DebugLog (<< request);
            }
            else
            {
               mInviteSession->dispatch(request);
            }
            break;

         case SUBSCRIBE:
         case REFER: //!jf! does this create a server subs?
            if (mServerSubscription == 0)
            {
               mServerSubscription = mDum.makeServerSubscription(*this, request);
            }
               
            mServerSubscription->dispatch(request);
            break;
               
         case NOTIFY:
            if (request.header(h_To).exists(p_tag))
            {
               ClientSubscription* client = findMatchingClientSub(request);
               if (client)
               {
                  client->dispatch(request);
               }
               else
               {
                  BaseCreator* creator = mDum.findCreator(mId);
                  if (creator)
                  {
                     ClientSubscription* sub = mDum.makeClientSubscription(*this, request);
                     mClientSubscriptions.push_back(sub);
                     sub->dispatch(request);
                  }
                  else
                  {
                     std::auto_ptr<SipMessage> failure(Helper::makeResponse(request, 481));
                     mDum.send(*failure);
                     return;
                  }
               }
            }
            else // no to tag - unsolicited notify
            {
               assert(mServerOutOfDialogRequest == 0);
               mServerOutOfDialogRequest = mDum.makeServerOutOfDialog(*this, request);
               mServerOutOfDialogRequest->dispatch(request);
            }
            break;
               
         case PUBLISH:
            if (mServerPublication == 0)
            {
               mServerPublication = mDum.makeServerPublication(*this, request);
            }
            mServerPublication->dispatch(request);
            break;

         case REGISTER:
            if (mServerRegistration == 0)
            {
               mServerRegistration = mDum.makeServerRegistration(*this, request);
            }
            mServerRegistration->dispatch(request);
            break;
               
         default: 
            // only can be one ServerOutOfDialogReq at a time
            assert(mServerOutOfDialogRequest == 0);
            mServerOutOfDialogRequest = mDum.makeServerOutOfDialog(*this, request);
            mServerOutOfDialogRequest->dispatch(request);
            break;
      }
   }
   else if (msg.isResponse())
   {
      const SipMessage& response = msg;
      // !jf! should this only be for 2xx responses?
      switch (response.header(h_CSeq).method())
      {
         case INVITE:
            if (mInviteSession == 0)
            {
               BaseCreator* creator = mDum.findCreator(mId);
               assert (creator);
               creator->dispatch(response);
               if (response.header(h_StatusLine).statusCode() != 100)
               {
                  mInviteSession = mDum.makeClientInviteSession(*this, response);
                  mInviteSession->dispatch(response);
               }
            }
            else
            {
               mInviteSession->dispatch(response);
            }
            break;
               
         case ACK:
         case CANCEL:
            if (mInviteSession != 0)
            {
               mInviteSession->dispatch(response);
            }
            // else drop on the floor
            break;
               
         case SUBSCRIBE:
         case REFER: 
         {
            ClientSubscription* client = findMatchingClientSub(response);
            if (client)
            {
               client->dispatch(response);
            }
            else
            {
               ClientSubscription* sub = mDum.makeClientSubscription(*this, response);
               mClientSubscriptions.push_back(sub);
               sub->dispatch(response);
            }
            break;
         }
               
         case PUBLISH:
            if (mClientPublication == 0)
            {
               mClientPublication = mDum.makeClientPublication(*this, response);
            }
            mClientPublication->dispatch(response);
            break;
               
         case REGISTER:
            if (mClientRegistration == 0)
            {
               mClientRegistration = mDum.makeClientRegistration(*this, response);
            }
            mClientRegistration->dispatch(response);
            break;
               
            // unsolicited - not allowed but commonly implemented
            // by large companies with a bridge as their logo
         case NOTIFY: 
         case INFO:   
               
         default:
         {
            ClientOutOfDialogReq* req = findMatchingClientOutOfDialogReq(response);
            if (req == 0)
            {
               req = mDum.makeClientOutOfDialogReq(*this, response);
               mClientOutOfDialogRequests.push_back(req);
            }
            req->dispatch(response);
            break;
         }
      }
   }
}


ClientSubscription* 
Dialog::findMatchingClientSub(const SipMessage& msg)
{
   for (std::list<ClientSubscription*>::iterator i=mClientSubscriptions.begin(); 
        i != mClientSubscriptions.end(); ++i)
   {
      if ((*i)->matches(msg))
      {
         return *i;
      }
   }
   return 0;
}

ClientOutOfDialogReq*
Dialog::findMatchingClientOutOfDialogReq(const SipMessage& msg)
{
   for (std::list<ClientOutOfDialogReq*>::iterator i=mClientOutOfDialogRequests.begin(); 
        i != mClientOutOfDialogRequests.end(); ++i)
   {
      if ((*i)->matches(msg))
      {
         return *i;
      }
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
   
   for (std::list<ClientSubscription*>::const_iterator i = mClientSubscriptions.begin();
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

#if 0
ClientOutOfDialogReq::Handle 
Dialog::findClientOutOfDialog()
{
   if (mClientOutOfDialogRequests)
   {
      return mClientOutOfDialogReq->getHandle();
   }
   else
   {
      throw BaseUsage::Exception("no such client out of dialog",
                                 __FILE__, __LINE__);
   }
}
#endif

ServerOutOfDialogReq::Handle
Dialog::findServerOutOfDialog()
{
   if (mServerOutOfDialogRequest)
   {
      return mServerOutOfDialogRequest->getHandle();
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
#endif


void 
Dialog::makeRequest(SipMessage& request, MethodTypes method)
{
   RequestLine rLine(method);

   rLine.uri() = mRemoteTarget.uri();
   
   request.header(h_RequestLine) = rLine;
   request.header(h_To) = mRemoteTarget;
   request.header(h_To).param(p_tag) = mRemoteTag;
   request.header(h_From) = mLocalContact;
   request.header(h_From).param(p_tag) = mLocalTag; 

   request.header(h_CallId) = mCallId;
   request.header(h_Routes) = mRouteSet;
   request.header(h_Contacts).push_front(mLocalContact);
   request.header(h_CSeq).method() = method;
   request.header(h_MaxForwards).value() = 70;

   Via via;
   via.param(p_branch); // will create the branch
   request.header(h_Vias).push_front(via);

   request.header(h_CSeq).sequence() = ++mLocalCSeq;
}


void 
Dialog::makeResponse(const SipMessage& request, SipMessage& response, int code)
{
   assert( code >= 100 );
   response.header(h_To).param(p_tag) = mLocalTag;
   if ( (code < 300) && (code > 100) )
   {
      assert(request.isRequest());
      assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE);
      
      assert (request.header(h_Contacts).size() == 1);
      response.header(h_To).param(p_tag) = mLocalTag;

      Helper::makeResponse(response, request, code);

      if (!request.exists(h_Contacts) && request.header(h_Contacts).size() != 1)
      {
         InfoLog (<< "Request doesn't have a contact header or more than one contact, so can't create dialog");
         DebugLog (<< request);
         throw Exception("Invalid or missing contact header in request", __FILE__,__LINE__);
      }

      assert (response.header(h_To).exists(p_tag));
   }
   else
   {
      Helper::makeResponse(response, request, code, mLocalContact);
   }
}

Dialog::Exception::Exception(const Data& msg, const Data& file, int line)
   : BaseException(msg, file, line)
{}

void
Dialog::update(const SipMessage& msg)
{
}

#if 0
void 
Dialog::setLocalContact(const NameAddr& localContact)
{
   mLocalContact = localContact;
}

void 
Dialog::setRemoteTarget(const NameAddr& remoteTarget)
{
   mRemoteTarget = remoteTarget;
}
#endif

bool 
Dialog::shouldMerge(const SipMessage& request) const
{
   return false;
}

void Dialog::possiblyDie()
{
   if (mClientSubscriptions.empty() &&
       mClientOutOfDialogRequests.empty() &&
       !(mServerSubscription ||
         mInviteSession ||
         mClientRegistration ||
         mServerRegistration ||
         mClientPublication ||
         mServerPublication ||
         mServerOutOfDialogRequest))
   {
      delete this;
   }
}

