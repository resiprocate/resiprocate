
#ifndef __RADIUSServerAuthManager_h
#define __RADIUSServerAuthManager_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_RADIUS_CLIENT

#include "rutil/RADIUSDigestAuthenticator.hxx"
#include "resip/dum/ServerAuthManager.hxx"

namespace resip
{

class RADIUSServerAuthManager : public resip::ServerAuthManager {

private:
  resip::DialogUsageManager& dum;

public:
  RADIUSServerAuthManager(resip::DialogUsageManager& dum);
  virtual ~RADIUSServerAuthManager();

protected:
  resip::ServerAuthManager::AsyncBool requiresChallenge(const resip::SipMessage& msg);
  void requestCredential(const resip::Data& user, const resip::Data& realm, const resip::SipMessage& msg, const resip::Auth& auth, const resip::Data& transactionId);
  bool useAuthInt() const;

  bool authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, resip::Uri &fromUri);

  void onAuthSuccess(const resip::SipMessage& msg);
  void onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg);

};

class MyRADIUSDigestAuthListener : public RADIUSDigestAuthListener {
private:
  resip::Data user;
  resip::Data realm;
  resip::TransactionUser& tu;
  resip::Data transactionId;
public:
  MyRADIUSDigestAuthListener(const resip::Data& user, const resip::Data& realm, resip::TransactionUser& tu, const resip::Data& transactionId);
  virtual ~MyRADIUSDigestAuthListener();
  void onSuccess(const resip::Data& rpid);
  void onAccessDenied();
  void onError();
};

}

#endif

#endif

