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

      class Exception : public Vocal2::BaseException
      {
         public:
            Exception(const Vocal2::Data& msg,
                      const Vocal2::Data& file,
                      const int line) 
               : Vocal2::BaseException(msg, file, line) 
            {}

            ~Exception() throw() {}
            
            virtual const char* name() const 
            { 
               return "InviteServer::BaseException";
            }
      };
   private:
      Vocal2::SipMessage* waitForResponse(int responseCode,
                                          int waitMs);
      
      Vocal2::SipMessage* waitForRequest(Vocal2::MethodTypes method,
                                         int waitMs);
      Transceiver& mTransceiver;
};



}



#endif








