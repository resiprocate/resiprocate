#include <algorithm>
#include <iterator>

#include "resip/stack/Helper.hxx"
#include "resip/dum/BaseCreator.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/UsageUseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Random.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

static const int UnreasonablyLowExpirationThreshold = 7;  // The threshold before which we consider a contacts expiry to be unreasonably low

ClientRegistrationHandle
ClientRegistration::getHandle()
{
   return ClientRegistrationHandle(mDum, getBaseHandle().getId());
}

ClientRegistration::ClientRegistration(DialogUsageManager& dum,
                                       DialogSet& dialogSet,
                                       SharedPtr<SipMessage> request)
   : NonDialogUsage(dum, dialogSet),
     mLastRequest(request),
     mTimerSeq(0),
     mState(mLastRequest->exists(h_Contacts) ? Adding : Querying),
     mEnding(false),
     mEndWhenDone(false),
     mUserRefresh(false),
     mRegistrationTime(mDialogSet.mUserProfile->getDefaultRegistrationTime()),
     mExpires(0),
     mRefreshTime(0),
     mQueuedState(None),
     mQueuedRequest(new SipMessage)
{
   // If no Contacts header, this is a query
   if (mLastRequest->exists(h_Contacts))
   {
      NameAddr all;
      all.setAllContacts();
      if(!(mLastRequest->header(h_Contacts).front() == all))
      {
         // store if not special all contacts header
         mMyContacts = mLastRequest->header(h_Contacts);
      }
   }

   if(mLastRequest->exists(h_Expires) && 
      mLastRequest->header(h_Expires).isWellFormed())
   {
      mRegistrationTime = mLastRequest->header(h_Expires).value();
   }

   mNetworkAssociation.setDum(&dum);
}

ClientRegistration::~ClientRegistration()
{
   DebugLog ( << "ClientRegistration::~ClientRegistration" );
   mDialogSet.mClientRegistration = 0;

   // !dcm! Will not interact well with multiple registrations from the same AOR
   mDialogSet.mUserProfile->setServiceRoute(NameAddrs());
}

void
ClientRegistration::addBinding(const NameAddr& contact)
{
   addBinding(contact, mDialogSet.mUserProfile->getDefaultRegistrationTime());
}

SharedPtr<SipMessage>
ClientRegistration::tryModification(ClientRegistration::State state)
{
   if (mState != Registered)
   {
      if(mState != RetryAdding && mState != RetryRefreshing)
      {
         if (mQueuedState != None)
         {
            WarningLog (<< "Trying to modify bindings when another request is already queued");
            throw UsageUseException("Queuing multiple requests for Registration Bindings", __FILE__,__LINE__);
         }

         *mQueuedRequest = *mLastRequest;
         mQueuedState = state;

         return mQueuedRequest;
      }
      else
      {
          ++mTimerSeq;  // disable retry timer
      }
   }

   resip_assert(mQueuedState == None);
   mState = state;

   return mLastRequest;
}

void
ClientRegistration::addBinding(const NameAddr& contact, UInt32 registrationTime)
{
   SharedPtr<SipMessage> next = tryModification(Adding);
   mMyContacts.push_back(contact);
   tagContact(mMyContacts.back());

   next->header(h_Contacts) = mMyContacts;
   mRegistrationTime = registrationTime;
   next->header(h_Expires).value() = mRegistrationTime;
   next->header(h_CSeq).sequence()++;
   // caller prefs

   if (mQueuedState == None)
   {
      send(next);
   }
}

void
ClientRegistration::removeBinding(const NameAddr& contact)
{
   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }

   SharedPtr<SipMessage> next = tryModification(Removing);
   for (NameAddrs::iterator i=mMyContacts.begin(); i != mMyContacts.end(); i++)
   {
      if (i->uri() == contact.uri())
      {
         next->header(h_Contacts).clear();
         next->header(h_Contacts).push_back(*i);
         next->header(h_Expires).value() = 0;
         next->header(h_CSeq).sequence()++;

         if (mQueuedState == None)
         {
            send(next);
         }

         mMyContacts.erase(i);
         return;
      }
   }

   // !jf! What state are we left in now?
   throw Exception("No such binding", __FILE__, __LINE__);
}

void
ClientRegistration::removeAll(bool stopRegisteringWhenDone)
{
   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }

   SharedPtr<SipMessage> next = tryModification(Removing);

   mAllContacts.clear();
   mMyContacts.clear();

   NameAddr all;
   all.setAllContacts();
   next->header(h_Contacts).clear();
   next->header(h_Contacts).push_back(all);
   next->header(h_Expires).value() = 0;
   next->header(h_CSeq).sequence()++;
   mEndWhenDone = stopRegisteringWhenDone;

   if (mQueuedState == None)
   {
      send(next);
   }
}

void
ClientRegistration::removeMyBindings(bool stopRegisteringWhenDone)
{
   InfoLog (<< "Removing binding");

   if (mState == Removing)
   {
      WarningLog (<< "Already removing a binding");
      throw UsageUseException("Can't remove binding when already removing registration bindings", __FILE__,__LINE__);
   }

   if (mMyContacts.empty())
   {
      WarningLog (<< "No bindings to remove");
      throw UsageUseException("No bindings to remove", __FILE__,__LINE__);
   }

   SharedPtr<SipMessage> next = tryModification(Removing);

   next->header(h_Contacts) = mMyContacts;
   mMyContacts.clear();

   NameAddrs& myContacts = next->header(h_Contacts);

   for (NameAddrs::iterator i=myContacts.begin(); i != myContacts.end(); i++)
   {
      i->param(p_expires) = 0;
   }

   next->remove(h_Expires);
   next->header(h_CSeq).sequence()++;

   // !jf! is this ok if queued
   mEndWhenDone = stopRegisteringWhenDone;

   if (mQueuedState == None)
   {
      if(mEnding && whenExpires() == 0)
      {
         resip_assert(mEndWhenDone);  // will always be true when mEnding is true
         // We are not actually registered, and we are ending - no need to send un-register - just terminate now
         stopRegistering();
         return;
      }
      send(next);
   }
}

class ClientRegistrationRemoveMyBindings : public DumCommandAdapter
{
public:
   ClientRegistrationRemoveMyBindings(const ClientRegistrationHandle& clientRegistrationHandle, bool stopRegisteringWhenDone)
      : mClientRegistrationHandle(clientRegistrationHandle),
        mStopRegisteringWhenDone(stopRegisteringWhenDone)
   {
   }

   virtual void executeCommand()
   {
      if(mClientRegistrationHandle.isValid())
      {
         mClientRegistrationHandle->removeMyBindings(mStopRegisteringWhenDone);
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientRegistrationRemoveMyBindings";
   }
private:
   ClientRegistrationHandle mClientRegistrationHandle;
   bool mStopRegisteringWhenDone;
};

void
ClientRegistration::removeMyBindingsCommand(bool stopRegisteringWhenDone)
{
   mDum.post(new ClientRegistrationRemoveMyBindings(getHandle(), stopRegisteringWhenDone));
}

void 
ClientRegistration::stopRegistering()
{
   //timers aren't a concern, as DUM checks for Handle validity before firing.
   delete this;
}

void
ClientRegistration::requestRefresh(UInt32 expires)
{
    // Set flag so that handlers get called for success/failure
    mUserRefresh = true;
    internalRequestRefresh(expires);
}

void
ClientRegistration::internalRequestRefresh(UInt32 expires)
{
   if(mState == RetryAdding || mState == RetryRefreshing)
   {
      // disable retry time and try refresh immediately
      ++mTimerSeq;
   }
   else if (mState != Registered)
   {
      InfoLog (<< "a request is already in progress, no need to refresh " << *this);
      return;
   }

   InfoLog (<< "requesting refresh of " << *this);
   
   mState = Refreshing;
   mLastRequest->header(h_CSeq).sequence()++;
   mLastRequest->header(h_Contacts)=mMyContacts;
   if(expires > 0)
   {
      mRegistrationTime = expires;
   }
   mLastRequest->header(h_Expires).value()=mRegistrationTime;

   send(mLastRequest);
}

const NameAddrs&
ClientRegistration::myContacts()
{
   return mMyContacts;
}

const NameAddrs&
ClientRegistration::allContacts()
{
   return mAllContacts;
}

UInt32
ClientRegistration::whenExpires() const
{
   UInt64 now = Timer::getTimeSecs();
   if(mExpires > now)
   {
       return (UInt32)(mExpires - now);
   }
   else
   {
       return 0;
   }
}

void
ClientRegistration::end()
{
   if(!mEnding)
   {
      mEnding = true;
      removeMyBindings(true);
   }
}

class ClientRegistrationEndCommand : public DumCommandAdapter
{
public:
   ClientRegistrationEndCommand(const ClientRegistrationHandle& clientRegistrationHandle)
      : mClientRegistrationHandle(clientRegistrationHandle)
   {
   }

   virtual void executeCommand()
   {
      if(mClientRegistrationHandle.isValid())
      {
         mClientRegistrationHandle->end();
      }
   }

   virtual EncodeStream& encodeBrief(EncodeStream& strm) const
   {
      return strm << "ClientRegistrationEndCommand";
   }
private:
   ClientRegistrationHandle mClientRegistrationHandle;
};

void
ClientRegistration::endCommand()
{
   mDum.post(new ClientRegistrationEndCommand(getHandle()));
}

EncodeStream& 
ClientRegistration::dump(EncodeStream& strm) const
{
   strm << "ClientRegistration " << mLastRequest->header(h_From).uri();
   return strm;
}

void
ClientRegistration::dispatch(const SipMessage& msg)
{
   try
   {
      // !jf! there may be repairable errors that we can handle here
      resip_assert(msg.isResponse());
      const int& code = msg.header(h_StatusLine).statusCode();
      bool nextHopSupportsOutbound = false;
      int keepAliveTime = 0;

      // If registration was successful - look for next hop indicating support for outbound
      if(mDialogSet.mUserProfile->clientOutboundEnabled() && msg.isExternal() && code >= 200 && code < 300)
      {
         // If ClientOutbound support is enabled and the registration response contains 
         // Requires: outbound or a Path with outbound then outbound is supported by the 
         // server, so store the flow key in the UserProfile 
         // and use it for sending all messages (DialogUsageManager::sendUsingOutboundIfAppropriate)
         try
         {
            if((!msg.empty(h_Paths) && msg.header(h_Paths).back().uri().exists(p_ob)) ||
               (!msg.empty(h_Requires) && msg.header(h_Requires).find(Token(Symbols::Outbound))))
            {
               mDialogSet.mUserProfile->mClientOutboundFlowTuple = msg.getSource();
               mDialogSet.mUserProfile->mClientOutboundFlowTuple.onlyUseExistingConnection = true;
               nextHopSupportsOutbound = true;
               if(!msg.empty(h_FlowTimer))
               {
                  keepAliveTime = msg.header(h_FlowTimer).value();
               }
            }
         }
         catch(BaseException&e)
         {
            ErrLog(<<"Error parsing Path or Requires:" << e);
         }
      }

      if(msg.isFromWire())
      {
         resip::TransportType receivedTransport = toTransportType(
            msg.header(h_Vias).front().transport());
         if(keepAliveTime == 0)
         {
            if(isReliable(receivedTransport))
            {
               keepAliveTime = mDialogSet.mUserProfile->getKeepAliveTimeForStream();
            }
            else
            {
               keepAliveTime = mDialogSet.mUserProfile->getKeepAliveTimeForDatagram();
            }
         }

         if(keepAliveTime > 0)
         {
            mNetworkAssociation.update(msg, keepAliveTime, nextHopSupportsOutbound);
         }
      }

      if (code < 200)
      {
         // throw it away
         return;
      }
      else if (code < 300) // success
      {
         try
         {
            if (msg.exists(h_ServiceRoutes))
            {
               InfoLog(<< "Updating service route: " << Inserter(msg.header(h_ServiceRoutes)));
               mDialogSet.mUserProfile->setServiceRoute(msg.header(h_ServiceRoutes));
            }
            else
            {
               DebugLog(<< "Clearing service route (" << Inserter(getUserProfile()->getServiceRoute()) << ")");
               mDialogSet.mUserProfile->setServiceRoute(NameAddrs());
            }
         }
         catch(BaseException &e)
         {
            ErrLog(<< "Error Parsing Service Route:" << e);
         }    

         //gruu update, should be optimized
         try
         {
            if(mDialogSet.mUserProfile->gruuEnabled() && msg.exists(h_Contacts))
            {
               for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin(); 
                    it != msg.header(h_Contacts).end(); it++)
               {
                  if (it->exists(p_Instance) && it->param(p_Instance) == mDialogSet.mUserProfile->getInstanceId())
                  {
                     if(it->exists(p_pubGruu))
                     {
                        mDialogSet.mUserProfile->setPublicGruu(Uri(it->param(p_pubGruu)));
                     }
                     if(it->exists(p_tempGruu))
                     {
                        mDialogSet.mUserProfile->setTempGruu(Uri(it->param(p_tempGruu)));
                     }
                     break;
                   }
               }
            }
         }
         catch(BaseException&e)
         {
            ErrLog(<<"Error parsing GRUU:" << e);
         }
         
         // !jf! consider what to do if no contacts
         // !ah! take list of ctcs and push into mMy or mOther as required.

         // make timers to re-register
         UInt64 nowSecs = Timer::getTimeSecs();
         UInt32 expiry = calculateExpiry(msg);
         mExpires = nowSecs + expiry;
         if(msg.exists(h_Contacts))
         {
            mAllContacts = msg.header(h_Contacts);
         }
         else
         {
            mAllContacts.clear();
         }

         if (expiry != 0 && expiry != UINT_MAX)
         {
            if(expiry >= UnreasonablyLowExpirationThreshold)
            {
               int exp = Helper::aBitSmallerThan(expiry);
               mRefreshTime = exp + nowSecs;
               mDum.addTimer(DumTimeout::Registration,
                             exp,
                             getBaseHandle(),
                             ++mTimerSeq);
            }
            else
            {
               WarningLog(<< "Server is using an unreasonably low expiry: " 
                           << expiry 
                           << " We're just going to end this registration.");
               end();
               return;
            }
         }

         switch (mState)
         {
            case Querying:
            case Adding:
               if(expiry != 0)
               {
                  mState = Registered;
                  mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
               }
               else
               {
                  mDum.mClientRegistrationHandler->onRemoved(getHandle(), msg);
                  checkProfileRetry(msg);
               }
               break;

            case Removing:
               //mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
               mDum.mClientRegistrationHandler->onRemoved(getHandle(), msg);
               InfoLog (<< "Finished removing registration " << *this << " mEndWhenDone=" << mEndWhenDone);
               if (mEndWhenDone)
               {
                  // !kh!
                  // stopRegistering() deletes 'this'
                  // furthur processing makes no sense
                  //assert(mQueuedState == None);
                  stopRegistering();
                  return;
               }
               break;

            case Registered:
            case Refreshing:
               mState = Registered;
               if(expiry != 0)
               {
                  if(mUserRefresh)
                  {
                      mUserRefresh = false;
                      mDum.mClientRegistrationHandler->onSuccess(getHandle(), msg);
                  }
               }
               else
               {
                  mDum.mClientRegistrationHandler->onRemoved(getHandle(), msg);
                  checkProfileRetry(msg);
               }
               break;

            default:
               break;
         }

         if (mQueuedState != None)
         {
            if(mQueuedState == Removing && mEnding && whenExpires() == 0)
            {
               resip_assert(mEndWhenDone);  // will always be true when mEnding is true
               // We are not actually registered, and we are ending - no need to send un-register - just terminate now
               stopRegistering();
               return;
            }
            InfoLog (<< "Sending queued request: " << *mQueuedRequest);
            mState = mQueuedState;
            mQueuedState = None;
            *mLastRequest = *mQueuedRequest;
            send(mLastRequest);
         }
      }
      else
      {
         if((mState == Adding || mState == Refreshing) && !mEndWhenDone)
         {
            if (code == 423) // interval too short
            {
               UInt32 maxRegistrationTime = mDialogSet.mUserProfile->getDefaultMaxRegistrationTime();
               if (msg.exists(h_MinExpires) && 
                   (maxRegistrationTime == 0 || msg.header(h_MinExpires).value() < maxRegistrationTime)) // If maxRegistrationTime is enabled, then check it
               {
                  mRegistrationTime = msg.header(h_MinExpires).value();
                  mLastRequest->header(h_Expires).value() = mRegistrationTime;
                  mLastRequest->header(h_CSeq).sequence()++;
                  send(mLastRequest);
                  return;
               }
            }
            else if (code == 408 || (code == 503 && !msg.isFromWire()))
            {
               int retry = mDum.mClientRegistrationHandler->onRequestRetry(getHandle(), 0, msg);
            
               if (retry < 0)
               {
                  DebugLog(<< "Application requested failure on Retry-After");
               }
               else if (retry == 0)
               {
                  DebugLog(<< "Application requested immediate retry on 408 or internal 503");
               
                  mLastRequest->header(h_CSeq).sequence()++;
                  send(mLastRequest);
                  mUserRefresh = true;  // Reset this flag, so that the onSuccess callback will be called if we are successful when re-trying
                  return;
               }
               else
               {
                  DebugLog(<< "Application requested delayed retry on 408 or internal 503: " << retry);
                  mRefreshTime = 0;
                  switch(mState)
                  {
                  case Adding:
                     mState = RetryAdding;
                     break;
                  case Refreshing:
                     mState = RetryRefreshing;
                     break;
                  default:
                     resip_assert(false);
                     break;
                  }
                  if(mDum.mClientAuthManager.get()) mDum.mClientAuthManager.get()->clearAuthenticationState(DialogSetId(*mLastRequest));
                  mDum.addTimer(DumTimeout::RegistrationRetry, 
                                retry, 
                                getBaseHandle(),
                                ++mTimerSeq);       
                  mUserRefresh = true;  // Reset this flag, so that the onSuccess callback will be called if we are successful when re-trying
                  return;
               }
            }
         }
         
         mDum.mClientRegistrationHandler->onFailure(getHandle(), msg);
         mUserRefresh = true;  // Reset this flag, so that the onSuccess callback will be called if we are successful when re-trying

         // Retry if Profile setting is set
         unsigned int retryInterval = checkProfileRetry(msg);
         if(retryInterval > 0)
         {
            InfoLog( << "Registration error " << code << " for " << msg.header(h_To) << ", retrying in " << retryInterval << " seconds.");
            return;
         }

         // assume that if a failure occurred, the bindings are gone
         if (mEndWhenDone)
         {
            mDum.mClientRegistrationHandler->onRemoved(getHandle(), msg);
         }
         delete this;
      }
   }
   catch(BaseException& e)
   {
      InfoLog( << "Exception in ClientRegistration::dispatch: "  <<  e.getMessage());
      mDum.mClientRegistrationHandler->onFailure(getHandle(), msg);
      delete this;
   }
}

void 
ClientRegistration::tagContact(NameAddr& contact) const
{
   tagContact(contact, mDum, mDialogSet.mUserProfile);
}

void 
ClientRegistration::tagContact(NameAddr& contact, DialogUsageManager& dum, SharedPtr<UserProfile>& userProfile)
{
   if(contact.uri().host().empty() || 
      dum.getSipStack().isMyDomain(contact.uri().host(), contact.uri().port()))
   {
      // Contact points at us; it is appropriate to add a +sip.instance to 
      // this Contact. We don't need to have full gruu support enabled to add
      // a +sip.instance either...
      if(userProfile->hasInstanceId())
      {
         contact.param(p_Instance) = userProfile->getInstanceId();
         if(userProfile->getRegId() != 0)
         {
            contact.param(p_regid) = userProfile->getRegId();
         }
      }
      else if(userProfile->getRinstanceEnabled())
      {
         // !slg! poor mans instance id so that we can tell which contacts 
         // are ours - to be replaced by gruu someday.
         InfoLog(<< "You really should consider setting an instance id in"
                     " the UserProfile (see UserProfile::setInstanceId())."
                     " This is really easy, and makes this class much less "
                     "likely to clash with another endpoint registering at "
                     "the same AOR.");
         contact.uri().param(p_rinstance) = Random::getCryptoRandomHex(8);  
      }
      else if(!contact.uri().user().empty())
      {
         WarningLog(<< "Ok, not only have you not specified an instance id, "
                  "you have disabled the rinstance hack (ie; resip's \"poor"
                  " man's +sip.instance\"). We will try to match Contacts"
                  " based on what you've put in the user-part of your "
                  "Contact, but this can be dicey, especially if you've put"
                  " something there that another endpoint is likely to "
                  "use.");
      }
      else
      {
         ErrLog(<< "Ok, not only have you not specified an instance id, "
                  "you have disabled the rinstance hack (ie; resip's \"poor"
                  " man's +sip.instance\"), _and_ you haven't put anything"
                  " in the user-part of your Contact. This is asking for "
                  "confusion later. We'll do our best to try to match things"
                  " up later when the response comes in...");
      }
   }
   else
   {
      // Looks like a third-party registration. +sip.instance is out of the 
      // question, but we can still use rinstance.
      if(userProfile->getRinstanceEnabled())
      {
         // !slg! poor mans instance id so that we can tell which contacts 
         // are ours - to be replaced by gruu someday.
         contact.uri().param(p_rinstance) = Random::getCryptoRandomHex(8);  
      }
      else if(!contact.uri().user().empty())
      {
         WarningLog(<< "You're trying to do a third-party registration, but "
                  "you have disabled the rinstance hack (ie; resip's \"poor"
                  " man's +sip.instance\"). We will try to match Contacts"
                  " based on what you've put in the user-part of your "
                  "Contact, but this can be dicey, especially if you've put"
                  " something there that another endpoint is likely to "
                  "use.");
      }
      else
      {
         ErrLog(<< "You're trying to do a third-party registration,  and "
                  "not only have you disabled the rinstance hack (ie; "
                  "resip's \"poor man's +sip.instance\"), you haven't"
                  " put anything in the user-part of your Contact. This is "
                  "asking for confusion later. We'll do our best to try to "
                  "match things up later when the response comes in...");
      }
   }
   
   if (userProfile->getMethodsParamEnabled())
   {
      contact.param(p_methods) = dum.getMasterProfile()->getAllowedMethodsData();
   }

   // ?bwc? Host and port override?
}

unsigned long 
ClientRegistration::calculateExpiry(const SipMessage& reg200) const
{
   unsigned long expiry=mRegistrationTime;
   if(reg200.exists(h_Expires) &&
      reg200.header(h_Expires).isWellFormed() &&
      reg200.header(h_Expires).value() < expiry)
   {
      expiry=reg200.header(h_Expires).value();
   }

   if(!reg200.exists(h_Contacts))
   {
      return expiry;
   }

   const NameAddrs& contacts(reg200.header(h_Contacts));

   // We are going to track two things here:
   // 1. expiry - the lowest expiration value of all of our contacts
   // 2. reasonableExpiry - the lowest expiration value of all of our contacts
   //                       that is above the UnreasonablyLowExpirationThreshold (7 seconds)
   // Before we return, if expiry is less than UnreasonablyLowExpirationThreshold
   // but we had another contact that had a reasonable expiry value, then return
   // that value instead.  This logic covers a very interesting scenario:
   //
   // Consider the case where we are registered over TCP due to DNS SRV record
   // configuration.  Let's say an administrator reconfigures the DNS records to
   // now make UDP the preferred transport.  When we re-register we will now
   // send the re-registration message over UDP.  This will cause our contact
   // address to be changed (ie: ;tranport=tcp will no longer exist).  So for a
   // short period of time the registrar will return two contacts to us, both 
   // belonging to us, one for TCP and one for UDP.  The TCP one will expire in
   // a short amount of time, and if we return this expiry to the dispatch() 
   // method then it will cause the ClientRegistration to end (see logic in 
   // dispatch() that prints out the error "Server is using an unreasonably low 
   // expiry: "...
   unsigned long reasonableExpiry = 0xFFFFFFFF;

   for(NameAddrs::const_iterator c=contacts.begin();c!=contacts.end();++c)
   {
      // Our expiry is never going to increase if we find one of our contacts, 
      // so if the expiry is not lower, we just ignore it. For registrars that
      // leave our requested expiry alone, this code ends up being pretty quick,
      // especially if there aren't contacts from other endpoints laying around.     
      if(c->isWellFormed() && c->exists(p_expires))
      {
         unsigned long contactExpires = c->param(p_expires);
         if((contactExpires < expiry ||
             contactExpires < reasonableExpiry) &&
            contactIsMine(*c))
         {
            expiry = contactExpires;
            if(contactExpires >= UnreasonablyLowExpirationThreshold)
            {
                reasonableExpiry = contactExpires;
            }
         }
      }
   }
   // If expiry is less than UnreasonablyLowExpirationThreshold and we have another
   // contact that has a reasonable expiry value, then return that value instead.
   // See large comment above for more details.
   if(expiry < UnreasonablyLowExpirationThreshold && reasonableExpiry != 0xFFFFFFFF)
   {
       expiry = reasonableExpiry;
   }
   return expiry;
}

bool 
ClientRegistration::contactIsMine(const NameAddr& contact) const
{
   // Try to find this contact in mMyContacts
   if(mDialogSet.mUserProfile->hasInstanceId() && 
      contact.exists(p_Instance))
   {
      return contact.param(p_Instance)==mDialogSet.mUserProfile->getInstanceId();
   }
   else if(mDialogSet.mUserProfile->getRinstanceEnabled() &&
            contact.uri().exists(p_rinstance))
   {
      return rinstanceIsMine(contact.uri().param(p_rinstance));
   }
   else
   {
      return searchByUri(contact.uri());
   }
}

bool 
ClientRegistration::rinstanceIsMine(const Data& rinstance) const
{
   // !bwc! This could be made faster if we used a single rinstance...
   for(NameAddrs::const_iterator m=mMyContacts.begin(); m!=mMyContacts.end(); ++m)
   {
      if(m->uri().exists(p_rinstance) && m->uri().param(p_rinstance)==rinstance)
      {
         return true;
      }
   }
   return false;
}

bool 
ClientRegistration::searchByUri(const Uri& cUri) const
{
   for(NameAddrs::const_iterator m=mMyContacts.begin(); m!=mMyContacts.end(); ++m)
   {
      if(m->uri()==cUri)
      {
         return true;
      }
      else if(m->uri().host().empty() && 
               m->uri().user()==cUri.user() &&
               m->uri().scheme()==cUri.scheme() &&
               mDum.getSipStack().isMyDomain(cUri.host(), cUri.port()))
      {
         // Empty host-part in our contact; this means we're relying on the 
         // stack to fill out this Contact header. Also, the user-part matches.
         return true;
      }
   }
   return false;
}

unsigned int 
ClientRegistration::checkProfileRetry(const SipMessage& msg)
{
   unsigned int retryInterval = mDialogSet.mUserProfile->getDefaultRegistrationRetryTime();
   if (retryInterval > 0 &&
      (mState == Adding || mState == Refreshing) &&
      !mEndWhenDone)
   {
      if (msg.exists(h_RetryAfter) && msg.header(h_RetryAfter).value() > 0)
      {
         // Use retry interval from error response
         retryInterval = msg.header(h_RetryAfter).value();
      }
      mRefreshTime = 0;
      switch(mState)
      {
      case Adding:
         mState = RetryAdding;
         break;
      case Refreshing:
         mState = RetryRefreshing;
         break;
      default:
         resip_assert(false);
         break;
      }

      if(mDum.mClientAuthManager.get()) mDum.mClientAuthManager.get()->clearAuthenticationState(DialogSetId(*mLastRequest));
      mDum.addTimer(DumTimeout::RegistrationRetry,
         retryInterval,
         getBaseHandle(),
         ++mTimerSeq);
      return retryInterval;
   }
   return 0;
}

void
ClientRegistration::dispatch(const DumTimeout& timer)
{
   switch(timer.type())
   {
      case DumTimeout::Registration:
         // If you happen to be Adding/Updating when the timer goes off, you should just ignore
         // it since a new timer will get added when the 2xx is received.
         if (timer.seq() == mTimerSeq && mState == Registered)
         {
            if (!mMyContacts.empty())
            {
               internalRequestRefresh();
            }
         }
         break;

      case DumTimeout::RegistrationRetry:
         if (timer.seq() == mTimerSeq)
         {
            switch(mState)
            {
            case RetryAdding:
               mState = Adding;
               break;
            case RetryRefreshing:
               mState = Refreshing;
               break;
            default:
              resip_assert(false);
              break;
            }

            // Resend last request
            mLastRequest->header(h_CSeq).sequence()++;
            mLastRequest->remove(h_ProxyAuthorizations);
            mLastRequest->remove(h_Authorizations); 
            send(mLastRequest);
         }
         break;
      default:
         break;
   }
}

void 
ClientRegistration::flowTerminated()
{
   // Clear the network association
   mNetworkAssociation.clear();

   // Notify application - not default handler implementation is to immediately attempt
   // a re-registration in order to form a new flow
   mDum.mClientRegistrationHandler->onFlowTerminated(getHandle());
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
