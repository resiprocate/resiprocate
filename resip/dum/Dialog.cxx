#include "resip/stack/Contents.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/BaseCreator.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/InviteSessionCreator.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "rutil/ResipAssert.h"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

Dialog::Dialog(DialogUsageManager& dum, const SipMessage& msg, DialogSet& ds)
   : mDum(dum),
     mDialogSet(ds),
     mId("INVALID", "INVALID", "INVALID"),
     mClientSubscriptions(),
     mServerSubscriptions(),
     mInviteSession(0),
     mType(Fake),
     mRouteSet(),
     mLocalContact(),
     mLocalCSeq(0),
     mRemoteCSeq(0),
     mRemoteTarget(),
     mLocalNameAddr(),
     mRemoteNameAddr(),
     mCallId(msg.header(h_CallID)),
     mAppDialog(0),
     mDestroying(false),
     mReUseDialogSet(false)
{
   resip_assert(msg.isExternal());

   resip_assert(msg.header(h_CSeq).method() != MESSAGE);
   resip_assert(msg.header(h_CSeq).method() != REGISTER);
   resip_assert(msg.header(h_CSeq).method() != PUBLISH);

   mNetworkAssociation.setDum(&dum);

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
         mRouteSet = request.header(h_RecordRoutes); 
      }

      switch (request.header(h_CSeq).method())
      {
         case INVITE:
         case SUBSCRIBE:
         case REFER:
         case NOTIFY:
            DebugLog ( << "UAS dialog ID creation, DS: " << ds.getId());
            mId = DialogId(ds.getId(), 
                           request.header(h_From).exists(p_tag) ? request.header(h_From).param(p_tag) : Data::Empty);            

            mRemoteNameAddr = request.header(h_From);
            mLocalNameAddr = request.header(h_To);
            mLocalNameAddr.param(p_tag) = mId.getLocalTag();
            if (request.exists(h_Contacts) && request.header(h_Contacts).size() == 1)
            {
               const NameAddr& contact = request.header(h_Contacts).front();
               if (isEqualNoCase(contact.uri().scheme(), Symbols::Sips) ||
                   isEqualNoCase(contact.uri().scheme(), Symbols::Sip))
               {
                  // Store Remote Target
                  mRemoteTarget = contact;
                  
                  // Create Local Contact
                  if(mDialogSet.mUserProfile->hasUserAgentCapabilities())
                  {
                     mLocalContact = mDialogSet.mUserProfile->getUserAgentCapabilities();
                  }
                  if(!mDialogSet.mUserProfile->isAnonymous() && mDialogSet.mUserProfile->hasPublicGruu())
                  {
                     mLocalContact.uri() = mDialogSet.mUserProfile->getPublicGruu();
                  }
                  else if(mDialogSet.mUserProfile->isAnonymous() && mDialogSet.mUserProfile->hasTempGruu())
                  {
                     mLocalContact.uri() = mDialogSet.mUserProfile->getTempGruu();
                  }
                  else
                  {
                     if (mDialogSet.mUserProfile->hasOverrideHostAndPort())
                     {
                        mLocalContact.uri() = mDialogSet.mUserProfile->getOverrideHostAndPort();
                     }
                     if(request.header(h_RequestLine).uri().user().empty())
                     {
                        mLocalContact.uri().user() = request.header(h_To).uri().user(); 
                     }
                     else
                     {
                        mLocalContact.uri().user() = request.header(h_RequestLine).uri().user(); 
                     }
                     const Data& instanceId = mDialogSet.mUserProfile->getInstanceId();
                     if (!contact.uri().exists(p_gr) && !instanceId.empty())
                     {
                        mLocalContact.param(p_Instance) = instanceId;
                     }
                  }
                  if(mDialogSet.mUserProfile->clientOutboundEnabled())
                  {
                     // Add ;ob parm to non-register requests - RFC5626 pg17
                     mLocalContact.uri().param(p_ob);
                  }
               }
               else
               {
                  InfoLog(<< "Got an INVITE or SUBSCRIBE with invalid scheme");
                  InfoLog(<< request);
                  throw Exception("Invalid scheme in request", __FILE__, __LINE__);
               }
            }
            else
            {
               InfoLog (<< "Got an INVITE or SUBSCRIBE that doesn't have exactly one contact");
               InfoLog (<< request);
               throw Exception("Too many (or no contact) contacts in request", __FILE__, __LINE__);
            }
            break;
         default:
            break;
      }

      mRemoteCSeq = request.header(h_CSeq).sequence();

      // This may actually be a UAC dialogset - ie. the case where the first NOTIFY creates the
      // SUBSCRIPTION dialog, instead of the 200/SUB.  If so, then we need to make sure the local
      // CSeq is correct - it's value may be greator than 1, if the original request (SUBSCRIBE)
      // got digest challenged.
      BaseCreator* creator = mDialogSet.getCreator();
      if(creator)
      {
         mLocalCSeq = creator->getLastRequest()->header(h_CSeq).sequence();
      }
      else
      {
         mLocalCSeq = 1;
      }

      DebugLog ( << "************** Created Dialog as UAS **************" );
      DebugLog ( << "mRemoteNameAddr: " << mRemoteNameAddr );
      DebugLog ( << "mLocalNameAddr: " << mLocalNameAddr );
      DebugLog ( << "mLocalContact: " << mLocalContact );
      DebugLog ( << "mRemoteTarget: " << mRemoteTarget );
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
                     BaseCreator* creator = mDialogSet.getCreator();
                     if(0 == creator)
                     {
                        ErrLog(<< "BaseCreator is null for DialogSet");
                        ErrLog(<< response);
                        throw Exception("BaseCreator is null for DialogSet", __FILE__, __LINE__);
                     }

                     SharedPtr<SipMessage> lastRequest(creator->getLastRequest());

                     if( 0 == lastRequest.get() ||
                        !lastRequest->exists(h_Contacts) ||
                        lastRequest->header(h_Contacts).empty())
                     {
                        InfoLog(<< "lastRequest does not contain a valid contact");						
                        InfoLog(<< response);
                        throw Exception("lastRequest does not contain a valid contact.", __FILE__, __LINE__);
                     }
                     mLocalContact = creator->getLastRequest()->header(h_Contacts).front();
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
      DebugLog ( << "************** Created Dialog as UAC **************" );
      DebugLog ( << "mRemoteNameAddr: " << mRemoteNameAddr );
      DebugLog ( << "mLocalNameAddr: " << mLocalNameAddr );
      DebugLog ( << "mLocalContact: " << mLocalContact );
      DebugLog ( << "mRemoteTarget: " << mRemoteTarget );
   }
   mDialogSet.addDialog(this);
   DebugLog ( <<"Dialog::Dialog " << mId);
}

Dialog::~Dialog()
{
   DebugLog ( <<"Dialog::~Dialog() ");

   mDestroying = true;

   while (!mClientSubscriptions.empty())
   {
      delete *mClientSubscriptions.begin();
   }

   while (!mServerSubscriptions.empty())
   {
      delete *mServerSubscriptions.begin();
   }

   delete mInviteSession;
   mDialogSet.mDialogs.erase(this->getId());
   delete mAppDialog;
   if(!mReUseDialogSet)
   {
      mDialogSet.possiblyDie();
   }
}

const DialogId&
Dialog::getId() const
{
   return mId;
}

const NameAddr&
Dialog::getLocalNameAddr() const
{
   return mLocalNameAddr;
}

const NameAddr&
Dialog::getLocalContact() const
{
   return mLocalContact;
}

const NameAddr&
Dialog::getRemoteNameAddr() const
{
   return mRemoteNameAddr;
}

const NameAddr&
Dialog::getRemoteTarget() const
{
   return mRemoteTarget;
}

const NameAddrs&
Dialog::getRouteSet() const
{
   return mRouteSet;
}

void
Dialog::cancel()
{
   resip_assert(mType == Invitation);
   ClientInviteSession* uac = dynamic_cast<ClientInviteSession*>(mInviteSession);
   resip_assert (uac);
   uac->cancel();
}

void
Dialog::end()
{
   if (mInviteSession)
   {
      mInviteSession->end();
   }

   // End Subscriptions
   // !jrm! WARN ClientSubscription and ServerSubscription have access to this dialog and will remove themselves
   // from the m<client|server>Subscriptions collections in the call to end().
   for (list<ClientSubscription*>::iterator it(mClientSubscriptions.begin());
        it != mClientSubscriptions.end();)
   {
	   ClientSubscription* c = *it;
       it++;       
	   c->end();
   }

   for (list<ServerSubscription*>::iterator it2(mServerSubscriptions.begin());
        it2 != mServerSubscriptions.end();)
   {
	   ServerSubscription* s = *it2;
       it2++;       
	   s->end();
   }
}

void
Dialog::handleTargetRefresh(const SipMessage& msg)
{
   switch(msg.header(h_CSeq).method())
   {
      case INVITE:
      case UPDATE:
      case SUBSCRIBE: // RFC6665 - Note: target refreshes via NOTIFY requests are handled via 
                      //           ClientSubscription usage after NOTIFY ordering is confirmed
         if (msg.isRequest() || (msg.isResponse() && msg.header(h_StatusLine).statusCode()/100 == 2))
         {
            //?dcm? modify local target; 12.2.2 of 3261 implies that the remote
            //target is immediately modified.  Should we wait until a 2xx class
            //reponse is sent to a re-invite(easy when all send requests go
            //through Dialog)
            if (msg.exists(h_Contacts))
            {
               //.dcm. replace or check then replace
               mRemoteTarget = msg.header(h_Contacts).front();
            }
         }
         break;
      default:
         return;
   }
}

void
Dialog::dispatch(const SipMessage& msg)
{
   // !jf! Should be checking for messages with out of order CSeq and rejecting

   DebugLog ( << "Dialog::dispatch: " << msg.brief());

   if(msg.isFromWire())
   {
      TransportType receivedTransport = toTransportType(
         msg.header(h_Vias).front().transport());
      int keepAliveTime = 0;
      if(isReliable(receivedTransport))
      {
         keepAliveTime = mDialogSet.mUserProfile->getKeepAliveTimeForStream();
      }
      else
      {
         keepAliveTime = mDialogSet.mUserProfile->getKeepAliveTimeForDatagram();
      }

      if(keepAliveTime > 0)
      {
         mNetworkAssociation.update(msg, keepAliveTime, false /* targetSupportsOutbound */); // target supports outbound is detected in registration responses only
      }
   }
   
   handleTargetRefresh(msg);
   if (msg.isRequest())
   {
      const SipMessage& request = msg;
      switch (request.header(h_CSeq).method())
      {
         case INVITE:  // new INVITE
            if (mInviteSession == 0)
            {
               DebugLog ( << "Dialog::dispatch  --  Created new server invite session" << msg.brief());
               mInviteSession = makeServerInviteSession(request);
            }
            mInviteSession->dispatch(request);
            break;
            //refactor, send bad request for BYE, INFO, CANCEL?
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
         case UPDATE:
            if (mInviteSession == 0)
            {
               InfoLog ( << "Spurious UPDATE" );
               return;
            }
            else
            {
               mInviteSession->dispatch(request);
            }
            break;
         case INFO:
            if (mInviteSession == 0)
            {
               InfoLog ( << "Spurious INFO" );
               return;
            }
            else
            {
               mInviteSession->dispatch(request);
            }
            break;
         case MESSAGE:
            if (mInviteSession == 0)
            {
               InfoLog ( << "Spurious MESSAGE" );
               return;
            }
            else
            {
               mInviteSession->dispatch(request);
            }
            break;
         case PRACK:
            if (mInviteSession == 0)
            {
               InfoLog ( << "Spurious PRACK" );
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
         {
            ServerSubscription* server = findMatchingServerSub(request);
            if (server)
            {
               server->dispatch(request);
            }
            else
            {
               if (request.exists(h_Event) && request.header(h_Event).value() == "refer")
               {
                  InfoLog (<< "Received a subscribe to a non-existent refer subscription: " << request.brief());
                  SipMessage failure;
                  makeResponse(failure, request, 403);
                  mDum.sendResponse(failure);
                  return;
               }
               else
               {
                  if (mDum.checkEventPackage(request))
                  {
                     server = makeServerSubscription(request);
                     mServerSubscriptions.push_back(server);
                     server->dispatch(request);
                  }
               }
            }
         }
         break;
         case REFER:
         {
//             if (mInviteSession == 0)
//             {
//                InfoLog (<< "Received an in dialog refer in a non-invite dialog: " << request.brief());
//                SipMessage failure;
//                makeResponse(failure, request, 603);
//                mDum.sendResponse(failure);
//                return;
//             }
//             else 

            if  (!request.exists(h_ReferTo))
            {
               InfoLog (<< "Received refer w/out a Refer-To: " << request.brief());
               SipMessage failure;
               makeResponse(failure, request, 400);
               mDum.sendResponse(failure);
               return;
            }
            else
            {
               if ((request.exists(h_ReferSub) && 
                     request.header(h_ReferSub).isWellFormed() &&
                     request.header(h_ReferSub).value()=="false") ||
                     (request.exists(h_Requires) &&
                     request.header(h_Requires).find(Token("norefersub"))))
               {
                  resip_assert(mInviteSession);
                  mInviteSession->referNoSub(msg);
               }
               else
               {
                  ServerSubscription* server = findMatchingServerSub(request);
                  ServerSubscriptionHandle serverHandle;
                  if (server)
                  {
                     serverHandle = server->getHandle();
                     server->dispatch(request);
                  }
                  else
                  {
                     server = makeServerSubscription(request);
                     mServerSubscriptions.push_back(server);
                     serverHandle = server->getHandle();
                     server->dispatch(request);
                  }

                  if (mInviteSession)
                  {
                     mDum.mInviteSessionHandler->onRefer(mInviteSession->getSessionHandle(), serverHandle, msg);
                  }
                  
               }
            }
         }
         break;
         case NOTIFY:
            {
               ClientSubscription* client = findMatchingClientSub(request);
               if (client)
               {
                  client->dispatch(request);
               }
               else
               {
                  BaseCreator* creator = mDialogSet.getCreator();
                  if (creator && (creator->getLastRequest()->header(h_RequestLine).method() == SUBSCRIBE ||
                     creator->getLastRequest()->header(h_RequestLine).method() == REFER))  
                  {
                     DebugLog (<< "Making subscription (from creator) request: " << *creator->getLastRequest());
                     ClientSubscription* sub = makeClientSubscription(*creator->getLastRequest());
                     mClientSubscriptions.push_back(sub);
                     sub->dispatch(request);
                  }
                  else
                  {
                     if (mInviteSession != 0 && (!msg.exists(h_Event) || msg.header(h_Event).value() == "refer") && 
                         mDum.getClientSubscriptionHandler("refer")!=0) 
                     {
                        DebugLog (<< "Making subscription from NOTIFY: " << msg);
                        ClientSubscription* sub = makeClientSubscription(msg);
                        mClientSubscriptions.push_back(sub);
                        ClientSubscriptionHandle client = sub->getHandle();
                        mDum.mInviteSessionHandler->onReferAccepted(mInviteSession->getSessionHandle(), client, msg);				      
                        sub->dispatch(request);
                     }
                     else
                     {
                        SharedPtr<SipMessage> response(new SipMessage);
                        makeResponse(*response, msg, 406);
                        send(response);
                     }
                  }
               }
            }
            break;
        default:
           resip_assert(0);
           return;
      }
   }
   else if (msg.isResponse())
   {
      // !jf! There is a substantial change in how this works in teltel-branch
      // from how it worked in main branch pre merge.
      // If the response doesn't match a cseq for a request I've sent, ignore
      // the response
      {//scope 'r' as it is invalidated below
         RequestMap::iterator r = mRequests.find(msg.header(h_CSeq).sequence());
         if (r != mRequests.end())
         {         
            bool handledByAuth = false;
            if (mDum.mClientAuthManager.get() && 
                mDum.mClientAuthManager->handle(*mDialogSet.mUserProfile, *r->second, msg))
            {
               InfoLog( << "about to re-send request with digest credentials" << r->second->brief());

               resip_assert (r->second->isRequest());

               mLocalCSeq++;
               send(r->second);
               handledByAuth = true;
            }
            mRequests.erase(r);
            if (handledByAuth) return;
         }
      }
      
      const SipMessage& response = msg;
      int code = response.header(h_StatusLine).statusCode();
      // If this is a 200 response to the initial request, then store the routeset (if present)
      BaseCreator* creator = mDialogSet.getCreator();
      if (creator && (creator->getLastRequest()->header(h_CSeq) == response.header(h_CSeq)) && code >=200 && code < 300)
      {
         if (response.exists(h_RecordRoutes))
         {
            mRouteSet = response.header(h_RecordRoutes).reverse();
         }
         else
         {
            // Ensure that if the route-set in the 200 is empty, then we overwrite any existing route-sets
            mRouteSet.clear();
         }
      }

      // !jf! should this only be for 2xx responses? !jf! Propose no as an
      // answer !dcm! what is he on?
      switch (response.header(h_CSeq).method())
      {
         case INVITE:
            if (mInviteSession == 0)
            {
               DebugLog ( << "Dialog::dispatch  --  Created new client invite session" << msg.brief());

               mInviteSession = makeClientInviteSession(response);
               if (mInviteSession)
               {
                  mInviteSession->dispatch(response);
               }
               else
               {
                  ErrLog( << "Dialog::dispatch  --  Unable to create invite session from response" << msg.brief());
               }
            }
            else
            {
               mInviteSession->dispatch(response);
            }
            break;
         case BYE:
         case ACK:
         case CANCEL:
         case INFO:
         case MESSAGE:
         case UPDATE:
         case PRACK:
            if (mInviteSession)
            {
               mInviteSession->dispatch(response);
            }
            // else drop on the floor
            break;       

         case REFER:
            if(mInviteSession)
            {
               if (code >= 300)
               {
                  mDum.mInviteSessionHandler->onReferRejected(mInviteSession->getSessionHandle(), msg);
               }
               else
               {
                  //!dys! the OR condition below is not draft compliant.
                  if (!mInviteSession->mReferSub && 
                      ((msg.exists(h_ReferSub) && msg.header(h_ReferSub).value()=="false") || 
                       !msg.exists(h_ReferSub)))
                  {
                     DebugLog(<< "refer accepted with norefersub");
                     mDum.mInviteSessionHandler->onReferAccepted(mInviteSession->getSessionHandle(), ClientSubscriptionHandle::NotValid(), msg);
                  }
                  // else no need for action - first Notify will cause onReferAccepted to be called
               }
               mInviteSession->nitComplete();
               break;
            }
            // fall through, out of dialog refer was sent.

         case SUBSCRIBE:
         {
            int code = response.header(h_StatusLine).statusCode();
            ClientSubscription* client = findMatchingClientSub(response);
            if (client)
            {
               client->dispatch(response);
            }
            else
            {
               //!dcm! -- can't subscribe in an existing Dialog, this is all
               //a bit of a hack; currently, spurious failure messages may cause callbacks
               BaseCreator* creator = mDialogSet.getCreator();
               if (!creator || !creator->getLastRequest()->exists(h_Event))
               {
                  return;
               }
               else
               {
                  ClientSubscriptionHandler* handler =
                     mDum.getClientSubscriptionHandler(creator->getLastRequest()->header(h_Event).value());
                  if (handler)
                  {
                     ClientSubscription* sub = makeClientSubscription(*creator->getLastRequest());
                     mClientSubscriptions.push_back(sub);
                     sub->dispatch(response);
                  }
               }
            }

         }
         break;
         case NOTIFY:
         {
            //Failures are dispatched to all ServerSubsscriptions,
            //which may not be correct as per RFC 5057.

            int code = msg.header(h_StatusLine).statusCode();
            if (code >= 300)
            {
               //!dcm! -- ick, write guard
               mDestroying = true;
               for (list<ServerSubscription*>::iterator it = mServerSubscriptions.begin();
                    it != mServerSubscriptions.end(); )
               {
                  ServerSubscription* s = *it;
                  it++;
                  s->dispatch(msg);
               }
               mDestroying = false;
               possiblyDie();
            }
            else if (code >= 200)
            {
               ServerSubscription* server = findMatchingServerSub(response);
               if (server)
               {
                  server->dispatch(response);
               }
            }
         }
         break;
         default:
            resip_assert(0);
            return;
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

void
Dialog::redirected(const SipMessage& msg)
{
   //Established dialogs are not destroyed by a redirect
   if (!mClientSubscriptions.empty() || !mServerSubscriptions.empty())
   {
      return;
   }
   if (mInviteSession)
   {
      ClientInviteSession* cInv = dynamic_cast<ClientInviteSession*>(mInviteSession);
      if (cInv)
      {
         cInv->handleRedirect(msg);
         mReUseDialogSet = true;  // Set flag so that DialogSet will not be destroyed and new Request can use it
      }
   }
}

void
Dialog::makeRequest(SipMessage& request, MethodTypes method, bool incrementCSeq)
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
   request.remove(h_Replaces);

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
      resip_assert(request.exists(h_Vias));
   }

   //don't increment CSeq for ACK or CANCEL
   if (method != ACK && method != CANCEL)
   {
      if(incrementCSeq)
      {
         setRequestNextCSeq(request);
      }
   }
   else
   {
      // ACK and cancel have a minimal header set
      request.remove(h_Accepts);
      request.remove(h_AcceptEncodings);
      request.remove(h_AcceptLanguages);
      request.remove(h_Allows);
      request.remove(h_Requires);
      request.remove(h_ProxyRequires);
      request.remove(h_Supporteds);
      // request.header(h_CSeq).sequence() = ?;  // Caller should provide original request, or modify CSeq to proper value after calling this method
   }

   // If method is INVITE then advertise required headers
   if(method == INVITE || method == UPDATE)
   {
      // Add Advertised Capabilities
      mDum.setAdvertisedCapabilities(request, mDialogSet.mUserProfile);
   }

   if (mDialogSet.mUserProfile->isAnonymous())
   {
      request.header(h_Privacys).push_back(PrivacyCategory(Symbols::id));
   }

   DebugLog ( << "Dialog::makeRequest: " << std::endl << std::endl << request );
}

void
Dialog::makeResponse(SipMessage& response, const SipMessage& request, int code)
{
   resip_assert( code >= 100 );
   response.remove(h_Contacts);
   if (code < 300 && code > 100)
   {
      resip_assert(request.isRequest());
      resip_assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE ||
             request.header(h_RequestLine).getMethod() == BYE ||
             request.header(h_RequestLine).getMethod() == CANCEL ||
             request.header(h_RequestLine).getMethod() == REFER ||
             request.header(h_RequestLine).getMethod() == MESSAGE ||
             request.header(h_RequestLine).getMethod() == NOTIFY ||
             request.header(h_RequestLine).getMethod() == INFO ||
             request.header(h_RequestLine).getMethod() == OPTIONS ||
             request.header(h_RequestLine).getMethod() == PRACK ||
             request.header(h_RequestLine).getMethod() == UPDATE
             );

//      assert (request.header(h_RequestLine).getMethod() == CANCEL ||  // Contact header is not required for Requests that do not form a dialog
//		      request.header(h_RequestLine).getMethod() == BYE ||
//		      request.header(h_Contacts).size() == 1);
      Helper::makeResponse(response, request, code, mLocalContact);
      response.header(h_To).param(p_tag) = mId.getLocalTag();

      if((request.header(h_RequestLine).getMethod() == INVITE ||
          request.header(h_RequestLine).getMethod() == PRACK ||
          request.header(h_RequestLine).getMethod() == UPDATE)
         && code >= 200 && code < 300)
      {
         // Add Advertised Capabilities
         mDum.setAdvertisedCapabilities(response, mDialogSet.mUserProfile);
      }
   }
   else
   {
      Helper::makeResponse(response, request, code);
      response.header(h_To).param(p_tag) = mId.getLocalTag();
   }

   DebugLog ( << "Dialog::makeResponse: " << std::endl << std::endl << response);
}

void 
Dialog::setRequestNextCSeq(SipMessage& request)
{
   resip_assert(request.isRequest() && request.method() != ACK && request.method() != CANCEL);
   request.header(h_CSeq).sequence() = ++mLocalCSeq;
}

ClientInviteSession*
Dialog::makeClientInviteSession(const SipMessage& response)
{
   InviteSessionCreator* creator = dynamic_cast<InviteSessionCreator*>(mDialogSet.getCreator());
   if (!creator)
   {
      resip_assert(0); // !jf! this maybe can assert by evil UAS
      return 0;
   }
   //return mDum.createAppClientInviteSession(*this, *creator);
   return new ClientInviteSession(mDum, *this, creator->getLastRequest(),
                                  creator->getInitialOffer(), creator->getEncryptionLevel(), creator->getServerSubscription());
}

ClientSubscription*
Dialog::makeClientSubscription(const SipMessage& request)
{
   return new ClientSubscription(mDum, *this, request);
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

Dialog::Exception::Exception(const Data& msg, const Data& file, int line)
   : BaseException(msg, file, line)
{
}


void
Dialog::send(SharedPtr<SipMessage> msg)
{
   if (msg->isRequest() && msg->header(h_CSeq).method() != ACK)
   {
      mRequests[msg->header(h_CSeq).sequence()] = msg;
   }
   mDum.send(msg);
}

void
Dialog::onForkAccepted()
{
   ClientInviteSession* uac = dynamic_cast<ClientInviteSession*>(mInviteSession);
   if (uac)
   {
      uac->onForkAccepted();
   }
}

void 
Dialog::possiblyDie()
{
   if (!mDestroying)
   {
      if (mClientSubscriptions.empty() &&
          mServerSubscriptions.empty() &&
          !mInviteSession)
      {
         mDestroying = true;
         mDum.destroy(this);
      }
   }
}

void
Dialog::flowTerminated()
{
   // Clear the network association
   mNetworkAssociation.clear();
   
   // notify server subscirption dialogs
   std::list<ServerSubscription*> tempServerList = mServerSubscriptions;  // Create copy since subscription can be deleted
   for (std::list<ServerSubscription*>::iterator is=tempServerList.begin();
        is != tempServerList.end(); ++is)
   {
      (*is)->flowTerminated();
   }

   // notify client subscription dialogs
   std::list<ClientSubscription*> tempClientList = mClientSubscriptions;  // Create copy since subscription can be deleted
   for (std::list<ClientSubscription*>::iterator ic=tempClientList.begin();
        ic != tempClientList.end(); ++ic)
   {
      (*ic)->flowTerminated();
   }

   // notify invite session dialog
   if (mInviteSession)
   {
      mInviteSession->flowTerminated();
   }
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const Dialog& dialog)
{
   strm
      << "mClientSubscriptions("
      << dialog.mClientSubscriptions.size()
      << "), "
      << "mServerSubscriptions("
      << dialog.mServerSubscriptions.size()
      << ")";
   return strm;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */



