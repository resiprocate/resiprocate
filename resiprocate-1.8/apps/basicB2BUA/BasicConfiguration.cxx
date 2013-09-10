

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>


#include "BasicConfiguration.hxx"
#include "XMLHelpers.hxx"

XERCES_CPP_NAMESPACE_USE
using namespace resip;
using namespace std;

BasicConfiguration::BasicConfiguration(const char* filename) :
   mFilename(filename) {
   init();
}

BasicConfiguration::BasicConfiguration(const std::string& filename) :
   mFilename(filename) {
   init();
}

void BasicConfiguration::init() {
   // Initialize the XML4C2 system
   try {
      XMLPlatformUtils::Initialize();
   } catch (const XMLException& toCatch) {
      throw;
   }

   load();

}

BasicConfiguration::~BasicConfiguration() {
   XMLPlatformUtils::Terminate();
}

void BasicConfiguration::load() {

   XercesDOMParser* parser = new XercesDOMParser;
   //parser->setValidationScheme(???);
   parser->setDoNamespaces(false);

   int errorCount;
   try {
      parser->setErrorHandler(this);
      parser->parse(mFilename.c_str());
      //errorCount = parser->getErrorCount();
   } catch (const OutOfMemoryException&) {
      throw;
   } catch (const XMLException& toCatch) {
      throw;
   } catch (const DOMException& e) {
      throw;
   } catch(...) {
      throw;
   }
   DOMNode *doc = parser->getDocument();

   DOMNode *b2buaNode;
   if((b2buaNode = doc->getFirstChild()) == NULL)
      throw;
   if(b2buaNode->getNodeType() == DOMNode::PROCESSING_INSTRUCTION_NODE)
      b2buaNode = b2buaNode->getNextSibling();
   DOMNode *optionNode = b2buaNode->getFirstChild();
   while(optionNode != NULL) {
      if(optionNode->getNodeType() == DOMNode::ELEMENT_NODE) {
         if(strcmp(StrX(optionNode->getNodeName()).localForm(), "peers") == 0)
            handlePeers(optionNode);
         else if(strcmp(StrX(optionNode->getNodeName()).localForm(), "contexts") == 0)
            handleContexts(optionNode);
         else {
            cerr << StrX(optionNode->getNodeName()).localForm() << endl;
            throw;
         }
      } 
      optionNode = optionNode->getNextSibling();
   }
   delete parser;
}

void BasicConfiguration::handlePeers(DOMNode *peersNode) {
   DOMNode *peerNode = peersNode->getFirstChild();
   while(peerNode != NULL) {
      if(peerNode->getNodeType() == DOMNode::ELEMENT_NODE) {
         if(strcmp(StrX(peerNode->getNodeName()).localForm(), "peer") == 0) {
            Peer *peer = new Peer(peerNode);
            peersByName[peer->getName()] = peer;
            //peersByIp[peer->getIp()] = peer;
         } else {
            throw;
         }
      } 
      peerNode = peerNode->getNextSibling();
   }
}

void BasicConfiguration::handleContexts(DOMNode *contextsNode) {
   DOMNode *contextNode = contextsNode->getFirstChild();
   while(contextNode != NULL) {
      if(contextNode->getNodeType() == DOMNode::ELEMENT_NODE) {
         if(strcmp(StrX(contextNode->getNodeName()).localForm(), "context") == 0) {
            Context *context = new Context(contextNode);
            contexts[context->getName()] = context;
         } else {
            throw;
         }
      } 
      contextNode = contextNode->getNextSibling();
   }
}

void BasicConfiguration::warning(const SAXParseException& toCatch) {
}

void BasicConfiguration::error(const SAXParseException& toCatch) {
}

void BasicConfiguration::fatalError(const SAXParseException& toCatch) {
}

void BasicConfiguration::resetErrors() {
}


Peer *BasicConfiguration::getPeerByName(const resip::Data& peerName) {
   checkReload();
   std::map<resip::Data, Peer *>::iterator i = peersByName.find(peerName);
   if(i == peersByName.end())
      return NULL;
   return (*i).second;
}

Peer *BasicConfiguration::getPeerByIp(const struct in_addr& ipAddress) {
   checkReload();
   std::map<resip::Data, Peer *>::iterator i = peersByName.begin();
   while(i != peersByName.end()) {
      if(memcmp(&((*i).second->getIp()), &ipAddress, sizeof(struct in_addr)) == 0)
         return (*i).second;
      i++;
   }
   return NULL;
}

Context *BasicConfiguration::getContextByName(const resip::Data& contextName) {
   checkReload();
   std::map<resip::Data, Context *>::iterator i = contexts.find(contextName);
   if(i == contexts.end())
      return NULL;
   return (*i).second;
}

void BasicConfiguration::checkReload() {
   if(!mReloadRequired)
      return;
   load();
   mReloadRequired = false;
}


int old_main(int argc, char *argv[]) {

  string filename("doc/config.xml");

  BasicConfiguration *s = new BasicConfiguration(filename);

  cerr << s->getPeerByName("testM")->getContextName() << endl;

  Context *ctx = s->getContextByName(Data("ctx_excel"));
  cerr << ctx->getName() << endl;
  ContextRule *r = ctx->getRule(Data(argv[1]));
  if(r == NULL)
    cerr << "no match" << endl;
  else 
    cerr << "match: " << r->getPattern() << endl;

}


