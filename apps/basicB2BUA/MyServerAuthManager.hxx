
#ifndef __MyServerAuthManager_h
#define __MyServerAuthManager_h

#include "resip/dum/ServerAuthManager.hxx"

#include "Peer.hxx"
#include "BasicConfiguration.hxx"

class MyServerAuthManager : public resip::ServerAuthManager {

private:
  resip::DialogUsageManager& dum;
  BasicConfiguration& basicConfiguration;

public:
  MyServerAuthManager(resip::DialogUsageManager& dum, BasicConfiguration& basicConfiguration);
  virtual ~MyServerAuthManager();

protected:
  resip::ServerAuthManager::AsyncBool requiresChallenge(const resip::SipMessage& msg);
  void requestCredential(const resip::Data& user, const resip::Data& realm, const resip::SipMessage& msg, const resip::Auth& auth, const resip::Data& transactionId);
  bool useAuthInt() const;

  bool authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, resip::Uri &fromUri);

  void onAuthSuccess(const resip::SipMessage& msg);
  void onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg);

};




#endif


