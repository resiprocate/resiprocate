#ifndef InviteClient_hxx
#define InviteClient_hxx

#include "resiprocate/util/BaseException.hxx"
#include "resiprocate/sipstack/Uri.hxx"

namespace Loadgen
{

class Transceiver;


class InviteClient
{
   public:
      InviteClient(Transceiver& tranceiver, const Vocal2::Uri& proxy, 
                   int firstExtension, int lastExtension, 
                   int numInvites = 0);
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
               return "InviteClient::BaseException";
            }
      };
   private:
      Vocal2::SipMessage* waitForResponse(int responseCode,
                                          int waitMs);
      Transceiver& mTransceiver;
      Vocal2::Uri mProxy;
      int mFirstExtension;
      int mLastExtension;
      int mNumInvites;
};



}



#endif








