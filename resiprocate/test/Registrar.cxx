#include <memory>

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"

#include "Registrar.hxx"
#include "Transceiver.hxx"



using namespace Vocal2;
using namespace Loadgen;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

Registrar::Registrar(Transceiver& transceiver)
   : mTransceiver(transceiver)
{}

void
Registrar::go()
{
   while(true)
   {
      SipMessage* reg = mTransceiver.receive(5);
      if(reg)
      {
         auto_ptr<SipMessage> forDel(reg);
         
         auto_ptr<SipMessage> response(Helper::makeResponse(*reg, 200));
         response->header(h_StatusLine).reason() = "OK";
         response->header(h_Contacts) = reg->header(h_Contacts);
         mTransceiver.send(*response);
      }
   }
}
