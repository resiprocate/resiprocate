#include "resip/stack/SipMessage.hxx"
#include "resip/stack/HeaderTypes.hxx"

int main()
{
   resip::Data msg("INVITE sip:foo@bar SIP/2.0\r\n"
                     "Via:\r\n"
                     "From:\r\n"
                     "To:\r\n"
                     "CSeq:\r\n"
                     "Call-ID:\r\n"
                     "Max-Forwards:\r\n"
                     "Content-Length:\r\n");
                     
   resip::SipMessage* sip=resip::SipMessage::make(msg);
   
   if(sip->exists(resip::h_From))
   {
      sip->header(resip::h_From);
   }



}