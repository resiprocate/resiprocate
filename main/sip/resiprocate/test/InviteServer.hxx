#ifndef InviteServer_hxx
#define InviteServer_hxx

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/SipMessage.hxx"

namespace Loadgen
{

class Transceiver;


class InviteServer
{
   public:
      InviteServer(Transceiver& tranceiver);
      void go();

      class Exception : public resip::BaseException
      {
         public:
            Exception(const resip::Data& msg,
                      const resip::Data& file,
                      const int line) 
               : resip::BaseException(msg, file, line) 
            {}

            ~Exception() throw() {}
            
            virtual const char* name() const 
            { 
               return "InviteServer::BaseException";
            }
      };
   private:
      resip::SipMessage* waitForResponse(int responseCode,
                                          int waitMs);
      
      resip::SipMessage* waitForRequest(resip::MethodTypes method,
                                         int waitMs);
      Transceiver& mTransceiver;
};



}



#endif








