#include "tfm/CheckContacts.hxx"
#include "tfm/SipEvent.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace std;
using namespace resip;

CheckContacts::CheckContacts(const std::set<resip::NameAddr>& contacts,
                             unsigned int expiresHeader)
   : _contacts(contacts),
     _expiresHeader(expiresHeader)
{}

bool 
CheckContacts::compareContacts(const NameAddr & s1, 
                               const NameAddr & s2)
{
   float q1 = (float)(s1.exists(p_q) ? s1.param(p_q) : 1.0);
   float q2 = (float)(s2.exists(p_q) ? s2.param(p_q) : 1.0);
   return (s1.uri().getAor() == s2.uri().getAor() && q1 == q2);
}      

bool
nearTo(int t1, int t2, int tol ) 
{
   return (abs(t2-t1) < tol);
}

void 
CheckContacts::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();

   set<NameAddr> msgContacts;
   for (NameAddrs::iterator it = msg->header(h_Contacts).begin();
        it != msg->header(h_Contacts).end(); it++)
   {
      DebugLog(<< *it);
      msgContacts.insert(*it);
   }

   if (msgContacts.size() != msg->header(h_Contacts).size())
   {
      InfoLog(<< "Mismatched contacts: different number of contacts, msg: " 
              << msg->header(h_Contacts).size() 
              << " expected: " << _contacts.size() 
              << endl 
              << *msg);
      throw TestException("contacts did not match: different number of contacts", __FILE__, __LINE__);
   }
   
   if (msgContacts.size() != _contacts.size())
   {
      InfoLog(<< "Mismatched contacts: different number " << msgContacts.size() << " / " << _contacts.size() );
      InfoLog (<< "msg contacts: " << Inserter(msgContacts));
      InfoLog (<< "expected contacts: " << Inserter(_contacts));
      throw TestException("Mismatched contacts: different number", __FILE__,__LINE__);
   }
   
   set<NameAddr>::const_iterator uCon;
   set<NameAddr>::const_iterator msgCon;
   
   for(uCon = _contacts.begin(), msgCon = msgContacts.begin();
       uCon != _contacts.end();
       uCon++, msgCon++)
   {
      if ( !(compareContacts(*uCon, *msgCon)) ||
           (uCon->exists(p_expires) && !msgCon->exists(p_expires)) ||
           (uCon->exists(p_expires) && (msgCon->param(p_expires) > uCon->param(p_expires))) ||
           (!uCon->exists(p_expires) && _expiresHeader && (!msgCon->exists(p_expires) || msgCon->param(p_expires) > _expiresHeader )))
           
         //(uCon->exists(p_expires) && (uCon->param(p_expires) != msgCon->param(p_expires))) ||
         //(!uCon->exists(p_expires) && (_expiresHeader != msgCon->param(p_expires))))
      {
         InfoLog(<< "Mismatched contact, msg:" << *msg);
         InfoLog (<< "-**Expected contact:**-" << *uCon);
         InfoLog (<< "-**Contact from msg:**-" << *msgCon);
         throw TestException("mismatched contacts", __FILE__,__LINE__);
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
