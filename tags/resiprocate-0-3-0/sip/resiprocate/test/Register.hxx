#ifndef Register_hxx
#define Register_hxx

#include "resiprocate/Uri.hxx"

namespace Loadgen
{

class Transceiver;


class Register
{
   public:
      Register(Transceiver& tranceiver, const resip::Uri& registrand, 
               int firstExtension, int lastExtension, 
               int numRegistrations = 0);
      
      void go();
   private:
      Transceiver& mTransceiver;
      resip::Uri mRegistrand;
      int mFirstExtension;
      int mLastExtension;
      int mNumRegistrations;
};



}



#endif

