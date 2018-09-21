#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <AppSubsystem.hxx>

#include "MyMessageDecorator.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace reconserver;

MyMessageDecorator::MyMessageDecorator()
{
}

void
MyMessageDecorator::decorateMessage(SipMessage &msg,
                                  const Tuple &source,
                                  const Tuple &destination,
                                  const Data& sigcompId)
{
   StackLog(<<"Got a message for decoration");

   SdpContents *sdp = dynamic_cast<SdpContents*>(msg.getContents());
   if(sdp)
   {
      StackLog(<<"SDP found, checking message");

      /* If we are bound to 0.0.0.0 and no NAT traversal mode has
         been enabled then at this point, the SDP will usually have
         0.0.0.0 as the connection IP.

         This is generally not desirable.
         
         Under legacy SIP RFC 2543, this would signify putting a call on hold.

         Under RFC 3264, it is also suggested that this special
         address can be used in the connection line by a UA
         that is yet to discover which address it should use.
         This could be the case if ICE/TURN is not yet complete.

         However, for the vast majority of cases, it is desirable
         to replace 0.0.0.0 with the source IP that will be used
         for the outgoing SIP packet.  The SIP stack only knows
         this at the last moment before sending it to the wire
         and that is why it is substituted here in a MessageDecorator.
      */
      SdpContents::Session::Connection& c = sdp->session().connection();
      StackLog(<<"session connection address = " << c.getAddress());
      if(c.getAddress() == "0.0.0.0")
      {
         Data newAddr = Tuple::inet_ntop(source);
         StackLog(<<"replacing session connection address with " << newAddr);
         c.setAddress(newAddr);
      }
      std::list<SdpContents::Session::Connection>& mc = sdp->session().media().front().getMediumConnections();
      std::list<SdpContents::Session::Connection>::iterator it = mc.begin();
      for( ; it != mc.end(); it++)
      {
         SdpContents::Session::Connection& _mc = *it;
         if(_mc.getAddress() == "0.0.0.0")
         {
            Data newAddr = Tuple::inet_ntop(source);
            StackLog(<<"replacing media stream connection address with " << newAddr);
            _mc.setAddress(newAddr);
         }
      }
   }
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

