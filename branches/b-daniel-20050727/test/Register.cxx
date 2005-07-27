#include <memory>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"

#include "Register.hxx"
#include "Transceiver.hxx"

using namespace resip;
using namespace Loadgen;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

Register::Register(Transceiver& transceiver, const resip::Uri& registrand, 
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
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
