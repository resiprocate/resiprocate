
#ifndef __CONTEXTRULE_H
#define __CONTEXTRULE_H

#include <list>

#include <xercesc/dom/DOMNode.hpp>

#include <pcrecpp.h>

#include <rutil/Data.hxx>

XERCES_CPP_NAMESPACE_USE

class ContextRule {


public:
  ContextRule(DOMNode *node);
  ~ContextRule();

  const resip::Data& getPattern() { return mPattern; };

  bool match(const resip::Data& dialed);
  resip::Data translate(const resip::Data& dialed);

  const std::list<resip::Data>& getPeerNames() { return mPeerNames; };

protected:
  resip::Data mPattern;
  pcrecpp::RE *mPatternRE;
  unsigned int mStripDigits;
  resip::Data mPrefixDigits;
  std::list<resip::Data> mPeerNames;

};



#endif

