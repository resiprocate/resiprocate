#ifndef InviteClient_hxx
#define InviteClient_hxx

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/Uri.hxx"

namespace Loadgen
{

class Transceiver;


class InviteClient
{
   public:
      InviteClient(Transceiver& tranceiver, const resip::Uri& proxy, 
                   int firstExtension, int lastExtension, 
                   int numInvites = 0);
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
               return "InviteClient::BaseException";
            }
      };
   private:
      resip::SipMessage* waitForResponse(int responseCode,
                                          int waitMs);
      Transceiver& mTransceiver;
      resip::Uri mProxy;
      int mFirstExtension;
      int mLastExtension;
      int mNumInvites;
};



}



#endif








