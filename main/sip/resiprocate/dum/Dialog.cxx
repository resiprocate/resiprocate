#include "resiprocate/Contents.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/BaseCreator.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"
#include "resiprocate/dum/ClientOutOfDialogReq.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionCreator.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/ServerPublication.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

Dialog::Dialog(DialogUsageManager& dum, const SipMessage& msg, DialogSet& ds) 
   : mDum(dum),
     mDialogSet(ds),
     mClientSubscriptions(),
     mServerSubscriptions(),
     mInviteSession(0),
     mClientRegistration(0),
     mServerRegistration(0),
     mClientPublication(0),
     mServerPublication(0),
     mClientOutOfDialogRequests(),
     mServerOutOfDialogRequest(0),
     mType(Fake),
     mRouteSet(),
     mLocalContact(),
     mLocalCSeq(0),
     mRemoteCSeq(0),
     mId("INVALID", "INVALID", "INVALID"),
     mRemoteTarget(),
     mLocalNameAddr(),
     mRemoteNameAddr(),
     mCallId(msg.header(h_CallID)),
     mDestroying(false)
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
            //!dcm! -- event header check
            mType = Subscription;
            break;

         default:
            mType = Fake;
      }
      if (request.exists(h_RecordRoutes))
      {
         mRouteSet = request.header(h_RecordRoutes); // !jf! is this right order
      }

      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSCRIBE:
         case REFER:
            InfoLog ( << "UAS dialog ID creation, DS: " << ds.getId());            
            mId = DialogId(ds.getId(), request.header(h_From).param(p_tag));
            mRemoteNameAddr = request.header(h_From);
            mLocalNameAddr = request.header(h_To);
            mLocalNameAddr.param(p_tag) = mId.getLocalTag();
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

      InfoLog ( << "************** Created Dialog as UAS **************" );      
      InfoLog ( << "mRemoteNameAddr: " << mRemoteNameAddr ); 
      InfoLog ( << "mLocalNameAddr: " << mLocalNameAddr ); 
      InfoLog ( << "mLocalContact: " << mLocalContact );
      InfoLog ( << "mRemoteTarget: " << mRemoteTarget );
   }
   else if (msg.isResponse())
   {
      mId = DialogId(msg);      
      const SipMessage& response = msg;
      mRemoteNameAddr = response.header(h_To);
      mLocalNameAddr = response.header(h_From);

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
            if (response.header(h_StatusLine).statusCode() > 100 &&
                response.header(h_StatusLine).statusCode() < 300)
            {
               
               if  (response.exists(h_Contacts) && response.header(h_Contacts).size() == 1)
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
      }
      
      mLocalCSeq = response.header(h_CSeq).sequence();
      mRemoteCSeq = 0;
      InfoLog ( << "************** Created Dialog as UAC **************" );      
      InfoLog ( << "mRemoteNameAddr: " << mRemoteNameAddr ); 
      InfoLog ( << "mLocalNameAddr: " << mLocalNameAddr ); 
      InfoLog ( << "mLocalContact: " << mLocalContact );
      InfoLog ( << "mRemoteTarget: " << mRemoteTarget );

      
   }
   mDialogSet.addDialog(this);
}

Dialog::~Dialog()
{
   mDestroying = true;
   //does removing an elemnt from a list invalidate iterators?
   for(std::list<ClientSubscription*>::iterator it = mClientSubscriptions.begin(); 
       it != mClientSubscriptions.end(); it++)
   {
      delete *it;
   }
   for(std::list<ServerSubscription*>::iterator it = mServerSubscriptions.begin(); 
       it != mServerSubscriptions.end(); it++)
   {
      delete *it;
   }
   for(std::list<ClientOutOfDialogReq*>::iterator it = mClientOutOfDialogRequests.begin(); 
       it != mClientOutOfDialogRequests.end(); it++)
   {
      delete *it;
   }
   delete mInviteSession;
   delete mClientRegistration;
   delete mServerRegistration;
   delete mClientPublication;
   delete mServerPublication;
   delete mServerOutOfDialogRequest;

   mDialogSet.mDialogs.erase(this->getId());
   delete mAppDialog;
   mDialogSet.possiblyDie();
}

DialogId
Dialog::getId() const
{
   return mId;
}

void
Dialog::cancel()
{
   if (mInviteSession)
   {
      mInviteSession->end();
   }
   else
   {
      if (mDialogSet.getCreator())
      {
         makeRequest(mDialogSet.getCreator()->getLastRequest(), CANCEL);
         mDum.send(mDialogSet.getCreator()->getLastRequest());
         delete this;
      }
   }
}

void
Dialog::dispatch(const SipMessage& msg)
{
   InfoLog ( << "Dialog::dispatch: " << msg.brief());
   if (msg.isRequest())
   {
      const SipMessage& request = msg;
      switch (request.header(h_CSeq).method())
      {
         case INVITE:  // new INVITE
            if (mInviteSession == 0)
            {
               InfoLog ( << "Dialog::dispatch  --  Created new server invite session" << msg.brief());
               mInviteSession = makeServerInviteSession(request);
            }
            mInviteSession->dispatch(request);
            break;
         case BYE:
            if (mInviteSession == 0)
            {
               InfoLog ( << "Spurious BYE" );
               return;               
            }
            else
            {
               mInviteSession->dispatch(request);
            }
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
         {
            ServerSubscription* server = findMatchingServerSub(request);
            if (server)
            {
               server->dispatch(request);
            }
            else
            {
               server = makeServerSubscription(request);
               mServerSubscriptions.push_back(server);
               server->dispatch(request);
            }
         }
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
                     ClientSubscription* sub = makeClientSubscription(request);
                     mClientSubscriptions.push_back(sub);
                     sub->dispatch(request);
                  }
                  else
                  {
                     SipMessage failure;
                     makeResponse(failure, request, 481);
                     mDum.sendResponse(failure);
                     return;
                  }
               }
            }
            else // no to tag - unsolicited notify
            {
               assert(mServerOutOfDialogRequest == 0);
               mServerOutOfDialogRequest = makeServerOutOfDialog(request);
               mServerOutOfDialogRequest->dispatch(request);
            }
            break;
               
         case PUBLISH:
            if (mServerPublication == 0)
            {
               mServerPublication = makeServerPublication(request);
            }
            mServerPublication->dispatch(request);
            break;

         case REGISTER:
            if (mServerRegistration == 0)
            {
               mServerRegistration = makeServerRegistration(request);
            }
            mServerRegistration->dispatch(request);
            break;
               
         default: 
            InfoLog ( << "In Dialog::dispatch, default(ServerOutOfDialogRequest), msg: " << msg );            
            // only can be one ServerOutOfDialogReq at a time
            assert(mServerOutOfDialogRequest == 0);
            mServerOutOfDialogRequest = makeServerOutOfDialog(request);
            mServerOutOfDialogRequest->dispatch(request);
            break;
      }
   }
   else if (msg.isResponse())
   {
#if 0
//       //Auth related
//       if (mDum.mClientAuthManager && !mDialogSet.mCancelled)
//       {
//          if (mDialogSet.getCreator())
//          {
//             if ( mDum.mClientAuthManager->handle( mDialogSet.getCreator()->getLastRequest(), msg))
//             {
//                InfoLog( << "about to retransmit request with digest credentials" );
//                InfoLog( << mDialogSet.getCreator()->getLastRequest() );
               
//                mDum.send(mDialogSet.getCreator()->getLastRequest());
//                return;
//             }
//          }
//          else
//          {
//             SipMessage* lastRequest = 0;            
//             switch (msg.header(h_CSeq).method())
//             {
//                case INVITE:
//                case CANCEL:
//                case REFER: 
//                   if (mInviteSession == 0)
//                   {
//                      return;
//                   }
//                   else
//                   {
//                      lastRequest = &mInviteSession->mLastRequest;
//                   }
//                   break;               
//                case REGISTER:
//                   if (mClientRegistration == 0)
//                   {
//                      return;
//                   }
//                   else
//                   {
//                      lastRequest = &mClientRegistration->mLastRequest;
//                   }
//                   break;               
//                default:
//                   break;
//             }
//             if ( lastRequest && mDum.mClientAuthManager->handle( *lastRequest, msg ) )
//             {
//                InfoLog( << "about to retransmit request with digest credentials" );
//                InfoLog( << *lastRequest );
               
//                mDum.send(*lastRequest);
//                return;
//             }
//          }
//       }
#endif
      const SipMessage& response = msg;
      // !jf! should this only be for 2xx responses? !jf! Propose no as an
      // answer !dcm! what is he on?
      switch (response.header(h_CSeq).method())
      {
         case INVITE:
            if (mInviteSession == 0)
            {
               // #if!jf! don't think creator needs a dispatch
               //BaseCreator* creator = mDum.findCreator(mId);
               //assert (creator); // stray responses have been rejected already 
               //creator->dispatch(response); 
               // #endif!jf! 
               InfoLog ( << "Dialog::dispatch  --  Created new client invite session" << msg.brief());

               mInviteSession = makeClientInviteSession(response);
               mInviteSession->dispatch(response);
            }
            else
            {
               mInviteSession->dispatch(response);
            }
            break;
         case BYE:
         case ACK:
         case CANCEL:
            if (mInviteSession != 0)
            {
               mInviteSession->dispatch(response);
            }
            // else drop on the floor
            break;               
         case REFER: 
         {
            int code = response.header(h_StatusLine).statusCode();
            if (code < 300)
            {
               // throw it away
               return;
            }            
            else
            {
               if (mInviteSession && mDum.mInviteSessionHandler)
               {
                  mDum.mInviteSessionHandler->onReferRejected(mInviteSession->getSessionHandle(), response);
               }
            }         
         }
         break;         
         case SUBSCRIBE:
         {
            int code = response.header(h_StatusLine).statusCode();
            if (code < 300)
            {
               // throw it away
               return;
            }            
            else
            {
               ClientSubscription* client = findMatchingClientSub(response);
               if (client)
               {
                  client->dispatch(response);
               }
            }
         }
         break;
         case PUBLISH:
            // !jf! could assert that no other usages exist
            if (mClientPublication == 0)
            {
               mClientPublication = makeClientPublication(response);
            }
            mClientPublication->dispatch(response);
            break;
               
         case REGISTER:
            // !jf! could assert that no other usages exist
            if (mClientRegistration == 0)
            {
               mClientRegistration = makeClientRegistration(response);
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
               req = makeClientOutOfDialogReq(response);
               mClientOutOfDialogRequests.push_back(req);
            }
            req->dispatch(response);
            break;
         }
      }
   }
}

ServerSubscription* 
Dialog::findMatchingServerSub(const SipMessage& msg)
{
   for (std::list<ServerSubscription*>::iterator i=mServerSubscriptions.begin(); 
        i != mServerSubscriptions.end(); ++i)
   {
      if ((*i)->matches(msg))
      {
         return *i;
      }
   }
   return 0;
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


InviteSessionHandle
Dialog::getInviteSession()
{
   if (mInviteSession)
   {
      return mInviteSession->getSessionHandle();
   }
   else
   {
      return InviteSessionHandle::NotValid();
   }
}

std::vector<ClientSubscriptionHandle> 
Dialog::findClientSubscriptions(const Data& event)
{
   std::vector<ClientSubscriptionHandle> handles;
   
   for (std::list<ClientSubscription*>::const_iterator i = mClientSubscriptions.begin();
        i != mClientSubscriptions.end(); ++i)
   {
      if ( (*i)->getEventType() == event)
      {
         handles.push_back((*i)->getHandle());
      }
   }
   return handles;
}

std::vector<ServerSubscriptionHandle> 
Dialog::findServerSubscriptions(const Data& event)
{
   std::vector<ServerSubscriptionHandle> handles;
   
   for (std::list<ServerSubscription*>::const_iterator i = mServerSubscriptions.begin();
        i != mServerSubscriptions.end(); ++i)
   {
      if ( (*i)->getEventType() == event)
      {
         handles.push_back((*i)->getHandle());
      }
   }
   return handles;
}


std::vector<ClientSubscriptionHandle> 
Dialog::getClientSubscriptions()
{
   std::vector<ClientSubscriptionHandle> handles;
   
   for (std::list<ClientSubscription*>::const_iterator i = mClientSubscriptions.begin();
        i != mClientSubscriptions.end(); ++i)
   {
      handles.push_back((*i)->getHandle());
   }

   return handles;
}

std::vector<ServerSubscriptionHandle> 
Dialog::getServerSubscriptions()
{
   std::vector<ServerSubscriptionHandle> handles;
   
   for (std::list<ServerSubscription*>::const_iterator i = mServerSubscriptions.begin();
        i != mServerSubscriptions.end(); ++i)
   {
      handles.push_back((*i)->getHandle());
   }

   return handles;
}

ClientRegistrationHandle 
Dialog::getClientRegistration()
{
   if (mClientRegistration)
   {
      return mClientRegistration->getHandle();
   }
   else
   {
      return ClientRegistrationHandle::NotValid();
   }
}

ServerRegistrationHandle 
Dialog::getServerRegistration()
{
   if (mServerRegistration)
   {
      return mServerRegistration->getHandle();
   }
   else
   {
      return ServerRegistrationHandle::NotValid();
   }
}

ClientPublicationHandle 
Dialog::getClientPublication()
{
   if (mClientPublication)
   {
      return mClientPublication->getHandle();
   }
   else
   {
      return ClientPublicationHandle::NotValid();      
   }
}

ServerPublicationHandle 
Dialog::getServerPublication()
{
   if (mServerPublication)
   {
      return mServerPublication->getHandle();
   }
   else
   {
      return ServerPublicationHandle::NotValid();      
   }
}

#if 0
ClientOutOfDialogReqHandle 
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

ServerOutOfDialogReqHandle
Dialog::getServerOutOfDialog()
{
   if (mServerOutOfDialogRequest)
   {
      return mServerOutOfDialogRequest->getHandle();
   }
   else
   {
      return ServerOutOfDialogReqHandle::NotValid();
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
   request.header(h_To) = mRemoteNameAddr;   
//   request.header(h_To).param(p_tag) = mId.getRemoteTag();
   request.header(h_From) = mLocalNameAddr;   
//   request.header(h_From).param(p_tag) = mId.getLocalTag(); 

   request.header(h_CallId) = mCallId;

   request.remove(h_RecordRoutes);  //!dcm! -- all of this is rather messy

   request.remove(h_Contacts);   
   request.header(h_Contacts).push_front(mLocalContact);   
   request.header(h_CSeq).method() = method;
   request.header(h_MaxForwards).value() = 70;

   //must keep old via for cancel
   if (method != CANCEL)
   {
      request.header(h_Routes) = mRouteSet;
      request.remove(h_Vias);      
      Via via;
      via.param(p_branch); // will create the branch
      request.header(h_Vias).push_front(via);
   }
   else
   {
      assert(request.exists(h_Vias));
   }
   //don't increment CSeq for ACK or CANCEL
   if (method != ACK && method != CANCEL)
   {
      request.header(h_CSeq).sequence() = ++mLocalCSeq;
   }
   InfoLog ( << "Dialog::makeRequest: " << request );
}

void 
Dialog::makeCancel(SipMessage& request)
{
   makeRequest(request, CANCEL);   

   //not allowed in a CANCEL
   request.remove(h_Requires);
   request.remove(h_ProxyRequires);
   request.header(h_To).remove(p_tag);   
}

void 
Dialog::makeResponse(SipMessage& response, const SipMessage& request, int code)
{
   assert( code >= 100 );
   if (code < 300 && code > 100)
   {
      assert(request.isRequest());
      assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE ||
             request.header(h_RequestLine).getMethod() == BYE ||
             request.header(h_RequestLine).getMethod() == CANCEL);
      
      assert (request.header(h_RequestLine).getMethod() == CANCEL ||  // Contact header is not required for Requests that do not form a dialog
		      request.header(h_RequestLine).getMethod() == BYE ||
		      request.header(h_Contacts).size() == 1);
      Helper::makeResponse(response, request, code, mLocalContact);
      response.header(h_To).param(p_tag) = mId.getLocalTag();
   }
   else
   {
      Helper::makeResponse(response, request, code, mLocalContact);
      response.header(h_To).param(p_tag) = mId.getLocalTag();

   }
   InfoLog ( << "Dialog::makeResponse: " << response);   
}


ClientInviteSession*
Dialog::makeClientInviteSession(const SipMessage& response)
{
   InviteSessionCreator* creator = dynamic_cast<InviteSessionCreator*>(mDialogSet.getCreator());
   assert(creator); // !jf! this maybe can assert by evil UAS
   //return mDum.createAppClientInviteSession(*this, *creator);
   return new ClientInviteSession(mDum, *this, creator->getLastRequest(), creator->getInitialOffer());
}

ClientRegistration*
Dialog::makeClientRegistration(const SipMessage& response)
{
   BaseCreator* creator = mDialogSet.getCreator();
   assert(creator);
   return new ClientRegistration(mDum, *this, creator->getLastRequest());
}

ClientPublication*
Dialog::makeClientPublication(const SipMessage& response)
{
   BaseCreator* creator = mDialogSet.getCreator();
   assert(creator);
   return new ClientPublication(mDum, *this, creator->getLastRequest());
}

ClientSubscription*
Dialog::makeClientSubscription(const SipMessage& request)
{
   return new ClientSubscription(mDum, *this, request);
}

ClientOutOfDialogReq*
Dialog::makeClientOutOfDialogReq(const SipMessage& response)
{
   BaseCreator* creator = mDialogSet.getCreator();
   assert(creator);
   return new ClientOutOfDialogReq(mDum, *this, creator->getLastRequest());
}

ServerInviteSession*
Dialog::makeServerInviteSession(const SipMessage& request)
{
   return new ServerInviteSession(mDum, *this, request);
}

ServerSubscription* 
Dialog::makeServerSubscription(const SipMessage& request)
{
   return new ServerSubscription(mDum, *this, request);
}

ServerRegistration* 
Dialog::makeServerRegistration(const SipMessage& request)
{
   return new ServerRegistration(mDum, *this, request);
}

ServerPublication* 
Dialog::makeServerPublication(const SipMessage& request)
{
   return new ServerPublication(mDum, *this, request);
}

ServerOutOfDialogReq* 
Dialog::makeServerOutOfDialog(const SipMessage& request)
{
   return new ServerOutOfDialogReq(mDum, *this, request);
}

Dialog::Exception::Exception(const Data& msg, const Data& file, int line)
   : BaseException(msg, file, line)
{
}

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

void Dialog::possiblyDie()
{
   if (!mDestroying)
   {
      if (mClientSubscriptions.empty() &&
          mServerSubscriptions.empty() &&
          mClientOutOfDialogRequests.empty() &&
          !(mInviteSession ||
            mClientRegistration ||
            mServerRegistration ||
            mClientPublication ||
            mServerPublication ||
            mServerOutOfDialogRequest))
      {
         delete this;
      }
   }   
}

ostream& 
resip::operator<<(ostream& strm, const Dialog& dialog)
{

   return strm;
}

