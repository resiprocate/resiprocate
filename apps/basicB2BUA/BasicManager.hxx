
#ifndef __BasicCallController_h
#define __BasicCallController_h


#include <iostream>
#include <list>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

#include "b2bua/AuthenticationManager.hxx"
#include "b2bua/AuthorizationManager.hxx"
#include "b2bua/CallHandle.hxx"


#include "BasicConfiguration.hxx"

class BasicManager;

class BasicCallRoute : public b2bua::CallRoute {

private:
  resip::Data authRealm;
  resip::Data authUser;
  resip::Data authPass;
  resip::NameAddr sourceAddr;
  resip::NameAddr destinationAddr;
  resip::Uri outboundProxy;
  bool srvQuery;
  resip::Data appRef1;
  resip::Data appRef2;

public:
  BasicCallRoute(const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPass, const resip::NameAddr& sourceAddr, const resip::NameAddr& destinationAddr, const resip::Uri& outboundProxy, bool srvQuery);
  virtual ~BasicCallRoute();
  const resip::Data& getAuthRealm() { return authRealm; }
  const resip::Data& getAuthUser() { return authUser; }
  const resip::Data& getAuthPass() { return authPass; }
  virtual const resip::NameAddr& getSourceAddr() { return sourceAddr; }
  const resip::NameAddr& getDestinationAddr() { return destinationAddr; }
  const resip::Uri& getOutboundProxy() { return outboundProxy; }
  const bool isSrvQuery() { return srvQuery; }
  virtual const resip::Data& getAppRef1() { return appRef1; };
  virtual const resip::Data& getAppRef2() { return appRef2; };
};

class BasicCallHandle : public b2bua::CallHandle {
private:
  static int idseq;
  int authResult;
  resip::Data realm;
  time_t maxDuration;			// The maximum length for a call,
					// only checked at connection time
					// when setting the initial value
					// of hangupTime.  -1 for no limit
  time_t hangupTime;			// Absolute time to stop the call,
					// 0 means unlimited
  std::list<b2bua::CallRoute *> routes;
  BasicManager *basicManager;
public:
  BasicCallHandle();
  BasicCallHandle(int authResult);
  BasicCallHandle(int authResult, resip::Data& realm);
  BasicCallHandle(int authResult, time_t maxDuration, std::list<b2bua::CallRoute *>& routes);
  virtual ~BasicCallHandle();
  void setBasicManager(BasicManager *basicManager) { this->basicManager = basicManager; }
  int getAuthResult() { return authResult; }
  void setAuthResult(int authResult);
  const resip::Data& getRealm() { return realm; }
  void setMaxDuration(time_t maxDuration);
  bool mustHangup();
  time_t getHangupTime() { return hangupTime; }
  void setHangupTime(const time_t& hangupTime) { this->hangupTime = hangupTime; }
  void connect(time_t *connectTime);
  void fail(time_t *finishTime);
  void finish(time_t *finishTime);
  std::list<b2bua::CallRoute *>& getRoutes() { return routes; }
};

class BasicManager : public b2bua::AuthorizationManager {

private:
//  const resip::Data sipDomain;
//  const resip::Data authRealm;
//  const resip::Data authUser;
//  const resip::Data authPass;
//  const resip::Uri fromUri;
//  const resip::Data dialPrefix;
//  int maxDuration;
  std::list<BasicCallHandle *> handles;
  BasicConfiguration& mBasicConfiguration;

public:

  BasicManager(BasicConfiguration& basicConfiguration);
  virtual ~BasicManager();

  void addCallHandle(BasicCallHandle *handle);
  void removeCallHandle(BasicCallHandle *handle);

  b2bua::CallHandle *authorizeCall(const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPass, const resip::Data& srcIp, const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, const resip::Data& controlId, time_t startTime);

};





#endif

