
#include <arpa/inet.h>
#include <pcre.h>

#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"

#include "b2bua/Logging.hxx"

#include "Context.hxx"
#include "ContextRule.hxx"
#include "Peer.hxx"
#include "BasicManager.hxx"

// CLI value to send when CLI restricted/not available
#define CLI_ANONYMOUS "0"

using namespace b2bua;
using namespace resip;
using namespace std;


//
// BasicCallRoute
//

BasicCallRoute::BasicCallRoute(const Data& authRealm, const Data& authUser, const Data& authPass, const NameAddr& sourceAddr, const NameAddr& destinationAddr, const Uri& outboundProxy, bool srvQuery) : authRealm(authRealm), authUser(authUser), authPass(authPass), sourceAddr(sourceAddr), destinationAddr(destinationAddr), outboundProxy(outboundProxy), srvQuery(srvQuery) {
}

BasicCallRoute::~BasicCallRoute() {
}

int BasicCallHandle::idseq = 0;

BasicCallHandle::BasicCallHandle() : realm("") {
  authResult = CC_PENDING;
  maxDuration = -1;
  hangupTime = 0;
}

BasicCallHandle::BasicCallHandle(int authResult) : realm("") {
  this->authResult = authResult;
  maxDuration = -1;
  hangupTime = 0;
}

BasicCallHandle::BasicCallHandle(int authResult, resip::Data& realm) : realm(realm) {
  this->authResult = authResult; 
  maxDuration = -1;
  hangupTime = 0;
}

BasicCallHandle::BasicCallHandle(int authResult, time_t maxDuration, std::list<CallRoute *>& routes) : realm(""), routes(routes) {
  this->authResult = authResult;
  this->maxDuration = maxDuration;
  this->hangupTime = 0;
}

BasicCallHandle::~BasicCallHandle() {
  list<CallRoute *>::iterator i = routes.begin();
  while(i != routes.end()) {
    CallRoute *callRoute = *i;
    delete callRoute;
    i++;
  }
}

void BasicCallHandle::setAuthResult(int authResult) {
  this->authResult = authResult;
}

void BasicCallHandle::setMaxDuration(time_t maxDuration) {
  this->maxDuration = maxDuration;
}

bool BasicCallHandle::mustHangup() {
  if(hangupTime == 0)
    return false;
  time_t now;
  time(&now);
  if(hangupTime <= now)  {
    B2BUA_LOG_DEBUG("hangupTime = %ld, now = %ld", hangupTime, now);
    return true;
  }
  return false;
}

void BasicCallHandle::connect(time_t *connectTime) {
  B2BUA_LOG_DEBUG("call connect");
  if(maxDuration != -1) {
    hangupTime = (*connectTime + maxDuration);
  }
}

void BasicCallHandle::fail(time_t *finishTime) {
  B2BUA_LOG_DEBUG("call failed");
}

void BasicCallHandle::finish(time_t *finishTime) {
  B2BUA_LOG_DEBUG("call finished successfully");
}

BasicManager::BasicManager(BasicConfiguration& basicConfiguration) : mBasicConfiguration(basicConfiguration) {
}

BasicManager::~BasicManager() {
}

void BasicManager::addCallHandle(BasicCallHandle *callHandle) {
  handles.push_back(callHandle);
}

void BasicManager::removeCallHandle(BasicCallHandle *callHandle) {
  handles.remove(callHandle);
}

inline const char *getCli(const NameAddr& sourceAddr) {
  // CLI setup
  // If the `display name' portion of the From: header contains an
  // E.164 number of the form +[1-9][0-9]*, length > 3 and <= 16, use
  // that as CLI.  Otherwise, use pres
  const char *cli = NULL;
  if(!sourceAddr.displayName().empty()) {
    if(sourceAddr.displayName().size() > 4 && sourceAddr.displayName().size() <= 16) {
      pcre *re;
      const char *error;
      int erroffset;
      re = pcre_compile("^\\+[0-9]*$", 0, &error, &erroffset, NULL);
      if(re != NULL) {
        int rc;
        int ovector[30];
        rc = pcre_exec(re, NULL, sourceAddr.displayName().c_str(), sourceAddr.displayName().size(), 0, 0, ovector, 30);
        if(rc == 1)
          cli = sourceAddr.displayName().c_str() + 1;
        pcre_free(re);
      }
    }
  }
  return cli;
}

CallHandle *BasicManager::authorizeCall(const NameAddr& sourceAddr, const Uri& destinationAddr, const Data& authRealm, const Data& authUser, const Data& authPass, const Data& srcIp, const Data& contextId, const Data& accountId, const Data& baseIp, const Data& controlId, time_t startTime) {

  Peer *peer = NULL;

  // First try and find a peer with matching IP
  struct in_addr ip_address;
  inet_aton(srcIp.c_str(), &ip_address);
  peer = mBasicConfiguration.getPeerByIp(ip_address);

  // If IP not matched, try the username
  if(peer == NULL) {
    peer = mBasicConfiguration.getPeerByName(authUser);
  }

  // If not a valid peer, reject the call
  if(peer == NULL) {
    B2BUA_LOG_WARNING("peer not recognised");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }

  // If no context, reject the call
  Context *context = mBasicConfiguration.getContextByName(peer->getContextName());
  if(context == NULL) {
    B2BUA_LOG_WARNING("context not found");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }

  // Find the rule that matches the destination number
  ContextRule *rule = context->getRule(destinationAddr.user());
  if(rule == NULL) {
    B2BUA_LOG_WARNING("rule not found in context");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }

  Data translatedDestination = rule->translate(destinationAddr.user());
  if(translatedDestination.size() == 0) {
    B2BUA_LOG_WARNING("failed to translate number");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }


  const std::list<resip::Data>& destPeerNames = rule->getPeerNames();
  if(destPeerNames.size() < 1) {
    B2BUA_LOG_WARNING("destination peer not specified");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }

  Peer *destPeer = mBasicConfiguration.getPeerByName(destPeerNames.front());
  if(destPeer == NULL) {
    B2BUA_LOG_WARNING("destination peer non-existant");
    BasicCallHandle *callHandle = new BasicCallHandle(CC_INVALID);
    callHandle->setBasicManager(this);
    addCallHandle(callHandle);
    return callHandle;
  }

  B2BUA_LOG_DEBUG("call permitted");
  BasicCallHandle *callHandle = new BasicCallHandle(CC_PERMITTED); 
  Data cliNum;
  if(sourceAddr.uri().user().empty())
    cliNum = Data(CLI_ANONYMOUS);
  else {
    cliNum = Data(sourceAddr.uri().user());
    char &c = cliNum.at(0);
    if(c >= '1' && c <= '9') {
      cliNum = Data("44") + Data(sourceAddr.uri().user());
    } 
  }
  Uri mySourceUri("sip:" + cliNum + "@myb2bua");
  NameAddr sourceAddrT(mySourceUri);
  Uri myDestUri("sip:" + translatedDestination + "@" + inet_ntoa(destPeer->getIp()));
  NameAddr destinationAddrT(myDestUri);
  Uri outboundProxy;
  bool srvQuery = false; // FIXME - implement and test SRV
  Data sipRealm(inet_ntoa(destPeer->getIp()));
  Data sipUser(destPeer->getName());
  Data sipPassword(destPeer->getPassword());
  BasicCallRoute *route = new BasicCallRoute(sipRealm, sipUser, sipPassword, sourceAddrT, destinationAddrT, outboundProxy, srvQuery);
  callHandle->getRoutes().push_back(route);
  callHandle->setBasicManager(this);
  addCallHandle(callHandle);
  return callHandle;
}


