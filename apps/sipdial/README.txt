

Make/install:

  make
  cp libsipdial.so /usr/lib
  cp sipdialer /usr/bin

Install in gconf:
  
Use the shell scipt/commands below to register the dialer with gconf.
gconf aware applications like Mozilla will then be able to use the dialer
to handle sip: and tel: URIs

for scheme in tel sip;
do
  gconftool-2 -t string -s /desktop/gnome/url-handlers/$scheme/command "/usr/local/bin/sipdial %s"
  gconftool-2 -s /desktop/gnome/url-handlers/$scheme/needs_terminal false -t bool
  gconftool-2 -t bool -s /desktop/gnome/url-handlers/$scheme/enabled true
done

Usage:

  E.164 format:
   sipdialer tel:+442071357000
 
  Local format:
   sipdialer tel:00442071357000

  SIP addresses:
   sipdialer sip:442071357000@lvdx.com


Testing with a SIP proxy:
  You can test with SER, http://www.iptel.org/ser

  If you don't want to install SER, you can test with the LVDX.com SIP proxy,
  visit

     http://www.opentelecoms.org
  
  to register.

Copyright (C) 2007 Daniel Pocock

