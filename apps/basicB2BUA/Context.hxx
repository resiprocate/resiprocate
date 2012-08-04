
#ifndef __CONTEXT_H
#define __CONTEXT_H

#include <list>

#include <xercesc/dom/DOMNode.hpp>

#include <rutil/Data.hxx>
#include <rutil/GenericIPAddress.hxx>

#include "ContextRule.hxx"

XERCES_CPP_NAMESPACE_USE

class Context {


public:
  Context(DOMNode *node);
  ~Context();

  const resip::Data& getName() { return mName; };

  ContextRule *getRule(const resip::Data& dialed);

protected:
  resip::Data mName;
  std::list<ContextRule *> rules;

};


#endif


