#include <memory>

#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/Timer.hxx"

#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/sipstack/Helper.hxx"
#include "Resolver.hxx"

#include "Register.hxx"
#include "Transceiver.hxx"

using namespace Vocal2;
using namespace Loadgen;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

Register::Register(Transceiver& transceiver, const Vocal2::Uri& registrand, 
                   int firstExtension, int lastExtension, 
                   int numRegistrations)
   : mTransceiver(transceiver),
     mRegistrand(registrand),
     mFirstExtension(firstExtension),
     mLastExtension(lastExtension),
     mNumRegistrations(numRegistrations)
{
   if (mNumRegistrations == 0)
   {
      mNumRegistrations = mLastExtension - mFirstExtension;
   }
}

void
Register::go()
{
   int numRegistered = 0;
   
   Resolver target(mRegistrand);
   NameAddr registrand;
   registrand.uri() = mRegistrand;
   NameAddr aor(registrand);
   
   NameAddr contact;
   contact.uri() = mTransceiver.contactUri();

   UInt64 startTime = Timer::getTimeMs();
   while (numRegistered < mNumRegistrations)
   {
      for (int i=mFirstExtension; i < mLastExtension && numRegistered < mNumRegistrations; i++)
      {
         aor.uri().user() = Data(i);
         contact.uri().user() = Data(i);
         
         auto_ptr<SipMessage> registration(Helper::makeRegister(registrand, aor, contact) );
         //registration->header(h_Contacts).push_back(contact);
         
         mTransceiver.send(target, *registration);
         
         SipMessage* reg = mTransceiver.receive(2000);
         if(reg)
         {         
            auto_ptr<SipMessage> forDel(reg);
            //validate here
            numRegistered++;
         }
         else
         {
            ErrLog(<< "Registrar not responding.");
            assert(0);
         }
      }
   }
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << mNumRegistrations << " peformed in " << elapsed << " ms, a rate of " 
        << mNumRegistrations / ((float) elapsed / 1000.0) << " registrations per second." << endl;
}
