#if !defined(RESIP_SERVERREGISTRATION_HXX)
#define RESIP_SERVERREGISTRATION_HXX

namespace resip
{

class ServerRegistration: public BaseUsage 
{
   public:
      class Handle
      {
      };

      void accept(const SipMessage& ok);
      void reject(int statusCode);
};
 
}

#endif
