/*
 * Copyright (C) 2014-2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/recon/ReconSubsystem.hxx>

#include "MyMessageDecorator.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;

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
      Data sourceAddr = Tuple::inet_ntop(source);
      bool ipv6Source = sourceAddr.find(":") != Data::npos;

      SdpContents::AddrType addrType = ipv6Source ? SdpContents::IP6 : SdpContents::IP4;
      SdpContents::Session::Connection& c = sdp->session().connection();
      StackLog(<<"session connection address = " << c.getAddress());
      if(isConnectionInvalid(c))
      {
         StackLog(<<"replacing session connection address with " << sourceAddr);
         c.setAddress(sourceAddr, addrType);
      }

      std::list<SdpContents::Session::Connection>& mc = sdp->session().media().front().getMediumConnections();
      std::list<SdpContents::Session::Connection>::iterator it = mc.begin();
      for( ; it != mc.end(); it++)
      {
         SdpContents::Session::Connection& _mc = *it;
         if(isConnectionInvalid(_mc))
         {
            StackLog(<<"replacing media stream connection address with " << sourceAddr);
            _mc.setAddress(sourceAddr, addrType);
         }
      }
   }
}

bool
MyMessageDecorator::isConnectionInvalid(const SdpContents::Session::Connection& connection)
{
   const Data& addr = connection.getAddress();
   return (addr == "0.0.0.0" || addr == "::" || addr.postfix(".invalid"));
}


