reTurn Status
=============

Original Author: Scott Godin 
( s g o d i n  AT  s i p s p e c t r u m  DOT  c o m )


What is reTurn?
---------------
reTurn is a Stun/Turn server and client library implementation of the latest 
Stun/Turn drafts:  RFC3489bis-15, and draft-ietf-behave-turn-07


Current External Library Usage
------------------------------
- currently uses ASIO, BOOST and RUTIL
- ASIO - 1.0.0
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
TLS Server Support                     yes          yes     
RFC3489 bis 13 message parsing         yes          partly
IPV6 message parsing support           yes          no 
Shared Secret with Short Term Cred     yes          yes     Checking username for expirey and correct HMAC is not completed
Shared Secret with Long Term Cred      mostly       yes     Implementation currently only accepts one hardcoded username/password - not implement on client side sync sockets
Finger Print Insertion and Validation  yes          yes     Uses BOOST:crc_optimal
Checking for unknown attributes        no           no
Bandwidth Check                        no           no
Turn Allocation                        yes          yes     Only UDP Relay's are implemented
Requested Props (Even, Pair)           yes          yes
Turn Permissions                       yes          yes      
Turn Relay                             yes          yes     UDP Peers only
Asyncronous Client APIs                yes          yes
Channel Binding                        yes          yes


General TODO
-------------
- reduce library use - remove BOOST and/or rutil requirement - remove ASIO for client??
- allow multiple interfaces to be used for relay
- per user allocation quota enforcement
- move TLS server settings to configuration
- cleanup stun message class so that there are accessors for all data members
- Check for unknown attributes
- Timeout Channel Bindings - currently binding last until the allocation is destroyed
- The server is supposed to prevent a relayed transport address and the 5-tuple from being
  reused in different allocations for 2 minutes after the allocation expires
- SASL Prep for unicode passwords (RFC4013)
- from chart above
 - Configuration Framework
 - Multi-threaded support
 - Bandwidth check


Client TODO
-----------
- rework synchronous sockets to use Asynchrous sockets to unify implementation better
- Note:  synchronous sockets currently do not support long term authentication
- retries should be paced at 500ms, 1000ms, 2000ms, etc. - after 442, 443, or 444 response - currently applications responsibility
- DNS SRV Discovery - currently only does host record lookup (using ASIO)
- implement 300 Try-Alternate response - currently applications responsibility
- use of a calculated RTO for retransmissions
- TLS client- post connect/handshake server hostname validation
- keepalive usage??
- add option to require message integrity - depends on usage - ICE
         


Client API
-----------
Current Asynchronous Implementation:
- Application must provide an asio::io_service object and is responsible for threading it out and calling run
- Async Turn sockets must be held in a shared pointer, in order to ensure safety of asio callbacks - this could be abstracted better
- When Async sockets are created a callback handler class is passed in to receive callback notifications when 
  operations are complete

API Set - Wrapping in a Turn(Async)Socket - Turn(Async)UdpSocket, Turn(Async)TcpSocket, Turn(Async)TlsSocket 
        - bound to local socket
 * setUsernameAndPassword()
 * requestSharedSecret() - username and password are returned
 * createAllocation(lifetime, 
                    bandwidth, 
                    requestedPortProps, 
                    reservationToken, 
                    requestedTransportType)
 * refreshAllocation(lifetime)
 * destroyAllocation() 
 * getRelayTuple() - (SYNC API ONLY) used to retrieve info about the allocation
 * getReflexiveTuple() - (SYNC API ONLY) used to retrieve info about the allocation
 * getLifetime() - (SYNC API ONLY) used to retrieve info about the allocation
 * getBandwidth() - (SYNC API ONLY) used to retrieve info about the allocation
 * setActiveDestination(destinationIP, destinationPort)
 * clearActiveDestination()
 * bindRequest() 
 * send(bufferToSend, bufferSize);      
 * sendTo(destinationIP, destinationPort, bufferToSend, bufferSize)
 * receive(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) 
   - last 2 args are return args - if receive is non-blocking then this data is returned in callback instead 
 * receiveFrom(bufferToReceiveIn, bufferSize[in/out], senderIPAddress, senderPort) 
   - in this case last 2 args are input and specify endpoint we want to receive from

NOTE:  could also add a binding discovery API for attempting to detect NAT type using RFC3489 methods
 
Asynchronous Callbacks:

onConnectSuccess(unsigned int socketDesc, 
                 const asio::ip::address& address, 
                 unsigned short port) = 0;
onConnectFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onSharedSecretSuccess(unsigned int socketDesc, 
                      const char* username, 
                      unsigned int usernameSize, 
                      const char* password, 
                      unsigned int passwordSize) = 0;  
onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple) = 0; 
onBindFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onAllocationSuccess(unsigned int socketDesc, 
                    const StunTuple& reflexiveTuple, 
                    const StunTuple& relayTuple, 
                    unsigned int lifetime, 
                    unsigned int bandwidth, 
                    UInt64& reservationToken) = 0; 
onAllocationFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime) = 0;
onRefreshFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onSetActiveDestinationSuccess(unsigned int socketDesc) = 0;
onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e) = 0;
onClearActiveDestinationSuccess(unsigned int socketDesc) = 0;
onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code &e) = 0;

onReceiveSuccess(unsigned int socketDesc, 
                 const asio::ip::address& address, 
                 unsigned short port, 
                 const char* buffer, 
                 unsigned int size) = 0;
onReceiveFailure(unsigned int socketDesc, const asio::error_code& e) = 0;

onSendSuccess(unsigned int socketDesc) = 0;
onSendFailure(unsigned int socketDesc, const asio::error_code& e) = 0;


License
-------


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
