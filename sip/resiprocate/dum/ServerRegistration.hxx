#if !defined(RESIP_SERVERREGISTRATION_HXX)
#define RESIP_SERVERREGISTRATION_HXX

/** @file ServerRegistration.hxx
 *   
 */

namespace resip
{

class ServerRegistration: public BaseUsage 
{
   /** An application uses sendResponse to reply to
    *  the stimulus received through 
    *  RegistrationHandler::onRegister.
    *  Note that the DUM will have to correlate the
    *  provide msg (a REGISTER response) to the appropriate
    *  transaction.
    */
   void sendResponse(SipMessage& msg);
};
 
}

#endif
