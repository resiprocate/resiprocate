#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <sstream>

#include "resip/dum/ChallengeInfo.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "resip/dum/RADIUSServerAuthManager.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"

#ifdef USE_RADIUS_CLIENT

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

RADIUSServerAuthManager::RADIUSServerAuthManager(resip::DialogUsageManager& dum) : ServerAuthManager(dum, dum.dumIncomingTarget()), dum(dum) {

  RADIUSDigestAuthenticator::init(NULL);
}

RADIUSServerAuthManager::~RADIUSServerAuthManager() {
}

ServerAuthManager::AsyncBool RADIUSServerAuthManager::requiresChallenge(const SipMessage& msg) {

  ostringstream s;
  s << msg.header(h_RequestLine).uri(); 
  DebugLog(<<"RADIUSServerAuthManager::requiresChallenge, uri = " << s.str().c_str());

  // default behaviour is to challenge
  ChallengeInfo *cmsg = new ChallengeInfo(false, true, msg.getTransactionId());
  dum.post(cmsg);
  return Async;
}

void RADIUSServerAuthManager::requestCredential(const resip::Data& user, const resip::Data& realm, const resip::SipMessage& msg, const resip::Auth& auth, const resip::Data& transactionId) {

  ostringstream s;
  s << msg.header(h_RequestLine).uri();
  DebugLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << s << " authUser = " << user);

  MyRADIUSDigestAuthListener *radiusListener = NULL;
  try {

    radiusListener = new MyRADIUSDigestAuthListener(user, realm, dum, transactionId);
    Data radiusUser(user + "@" + realm);
    DebugLog(<< "radiusUser = " << radiusUser.c_str() << ", " << "user = " << user.c_str());
    Data reqUri("");
    Data reqMethod("");
    if(msg.isRequest()) {
      //reqUri = Data(msg.header(h_RequestLine).uri());
  //    ostringstream s;
  //    s << msg.header(h_RequestLine).uri();
      //reqUri = Data(s.str());
      reqUri = auth.param(p_uri);
      reqMethod = Data(resip::getMethodName(msg.header(h_RequestLine).getMethod()));
    }
    RADIUSDigestAuthenticator *radius = NULL;
    if(auth.exists(p_qop)) {
      if(auth.param(p_qop) == Symbols::auth) {
        Data myQop("auth");
        radius = new RADIUSDigestAuthenticator(radiusUser, user, realm, auth.param(p_nonce), reqUri, reqMethod, myQop, auth.param(p_nc), auth.param(p_cnonce), auth.param(p_response), radiusListener);     
      } else if(auth.param(p_qop) == Symbols::authInt) {
        Data myQop("auth-int");
        radius = new RADIUSDigestAuthenticator(radiusUser, user, realm, auth.param(p_nonce), reqUri, reqMethod, myQop, auth.param(p_nc), auth.param(p_cnonce), auth.param(p_opaque), auth.param(p_response), radiusListener); 
      }
    }
    if(radius == NULL) {
      radius = new RADIUSDigestAuthenticator(radiusUser, user, realm, auth.param(p_nonce), reqUri, reqMethod, auth.param(p_response), radiusListener);
    }
    int result = radius->doRADIUSCheck(); 
    if(result < 0) {
      ErrLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << s <<" failed to start thread, error = " << result);
    }
  } catch(...) {
    WarningLog(<<"RADIUSServerAuthManager::requestCredential, uri = " << s <<" exception");
    delete radiusListener;
  }
}

bool RADIUSServerAuthManager::useAuthInt() const {
  return true;
}

bool RADIUSServerAuthManager::authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, resip::Uri &fromUri) {
  // Always returns true: in other words, any user can send any from URI
  // or forge any CLI
  return true;
}

void
RADIUSServerAuthManager::onAuthSuccess(const resip::SipMessage& msg) {
}

void 
RADIUSServerAuthManager::onAuthFailure(resip::ServerAuthManager::AuthFailureReason reason, const resip::SipMessage& msg) {
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
  WarningLog(<<"auth failure: " << failureMsg << ": src IP=" << sourceIp << ", uri=" << msg.header(h_RequestLine).uri().user() << ", from=" <<msg.header(h_From).uri().user() << ", to=" << msg.header(h_To).uri().user());
}

MyRADIUSDigestAuthListener::MyRADIUSDigestAuthListener(const resip::Data& user, const resip::Data& realm, resip::TransactionUser& tu, const resip::Data& transactionId) : user(user), realm(realm), tu(tu), transactionId(transactionId) {
}

MyRADIUSDigestAuthListener::~MyRADIUSDigestAuthListener() {
}

void MyRADIUSDigestAuthListener::onSuccess(const resip::Data& rpid) {
  DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess");
  if(!rpid.empty())
    DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess rpid = " << rpid.c_str());
  else
    DebugLog(<<"MyRADIUSDigestAuthListener::onSuccess, no rpid");
  UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::DigestAccepted, transactionId);
  tu.post(uai);
}

void MyRADIUSDigestAuthListener::onAccessDenied() {
  DebugLog(<<"MyRADIUSDigestAuthListener::onAccessDenied");
  UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::DigestNotAccepted, transactionId);
  tu.post(uai);
}

void MyRADIUSDigestAuthListener::onError() {
  WarningLog(<<"MyRADIUSDigestAuthListener::onError");
  UserAuthInfo *uai = new UserAuthInfo(user, realm, UserAuthInfo::Error, transactionId);
  tu.post(uai);
}

#endif

