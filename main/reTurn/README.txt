reTurn Status
=============

Original Author: Scott Godin 
( s g o d i n  AT  s i p s p e c t r u m  DOT  c o m )


What is reTurn?
---------------
reTurn is a Stun/Turn server and client library implementation of the latest 
Stun/Turn drafts:  RFC3489bis13, and draft-ietf-behave-turn-05


Current External Library Usage
------------------------------
- currently uses ASIO, BOOST and RUTIL
- ASIO - 0.3.8 
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
- RUTIL - Data class is used in StunMessage and StunAuth for strings and TurnData, SharedPtr is also used


Feature                                Implemented  Tested  Notes
-------------------------------------------------------------------
Configuration Framework                no           no      Currently just uses a few command line parameters and hardcoded settings
RFC3489 support                        yes          mostly  
Multi-threaded Server                  no           no      Once Turn code is implemented consider asio threading model and provide locking
TLS Server Support                     yes          yes     need to tweak openSSL cipher suite settings
RFC3489 bis 11 message parsing         yes          partly
IPV6 message parsing support           yes          no 
Shared Secret with Short Term Cred     yes          yes     Checking username for expirey and correct HMAC is not completed
Shared Secret with Long Term Cred      little       no      Implementation is not complete and currently only accepts one hardcoded username/password - nonce generation not implemented
Finger Print Insertion and Validation  yes          yes     Uses BOOST:crc_optimal
Checking for unknown attributes        no           no
Bandwidth Check                        no           no
Turn Allocation                        almost       no
Requested Port Props (Even, Odd, etc)  yes          yes
Turn Permissions                       yes          yes      
Turn Relay                             partly       partly  UDP Peers only
Asyncronous Client APIs                no           no


General TODO
-------------
- Complete Turn Implementation - TCP/TLS relay - Implement Connect Request and Connect Status Indication
- reduce library use - remove BOOST and/or rutil requirement - remove ASIO for client??
- allow multiple interfaces to be used for relay
- per user allocation quota enforcement
- set TLS server settings - ie. allowed cipher suite, etc.
- cleanup stun message class so that there are accessors for all data members
- Ability for Server to accept long term credentials - 3489-bis13 digest
- Check for unknown attributes
- Standardize on a logging interface (rutil?)
- from chart above
 - Configuration Framework
 - Multi-threaded support
 - Bandwidth check


Client TODO
-----------
- asynchrounous turn sockets need retranmission and stun request timeout logic
- rework synchronous sockets to use Asynchrous sockets to unify implementation better
- retries should be paced at 500ms, 1000ms, 2000ms, etc. - after 442, 443, or 444 response - currently applications responsibility
- DNS SRV Discovery - currently only does host record lookup (using ASIO)
- Note: requests can be piplined - currently client does not send pipelined
- client long term password use - auto re-request after 401
- implement 300 Try-Alternate response - currently applications responsibility
- Try next DNS entry on failure - currently applications responsibility
- sort out thread safety on Turn Async Sockets
- keepalive usage
         


Client API
-----------
Current Asynchronous Implementation:
- Application must provide an asio::io_service object and is responsible for threading it out and calling run
- Async Turn sockets must be held in a shared pointer, in order to ensure safety of asio callbacks - this could be abstracted better
- When Async sockets are created a callback handler class is passed in to receive callback notifications when 
  operations are complete

Synchronous API Set - Wrapping in a TurnSocket - TurnUdpSocket, TurnTcpSocket, TurnTlsSocket - bound to local socket
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
 - connectAllocation(destinationIP, destinationPort)
 - listenAllocation(destinationIP, destinationPort)
 * bindRequest(remoteIP, remotePort) - what other args are required? - need to return binding info - full 3489 not yet supported
 * send(bufferToSend, bufferSize);      
 * sendTo(destinationIP, destinationPort, bufferToSend, bufferSize)
 * receive(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) - last 2 args are return args - if receive is non-blocking the this data is returned in callback instead 
 * receiveFrom(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) - in this case last 2 args are input and specify endpoint we want to receive from
NOTE:  could also add a binding discovery API for attempting to detect NAT type using RFC3489 methods
 
Asynchronous Callbacks:

onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port) = 0;
onConnectFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize) = 0;  
onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple) = 0; 
onBindFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth) = 0; 
onAllocationFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime) = 0;
onRefreshFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size) = 0;
onReceiveFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onSendSuccess(unsigned int socketDesc) = 0;
onSendFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

