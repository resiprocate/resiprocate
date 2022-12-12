
This is a C++ client for controlling the Kurento Media Server

The client does not depend on reSIProcate or SIP, it can be used
in any C++ application.

The client is responsible for the following:

Managing the WebSocket connection used to exchange RPC messages
between the application and the Kurento Media Server.

Encoding and decoding the JSON messages used for commands, results
and notifications.

RPC interface, for example, assigning the ID number required in
each message and routing the response to the correct object.