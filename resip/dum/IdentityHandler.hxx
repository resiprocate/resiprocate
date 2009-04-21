#ifndef RESIP_IdentityHandler_HXX
#define RESIP_IdentityHandler_HXX

#include "resip/dum/DumFeature.hxx"
#include <map>

namespace resip
{

class HttpGetMessage;
class DumFeature;

class IdentityHandler : public DumFeature
{
   public:      
      IdentityHandler(DialogUsageManager& dum, TargetCommand::Target& target); 
      ~IdentityHandler();
      
      virtual ProcessingResult process(Message* msg);

   private:
      void processIdentityCheckResponse(const HttpGetMessage& msg);      
      bool queueForIdentityCheck(SipMessage* sipMsg);

      typedef std::map<Data, SipMessage*> RequiresCerts;
      RequiresCerts mRequiresCerts;
};

}


#endif

   
