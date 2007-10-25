SIPDialTool
===========

The project contains a MacOS Cocoa application which registers itself to
handle sip: and tel: URIs. When the user triggers such a link (for example
by clicking an OpenGroupware.org dial URL in Firefox), MacOS will find this
application and start it.
In turn this application will start any executable called 'sipdialer' with the
given URI as the first commandline argument. The dialer tool is looked for in
either the application bundle or in /usr/local/bin.

Note that the application has no interface, that is, it starts itself in the
background and then keeps running for five minutes. You can find the logs of
the application in the system Console.
