#include "rutil/Logger.hxx"
#include "resip/stack/SipMessage.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/SipEvent.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;
using namespace std;


CheckFetchedContacts::CheckFetchedContacts(std::set<resip::NameAddr> contacts)
   : _contacts(contacts)
{
}

bool 
CheckFetchedContacts::compareContacts(const NameAddr & s1, const NameAddr & s2)
{
   float q1 = (float)(s1.exists(p_q) ? s1.param(p_q) : 1.0);
   float q2 = (float)(s2.exists(p_q) ? s2.param(p_q) : 1.0);
   return (s1.uri().getAor() == s2.uri().getAor() && q1 == q2);
}      

void 
CheckFetchedContacts::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();

   set<NameAddr> msgContacts;
   
   if ((unsigned int)msg->header(h_Contacts).size() != _contacts.size())
   {
      InfoLog(<< "Mismatched contacts: different number of contacts, msg: " 
              << msg->header(h_Contacts).size() << " expected: " << _contacts.size());
      throw TestException("contacts did not match: different number of contacts", __FILE__,__LINE__);
   }
   
   for (NameAddrs::iterator it = msg->header(h_Contacts).begin();
        it != msg->header(h_Contacts).end(); it++)
   {
      msgContacts.insert(*it);
   }
   if (msgContacts.size() != _contacts.size())
   {
      InfoLog(<< "Mismatched contacts: duplicate entries in message contact set");
      throw TestException("duplicate entries in message contact set", __FILE__,__LINE__);
   }
   
   set<NameAddr>::const_iterator uCon;
   set<NameAddr>::const_iterator msgCon;
   
   for(uCon = _contacts.begin(), msgCon = msgContacts.begin();
       uCon != _contacts.end();
       uCon++, msgCon++)
   {
      resip_assert(uCon != _contacts.end());
      resip_assert(msgCon != msgContacts.end());
      
      if (!(compareContacts(*uCon, *msgCon)))
      {
         CerrLog(<< "Mismatched contact, msg:" << *msg << endl
                 << "-**Expected contact:**-" << endl
                 << *uCon << endl
                 << "Display Name:    " << uCon->displayName() << endl
                 <<  "Q Value:         " << uCon->param(p_q) << endl
                 << "Expires:          " << uCon->param(p_expires) << endl
                 <<  "-**Contact from message:**-" << endl
                 << *msgCon << endl
                 << "Display Name:    " << msgCon->displayName() << endl
                 << "Q Value:         " << msgCon->param(p_q) << endl
                 << "Expires:          " << msgCon->param(p_expires) );
         throw TestException("contacts did not match: mismatch", __FILE__, __LINE__);
      }
   }
}
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
