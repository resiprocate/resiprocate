

#include "resip/stack/SipMessage.hxx"

#include "resip/dum/ServerRegistration.hxx"

#include "DummyServerRegistrationHandler.hxx"
#include "Logging.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

void DummyServerRegistrationHandler::onRefresh(ServerRegistrationHandle sr, const SipMessage& reg) {
  sr->accept(200);
}

void DummyServerRegistrationHandler::onRemove(ServerRegistrationHandle sr, const SipMessage& reg) {
  sr->accept(200);
}

void DummyServerRegistrationHandler::onRemoveAll(ServerRegistrationHandle sr, const SipMessage& reg) {
  sr->accept(200);
}

void DummyServerRegistrationHandler::onAdd(ServerRegistrationHandle sr, const SipMessage& reg) {
  B2BUA_LOG_INFO("client trying to register, username=%s", reg.header(h_From).uri().user().c_str());
  sr->accept(200);
}

void DummyServerRegistrationHandler::onQuery(ServerRegistrationHandle sr, const SipMessage& reg) {
  sr->accept(200);
}

