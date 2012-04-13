

Make/install:
-------------

  sipdialer is built as part of the main reSIProcate build
  when you pass the option --with-apps to configure

    ./configure --with-ssl --with-apps

  From the top level of the reSIProcate source tree, running
  the command

    make install

  should install sipdialer to $bindir (default /usr/local/bin)

The configuration file:
-----------------------

  The configuration file must be located in $HOME/.sipdial/sipdial.cfg

  You can copy the the example file included in the source

  The parameter callerUserAgentVariety describes the type of phone
  that will make the outgoing call.  Each VoIP phone implements 
  support for click-to-dial in a different way.  These phones are
  supported, and the same mechanisms may work with other phones
  from the same manufacturer:

  PolycomIP501:  adds the header field "AlertInfo: AA"
  LinksysSPA941: adds the attribute answer-after=0 to header Callinfo
  Cisco7940:     same as generic
  Generic:       adds no special header fields

  In all cases, sipdialer sends a SIP REFER message to the phone.

Install in gconf:
-----------------
  
Use the shell scipt/commands below to register the dialer with gconf.
gconf aware applications like Mozilla will then be able to use the dialer
to handle sip: and tel: URIs

for scheme in tel sip;
do
  gconftool-2 -t string \
   -s /desktop/gnome/url-handlers/$scheme/command "/usr/local/bin/sipdialer %s"
  gconftool-2 -t bool \
   -s /desktop/gnome/url-handlers/$scheme/needs_terminal false 
  gconftool-2 -t bool \
   -s /desktop/gnome/url-handlers/$scheme/enabled true
done

Usage:
------

  E.164 format:
   sipdialer tel:+442071357000
 
  Local format:
   sipdialer tel:00442071357000

  SIP addresses:
   sipdialer sip:442071357000@lvdx.com


Testing with a SIP proxy:
-------------------------

  The reSIProcate package includes the `repro' SIP proxy, which is ideal
  for testing tools such as sipdial.

  Alternatively, you may also test with products like Kamailio, OpenSIPS,
  sipXpbx or possibly FreeSWITCH or Asterisk.

  Whichever proxy/PBX you choose must permit/recognise the SIP REFER request. 



/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2007 Daniel Pocock  All rights reserved.
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

