#ifndef Register_hxx
#define Register_hxx

#include "resiprocate/sipstack/Uri.hxx"

namespace Loadgen
{

class Transceiver;


class Register
{
   public:
      Register(Transceiver& tranceiver, const Vocal2::Uri& registrand, 
               int firstExtension, int lastExtension, 
               int numRegistrations = 0);
      
      void go();
   private:
      Transceiver& mTransceiver;
      Vocal2::Uri mRegistrand;
      int mFirstExtension;
      int mLastExtension;
      int mNumRegistrations;
};



}



#endif

