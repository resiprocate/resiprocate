reTurn Status
=============

Original Author: Scott Godin ( s g o d i n  AT  s i p s p e c t r u m  DOT  c o m )


What is reTurn?
---------------
reTurn is a Stun/Turn server and client library implementation of the latest 
Stun/Turn drafts:  RFC3489bis11, and draft-ietf-behave-turn-05


Current External Library Usage
------------------------------
- currently uses ASIO, BOOST and RUTIL
- ASIO - 0.3.8 RC3
       - Used for server sockets and transports
       - Tuple information used in StunMessage uses asio::ip::udp::endpoint - easily changed
       - StunMessage, TurnAllocation and RequestHandler use asio::ip:address to manipulate IPV6, 
         and IPV4 StunAddresses - easily changed
       - StunTuple uses asio::ip::address - easily changed
- BOOST - 1.34.1
        - Using BOOST in no-lib mode is fine
        - BOOST::bind is used in server transports
        - BOOST::crc_optimal is used for fingerprint CRC calculations
        - BOOST::shared_ptr, array, enable_shared_from_this is used in server transports
- RUTIL - Data class is used in StunMessage and StunAuth for strings and TurnData
        - Mutex class used in TurnManager


Feature                                Implemented  Tested  Notes
-------------------------------------------------------------------
Configuration Framework                no           no      Currently just uses a few command line parameters and hardcoded settings
RFC3489 support                        yes          mostly  
Multi-threaded Server                  no           no      Once Turn code is implemented consider asio threading model and provide locking
TLS Server Support                     yes          yes     TLS server is working but first connect on program start always fails - need to tweak openSSL settings
RFC3489 bis 11 message parsing         yes          partly
IPV6 message parsing support           yes          no 
Shared Secret with Short Term Cred     yes          yes     Checking username for expirey and correct HMAC is not completed
Shared Secret with Long Term Cred      little       no      Implementation is not complete and currently only accepts one hardcoded username/password - nonce generation not implemented
Finger Print Insertion and Validation  yes          yes     Uses BOOST:crc_optimal
Checking for unknown attributes        no           no
Bandwidth Check                        no           no
Turn Allocation                        almost       no
Requested Port Props (Even, Odd, etc)  yes          yes
Turn Permissions                       yes          no      Permissions need more thought when TCP/TLS relay is implemented
Turn Relay                             partly       partly  UDP destinations only
Asyncronous Client APIs                no           no


General TODO
-------------
- Complete Turn Implementation - TCP/TLS relay - Implement Connect Request and Connect Status Indication
- Troubleshoot why first TLS connection on server fails, but subsequent succeeds
- optimize server data copying - especially for TURN DATA forwarding
- optimize client data copying when receiving data
- reduce library use - remove BOOST requirement - remove ASIO for client??
- fix socket read code for TCP/TLS to be able to read messages that come in non-contiguously
- allow multiple interfaces to be used for relay
- per user allocation quota enforcement
- set TLS server settings - ie. allowed cipher suite, etc.
- cleanup stun message class so that there are accessors for all data members
- cleanup TurnAllocation class so that there are accessors for all data members
- Ability for Server to accept both long term credentials
- Check for unknown attributes
- if server receives SendInd with no Data attribute (for UDP only), send empty UDP packet, TCP - do nothing
- Shared Secret Re-Transmissions when receiving duplicate request, must return exact same response (need to store/cache in server)
- Standardize on a logging interface
- from chart above
 - Configuration Framework
 - Multi-threaded support
 - Bandwidth check
 - Asyncronous Client API's


Client TODO
-----------
- don't allow TCP allocate request over UDP
- refresh allocations (3/4 lifetime) <- before
- retries should be paced at 500ms, 1000ms, 2000ms, etc. - after 442, 443, or 444 response
- ability to send an empty packet in a SendInd
- DNS Discovery (?)
- Server Binding Usage - ability to handle Binding Requests, Shared Secret Requests (?)
- Keep Alive Usage
- requests can be piplined
- client long term password use - auto re-request after 401
- implement 300 Try-Alternate response
- Try next DNS entry on failure
         


Client API
-----------
Client API thoughts:
1.  Syncronous vs Asyncrous  - pretty much all of the client API commands require sending a request to the server and waiting for the response.  Assuming we want some sort of asyncrous api, there are a few ways we could implement this: 
 - Queue all results to an application fifo - I don't think this approach is really good from an API usability standpoint though
 - Use a callback handler approach - like DUM - I think this makes the most sense, but we need to be careful about threading, since if we are using a ASIO thread pool - completion conditions can be met from any thread in the pool - to avoid this we could queue results to a common fifo

2.  API Set - Wrapping in a TurnSocket - TurnUdpSocket, TurnTcpSocket, TurnTlsSocket - bound to local socket
Note: API with a * are implemented
 * requestSharedSecret(turnServerIP, turnServerPort, username, password) - username and password are returned
 * createAllocation(turnServerIP, turnServerPort, username, password, lifetime, bandwidth, requestedPortProps, requestedPort, requestedTransportType, requestedIpAddress)
 * destroyAllocation() 
 * refreshAllocation()
 * getRelayTuple() - used to retrieve info about the allocation
 * getReflexiveTuple() - used to retrieve info about the allocation
 * getLifetime() - used to retrieve info about the allocation
 * getBandwidth() - used to retrieve info about the allocation
 * setActiveDestination(destinationIP, destinationPort)
 * clearActiveDestination()
 - stunConnect(destinationIP, destinationPort) - is this used for TLS? 
 - stunBind(remoteIP, remotePort) - what other args are required? - need to return binding info
 * send(bufferToSend, bufferSize);      
 * sendTo(destinationIP, destinationPort, bufferToSend, bufferSize)
 * receive(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) - last 2 args are return args - if receive is non-blocking the this data is returned in callback instead 
 * receiveFrom(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) - in this case last 2 args are input and specify endpoint we want to receive from - do we really need this api?
NOTE:  could also add a binding discovery API for attempting to detect NAT type using RFC3489 methods
 
3.  Callbacks (not implemented yet):
- allocationSucceeded(mappedAddress, relayAddress)
- allocationFailed(status)
- bindingSucceeded(mappedAddress)
- bindingFailed(status)
- sharedSecretRequestSucceeded(secret)
- sharedSecretRequestFailed(status)
- connectStatus(state)
- sendSucceeded? - not sure if we need this - send can probably be syncronous
- sendFailed?
- dataReceived(data, senderIPAddress, senderPort, sendTransportType?)
