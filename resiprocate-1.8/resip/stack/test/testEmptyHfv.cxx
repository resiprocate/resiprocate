#include "resip/stack/SipMessage.hxx"
#include "resip/stack/HeaderTypes.hxx"

#include "rutil/Log.hxx"

#include <iostream>

using namespace std;
using namespace resip;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


int main()
{
   Log::initialize(Log::Cout, Log::Debug ,"testEmptyHfv");   
   resip::Data msg("INVITE sip:foo@bar SIP/2.0\r\n"
                     "Via:\r\n"
                     "From:\r\n"
                     "To:\r\n"
                     "CSeq:\r\n"
                     "Call-ID:\r\n"
                     "Max-Forwards:\r\n"
                   "Content-Length:\r\n\r\n\r\n");
                     
   resip::SipMessage* sip=resip::SipMessage::make(msg);
   
   assert(sip);
   
   try
   {
      if(sip->exists(resip::h_From))
      {
         sip->header(resip::h_From);
         cerr << sip->header(resip::h_From).uri();         
         cerr << "test failed" << endl;
         
         assert(0);         
         return -1;         
      }
   }
   catch(resip::BaseException& e)
   {
      cerr << "Caught: " << e << "as expected, PASSED" << endl;
   }

}
