

#include <sstream>

#include "resip/dum/ChallengeInfo.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "rutil/Data.hxx"
#include "rutil/MD5Stream.hxx"

#include "b2bua/Logging.hxx"

#include "MyServerAuthManager.hxx"

using namespace resip;
using namespace std;

MyServerAuthManager::MyServerAuthManager(resip::DialogUsageManager& dum, BasicConfiguration& basicConfiguration) : ServerAuthManager(dum, dum.dumIncomingTarget()), dum(dum), basicConfiguration(basicConfiguration) {

}

MyServerAuthManager::~MyServerAuthManager() {
}

ServerAuthManager::AsyncBool MyServerAuthManager::requiresChallenge(const SipMessage& msg) {

  ostringstream s;
  s << msg.header(h_RequestLine).uri(); 
  B2BUA_LOG_DEBUG("MyServerAuthManager::requiresChallenge, uri = %s", s.str().c_str());

  // Allow OPTIONS requests unchallenged
  if(msg.header(h_RequestLine).method() == OPTIONS)
    return False;

  // Allow locally initiated dialogs
  DialogId dialogId(msg);
  InviteSessionHandle h = mDum.findInviteSession(dialogId);
  if(h.isValid()) {
    // Session already exists, challenge may not be required
    ClientInviteSession *cis = dynamic_cast<ClientInviteSession *>(h.get());
    if(cis != NULL) {
      // Type is ClientInviteSession, a locally originated connection
      // therefore, we won't challenge
      B2BUA_LOG_DEBUG("MyServerAuthManager::requiresChallenge, uri = %s is locally initiated", s.str().c_str());
      return False;
    }
  }

  // Allow peers without passwords
  Tuple sourceTuple = msg.getSource();
  const struct in_addr& src_addr = sourceTuple.toGenericIPAddress().v4Address.sin_addr;
  Peer *peer = basicConfiguration.getPeerByIp(src_addr);
  if(peer != NULL) {
    B2BUA_LOG_DEBUG("found IP, checking for password");
    if(peer->getPassword().size() == 0) {
      B2BUA_LOG_DEBUG("password not required");
      return False;    
    }
  }

  // default behaviour is to challenge
  //ChallengeInfo *cmsg = new ChallengeInfo(false, true, msg.getTransactionId());
  //dum.post(cmsg);
  //return Async;
  B2BUA_LOG_DEBUG("password required");
  return True;
}

void MyServerAuthManager::requestCredential(const resip::Data& user, const resip::Data& realm, const resip::SipMessage& msg, const resip::Auth& auth, const resip::Data& transactionId) {

  Peer *peer = basicConfiguration.getPeerByName(user); 
  if(peer == NULL) {
    UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::DigestNotAccepted, transactionId);
    dum.post(uai);
    return;
  }

  MD5Stream a1;
  a1 << user << Symbols::COLON << realm << Symbols::COLON << peer->getPassword();
  a1.flush();
  UserAuthInfo *uai = new UserAuthInfo(user, realm, a1.getHex(), transactionId);
  dum.post(uai);

}

bool MyServerAuthManager::useAuthInt() const {
  return true;
}

bool MyServerAuthManager::authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, resip::Uri &fromUri) {
  // Always returns true: in other words, any user can send any from URI
  // or forge any CLI
  return true;
}

void
MyServerAuthManager::onAuthSuccess(const resip::SipMessage& msg) {
}

void 
MyServerAuthManager::onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg) {
  Data failureMsg("unknown failure");
  switch(reason) {
  case InvalidRequest: 
    failureMsg = Data("InvalidRequest");
    break;
  case BadCredentials:
    failureMsg = Data("BadCredentials");
    break;
  case Error:
    failureMsg = Data("Error");
    break;
  }
  Tuple sourceTuple = msg.getSource();
  Data sourceIp = Data(inet_ntoa(sourceTuple.toGenericIPAddress().v4Address.sin_addr));
  B2BUA_LOG_WARNING("auth failure: %s: src IP=%s, uri=%s, from=%s, to=%s", failureMsg.c_str(), sourceIp.c_str(), msg.header(h_RequestLine).uri().user().c_str(), msg.header(h_From).uri().user().c_str(), msg.header(h_To).uri().user().c_str());
}


