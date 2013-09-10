
#include "Peer.hxx"
#include "XMLHelpers.hxx"

Peer::Peer(DOMNode *node) {
   DOMNode *optionNode = node->getFirstChild();
   while(optionNode != NULL) {
      if(optionNode->getNodeType() == DOMNode::ELEMENT_NODE) {
	 if(strcmp(StrX(optionNode->getNodeName()).localForm(), "name") == 0) {
	    mName = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "ip") == 0) {
            DOMCharacterData *charData = (DOMCharacterData *)optionNode->getFirstChild();
            if(inet_aton(StrX(charData->getData()).localForm(), &mIp) == 0) 
               throw;
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "password") == 0) {
	    mPassword = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
	 } else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "context") == 0) {
	    mContextName = NodeToData((DOMCharacterData *)optionNode->getFirstChild());
	 }
      }
      optionNode = optionNode->getNextSibling();
   }
}

Peer::~Peer() {
}

