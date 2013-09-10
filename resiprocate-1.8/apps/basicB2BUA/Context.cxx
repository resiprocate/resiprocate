
#include "Context.hxx"
#include "XMLHelpers.hxx"

XERCES_CPP_NAMESPACE_USE

Context::Context(DOMNode *node) {
   DOMNode *optionNode = node->getFirstChild();
   while(optionNode != NULL) {
      if(optionNode->getNodeType() == DOMNode::ELEMENT_NODE) {
         if(strcmp(StrX(optionNode->getNodeName()).localForm(), "name") == 0) {
            mName = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
         } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "rule") == 0) { 
            ContextRule *r = new ContextRule(optionNode);
            rules.push_back(r);
         }
      }
      optionNode = optionNode->getNextSibling();
   }
}

Context::~Context() {
   std::list<ContextRule *>::iterator i = rules.begin();
   while(i != rules.end()) {
      delete *i;
      i++;
   }
}

ContextRule *Context::getRule(const resip::Data& dialed) {
   std::list<ContextRule *>::iterator i = rules.begin();
   while(i != rules.end()) {
      ContextRule *r = *i;
      if(r->match(dialed))
         return r;
      i++;
   }
   return NULL;
}

