#ifndef Registrar_hxx
#define Registrar_hxx

#include "resiprocate/sipstack/Uri.hxx"

namespace Loadgen
{

class Transceiver;


class Registrar
{
   public:
      Registrar(Transceiver& transceiver);
      void go();
   private:
      Transceiver& mTransceiver;
};



}



#endif

