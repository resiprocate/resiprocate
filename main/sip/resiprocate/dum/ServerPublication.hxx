#if !defined(RESIP_SERVERPUBLICATION_HXX)
#define RESIP_SERVERPUBLICATION_HXX

namespace resip
{

/** @file ServerPublication.hxx
 *   @todo This file is empty
 */

class ServerPublication : public BaseUsage 
{
   public:

      class Handle
      {
      };  
      
      /** An application uses sendResponse to reply to
       *  the stimulus received through 
       *  PublicationHandler::onPublication.
       *  Note that the DUM will have to correlate the
       *  provide msg (a REGISTER response) to the appropriate
       *  transaction.
       */
      void sendResponse(SipMessage& msg);
};

}

#endif
