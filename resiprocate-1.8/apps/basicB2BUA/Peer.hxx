
#ifndef __PEER_H
#define __PEER_H

#include <arpa/inet.h>

#include <xercesc/dom/DOMNode.hpp>

#include <rutil/Data.hxx>
#include <rutil/GenericIPAddress.hxx>

XERCES_CPP_NAMESPACE_USE

class Peer {

public:
  Peer(DOMNode *node);
  ~Peer();

  const resip::Data& getName() { return mName; };
  const struct in_addr& getIp() { return mIp; };
  const resip::Data& getPassword() { return mPassword; };
  const resip::Data& getContextName() { return mContextName; };

protected:
  resip::Data mName;
  struct in_addr mIp;
  resip::Data mPassword;
  resip::Data mContextName;

};



#endif

