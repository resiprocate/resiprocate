#if !defined(RESIP_REGISTRATIONHANDLER_HXX)
#define RESIP_REGISTRATIONHANDLER_HXX

namespace resip
{

class RegistrationHandler
{
   /** onRegister is invoked when the DUM receives a register request. The
    *  response to that request will be handed to the ServerRegistration
    *  asynchronously.
    */
   void onRegister(ServerRegistration& serverRegistration, SipMessage& msg);
};

}

#endif
