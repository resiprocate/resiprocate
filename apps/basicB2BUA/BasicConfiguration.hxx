
#ifndef __SKYCONFIGURATION_H
#define __SKYCONFIGURATION_H

#include <arpa/inet.h>
#include <map>
#include <string>
#include <vector>

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

#include <rutil/Data.hxx>

#include "Context.hxx"
#include "Peer.hxx"

XERCES_CPP_NAMESPACE_USE

class BasicConfiguration : public ErrorHandler {

public:
   BasicConfiguration(const char* filename);
   BasicConfiguration(const std::string& filename);
   ~BasicConfiguration();

   // (re)loads the configuration data from the source
   void load();


   void setReloadRequired(bool reloadRequired) { mReloadRequired = reloadRequired; };

   Peer *getPeerByName(const resip::Data& peerName);
   Peer *getPeerByIp(const struct in_addr& ipAddress);
  
   Context *getContextByName(const resip::Data& contextName);


   void warning(const SAXParseException& toCatch);
   void error(const SAXParseException& toCatch);
   void fatalError(const SAXParseException& toCatch);
   void resetErrors();


private:

   void init();

   void checkReload();

   bool mReloadRequired;

   void handlePeers(DOMNode *peersNode);
   void handleContexts(DOMNode *contextsNode);

   std::string mFilename;

   std::map<resip::Data, Peer *> peersByName;
   //std::map<int, Peer *) peersByIp;
   std::map<resip::Data, Context *> contexts;


};


#endif

