#if !defined(PresenceSubscriptionHandler_hxx)
#define PresenceSubscriptionHandler_hxx

#include <set>
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/DumCommand.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include <resip/dum/InMemorySyncRegDb.hxx>
#include <resip/dum/InMemorySyncPubDb.hxx>

namespace repro
{
class Dispatcher;
}

namespace resip
{
class SipMessage;
class SecurityAttributes;
class Data;
class Contents;
}

namespace repro
{

class PresenceSubscriptionHandler;

// This class is used to do an aynchronous check for user existence
class PresenceUserExists : public resip::DumCommand
{
public:
   PresenceUserExists(resip::DialogUsageManager& dum, PresenceSubscriptionHandler* handler, resip::ServerSubscriptionHandle h,
      bool sendAcceptReject, const resip::Uri& aor) :
      mDum(dum), mSubscriptionHandler(handler), mServerSubscriptionHandle(h), mSendAcceptReject(sendAcceptReject),
      mUserExists(false), mAor(aor) { mTu = &dum; }
   PresenceUserExists(const PresenceUserExists& orig) : DumCommand(orig),
      mDum(orig.mDum), mSubscriptionHandler(orig.mSubscriptionHandler), mServerSubscriptionHandle(orig.mServerSubscriptionHandle),
      mSendAcceptReject(orig.mSendAcceptReject), mUserExists(orig.mUserExists), mAor(orig.mAor)  {}
   virtual PresenceUserExists* clone() const { return new PresenceUserExists(*this); }

   virtual EncodeStream& encode(EncodeStream& ostr) const { ostr << "PresenceUserExists: " << mAor; return ostr; }
   virtual EncodeStream& encodeBrief(EncodeStream& ostr) const { return encode(ostr); }

   virtual void executeCommand();
   virtual void setUserExists(bool exists) { mUserExists = exists; }
   virtual bool getUserExists() { return mUserExists; }

   virtual const resip::Data getUser() { return mAor.user(); }
   virtual const resip::Data getDomain() { return mAor.host(); }

private:
   resip::DialogUsageManager& mDum;
   PresenceSubscriptionHandler* mSubscriptionHandler;
   resip::ServerSubscriptionHandle mServerSubscriptionHandle;
   bool mSendAcceptReject;
   bool mUserExists;
   resip::Uri mAor;
};

class PresenceServerSubscriptionRegFunctor;
class PresenceServerSubscriptionFunctor;
class PresenceServerRegStateChangeCommand;
class PresenceServerDocStateChangeCommand;
class PresenceServerCheckDocExpiredCommand;
class PresenceSubscriptionHandler : public resip::ServerSubscriptionHandler, 
                                    public resip::PublicationPersistenceManager::ETagMerger,
                                    public resip::InMemorySyncRegDbHandler,
                                    public resip::InMemorySyncPubDbHandler
{
public:
    PresenceSubscriptionHandler(resip::DialogUsageManager& dum,
                               repro::Dispatcher* userDispatcher,
                               bool presenceUsesRegistrationState,
                               bool PresenceNotifyClosedStateForNonPublishedUsers);
    virtual ~PresenceSubscriptionHandler();

    // ServerSubscriptionHandler interfaces
    virtual void onNewSubscription(resip::ServerSubscriptionHandle h, const resip::SipMessage& sub);
    virtual void onRefresh(resip::ServerSubscriptionHandle, const resip::SipMessage& sub);
    virtual void onPublished(resip::ServerSubscriptionHandle associated,
                             resip::ServerPublicationHandle publication,
                             const resip::Contents* contents,
                             const resip::SecurityAttributes* attrs);
    virtual void onTerminated(resip::ServerSubscriptionHandle);
    virtual void onError(resip::ServerSubscriptionHandle, const resip::SipMessage& msg);

    // PublicationPersistenceManager::ETagMerger interface
    virtual bool mergeETag(resip::Contents* eTagDest, resip::Contents* eTagSrc, bool isFirst);

    // InMemorySyncRegDb handler interface
    virtual void onAorModified(const resip::Uri& aor, const resip::ContactList& contacts);

    // InMemorySyncPubDb handler interface
    virtual void onDocumentModified(bool sync, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const resip::Contents* contents, const resip::SecurityAttributes* securityAttributes);
    virtual void onDocumentRemoved(bool sync, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated);

protected:
    resip::DialogUsageManager& mDum;
    resip::InMemorySyncPubDb* mPublicationDb;
    resip::InMemorySyncRegDb* mRegistrationDb;
    void notifyPresence(resip::ServerSubscriptionHandle h, bool sendAcceptReject);
    void notifyPresenceNoPublication(resip::ServerSubscriptionHandle h, bool sendAcceptReject, const resip::Uri& aor, bool isRegistered, UInt64 regMaxExpires);
    bool sendPublishedPresence(resip::ServerSubscriptionHandle h, bool sendAcceptReject);
    void adjustNotifyExpiresTime(resip::SipMessage& notify, UInt64 regMaxExpires);
    void fabricateSimplePresence(resip::ServerSubscriptionHandle h, bool sendAcceptReject, const resip::Uri& aor, bool online, UInt64 regMaxExpires);
    void continueNotifyPresenceAfterUserExistsCheck(resip::ServerSubscriptionHandle h, bool sendAcceptReject, const resip::Uri& aor, bool userExists);
    bool checkRegistrationStateChanged(const resip::Uri& aor, bool registered, UInt64 regMaxExpires);
    void notifySubscriptions(const resip::Data& documentKey);
    void checkExpired(const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated);
    friend class PresenceServerSubscriptionRegFunctor;
    friend class PresenceServerSubscriptionFunctor;
    friend class PresenceServerRegStateChangeCommand;
    friend class PresenceServerDocStateChangeCommand;
    friend class PresenceServerCheckDocExpiredCommand;
    friend class PresenceUserExists;

    bool mPresenceUsesRegistrationState;
    bool mPresenceNotifyClosedStateForNonPublishedUsers;
    Dispatcher* mUserDispatcher;
    std::set<resip::Uri> mOnlineAors;
};
 
}

#endif

/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*/
/*
* vi: set shiftwidth=3 expandtab:
*/
