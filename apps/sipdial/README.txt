

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

  sipdialer looks for a configuration file located in
   $HOME/.sipdial/sipdial.cfg

  You can also specify a configuration file on the command line
  as the second parameter (after the target URI).

  You can copy the the example file included in the source

  The parameter callerUserAgentVariety describes the type of phone
  that will make the outgoing call.  Each VoIP phone implements 
  support for click-to-dial in a different way.  These phones are
  supported, and the same mechanisms may work with other phones
  from the same manufacturer:

  AlertInfo:     adds the header field "AlertInfo: AA"
                 (this value was formerly called PolycomIP501)
  LinksysSPA941: adds the attribute answer-after=0 to header Callinfo
  Cisco7940:     same as generic
  Generic:       adds no special header fields

  In all cases, sipdialer sends a SIP REFER message to the phone.

Using TLS
---------

  sipdial expects to use TLS to secure the exchange of
  SIP REFER messages.

  sipdial expects to find CA root certificates in files matching
  the pattern $HOME/.sipdial/certs/root_cert_*.pem
  (the legacy reSIProcate SSL convention) or it can be told
  to load all CA root certificates from a directory such
  as /etc/ssl/certs on Debian by using the config option
  CADirectory = /etc/ssl/certs

  If multiple certificates are concatenated in a PEM file, it will only
  load the first.

  In ~/.sipdial/sipdial.cfg, the callerUserAgentAddress must use
  a sips URI:

    callerUserAgentAddress = sips:mydeskphone@example.org

Install in gconf:
-----------------
  
Use the shell scipt/commands below to register the dialer with gconf.
gconf aware applications like Mozilla will then be able to use the dialer
to handle sip: and tel: URIs

for scheme in tel sip sips;
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
   sipdialer sip:helpdesk@sip5060.net


Testing with a SIP proxy:
-------------------------

  The reSIProcate package includes the `repro' SIP proxy, which is ideal
  for testing tools such as sipdial.

  Alternatively, you may also test with products like Kamailio, OpenSIPS,
  sipXpbx or possibly FreeSWITCH or Asterisk.

  Whichever proxy/PBX you choose must permit/recognise the SIP REFER request. 



/* ====================================================================
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

