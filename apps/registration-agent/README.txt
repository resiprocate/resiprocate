
This is a standalone process for registering with a SIP
registration server.

It is not able to receive calls itself.  Instead, a Contact
header value is specified in the configuration file and
this value is given to the registration server.  It can be
convenient to use a Contact header that routes calls to a SIP
proxy as SIP proxies don't usually have a built-in registration
agent of their own.

