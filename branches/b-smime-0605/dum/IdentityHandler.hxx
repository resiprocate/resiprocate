#ifndef RESIP_IdentityHandler
#define RESIP_IdentityHandler

#include "DumFeature.hxx"
#include <map>

class HttpGetMessage;

class IdentityHandler : public DumFeature
{
   public:      
      IdentityHandler(DialogUsageManager& dum); 
      virtual ProcessingResult process(Message* msg);
   private:
      void processIdentityCheckResponse(const HttpGetMessage& msg);      
      bool queueForIdentityCheck(SipMessage* sipMsg);

      typedef std::map<Data, SipMessage*> RequiresCerts;
      RequiresCerts mRequiresCerts;      
};


#endif

   
