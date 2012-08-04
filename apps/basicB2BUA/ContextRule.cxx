
#include <pcrecpp.h>

#include "ContextRule.hxx"
#include "XMLHelpers.hxx"

ContextRule::ContextRule(DOMNode *node) : mPatternRE(0) {
   DOMNode *optionNode = node->getFirstChild();
   while(optionNode != NULL) {
      if(optionNode->getNodeType() == DOMNode::ELEMENT_NODE) {
	 if(strcmp(StrX(optionNode->getNodeName()).localForm(), "pattern") == 0) {
	    mPattern = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
            mPatternRE = new pcrecpp::RE(mPattern.c_str());
            std::string err = mPatternRE->error();
            if(err != "") {
               std::cerr << err << std::endl;
               throw;
            }
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "strip") == 0) {
	    mStripDigits  = atoi(NodeToData((DOMCharacterData *)optionNode->getFirstChild()).c_str());
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "insert") == 0) {
	    mPrefixDigits = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "peer") == 0) {
	    mPeerNames.push_back(NodeToData((DOMCharacterData *)optionNode->getFirstChild()));
	 }
      }
      optionNode = optionNode->getNextSibling();
   }
}

ContextRule::~ContextRule() {
   delete mPatternRE;
}

bool ContextRule::match(const resip::Data& dialed) {
   return mPatternRE->PartialMatch(dialed.c_str());
}

resip::Data ContextRule::translate(const resip::Data& dialed) {

   if(dialed.size() <= mStripDigits)
      return NULL;

   return resip::Data(mPrefixDigits + dialed.substr(mStripDigits, dialed.size() - mStripDigits));

}

