reTurn Status
=============

Original Author: Scott Godin 
( s g o d i n  AT  s i p s p e c t r u m  DOT  c o m )


What is reTurn?
---------------
reTurn is a Stun/Turn server and client library implementation of the latest 
Stun/Turn drafts:  RFC5389, and draft-ietf-behave-turn-15


Current External Library Usage
------------------------------
- currently uses OpenSSL, ASIO, BOOST and RUTIL
- OpenSSL
       - Used for TLS support (optional)
       - libcrypto used for calculating message authentication code (mandatory
         part of TURN, even without TLS)
       - Therefore,  unlike other parts of resiprocate, it is mandatory
         to have OpenSSL present for building reTurn, and you must
         specify --with-ssl when running configure or reTurn/* won't
         be built at all.
- ASIO - 1.2.0
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
Configuration Framework                partially    yes     Currently just uses a few command line parameters and hardcoded settings
RFC3489 support                        yes          mostly  
Multi-threaded Server                  no           no      Once Turn code is implemented consider asio threading model and provide locking
TLS Server Support                     yes          yes     
RFC5389 message parsing                yes          partly
IPV6 message parsing support           yes          no 
Short Term Credentials                 yes          yes     Implementation currently only accepts one hardcoded username/password
Long Term Credentials                  mostly       yes     Implementation currently only accepts one hardcoded username/password
Finger Print Insertion and Validation  yes          yes     Uses BOOST:crc_optimal
Checking for unknown attributes        yes          yes
Bandwidth Check                        no           no
Turn Allocation                        yes          yes     Only UDP Relay's are implemented
Requested Props (Even, Pair)           yes          yes
Turn Permissions                       yes          yes      
Turn UDP Relay                         yes          yes     
Turn TCP Relay                         no           no     
Asyncronous Client APIs                yes          yes
Channel Binding                        yes          yes
Don't Fragment Attribute               no           no      Server will reject requests asking for DF option


General TODO
-------------
- reduce library use - remove BOOST and/or rutil requirement - remove ASIO for client??
- allow multiple interfaces to be used for relay
- per user allocation quota enforcement
- cleanup stun message class so that there are accessors for all data members
- from chart above
 - Configuration Framework
 - Multi-threaded support in Server
 - Bandwidth check
 - TCP Relay
- Short Term passwords do not make any sense in reTurnServer (outside of RFC3489 backcompat) - they need to be supported on client APIs

TURN TODO's
-----------
- CreatePermission requests can contain multiple addresses - need to modify StunMessage in order to support this
- Clients need to install permissions before data can be sent - need to queue outbound data until CreateChannel response is received
- ChannelData messages must be padded to a multiple of 4 bytes, the padding is not to be reflected in the encoded length
- When client receives a Data Ind - it should ensure it is from an endpoint that it believes that it has installed a permission for, otherwise drop
- It is recommended that the client check if a permission exists towards the peer that just send a ChannelData message, if not - discard
- Could add checking that ChannelData messages always begin with bits 0b01, since bits 0b10 and 0b11 are reserved
- Need to give clients the ability to add Don't Fragment header to Allocate request
- If request with Don't Fragment fails with 420 code, then it can be retried without Don't Fragment (this will likely remain the responsibilty of the reTurn client application)
- It is recommended that the server impose limits on both the number of allocations active at one time for a given username and on the amount of bandwidth those allocations use. - 486 (Allocation Quota Exceeded)
- Port allocation algorithm should be better to ensure we won't run into collisions with other applications - port allocations should be random
- If the client receives a 400 response to a channel-bind request, then it is recommended that the allocation be deleted, and the client start again with a new allocation

RFC53389 TODO's
---------------
-Username must contain UTF-8 sequence of bytes, and must have been processed by SASLprep
-Realm qdtext or quoted-pair - It must UTF-8 encoded and MUST be less than 128 characters (which can be as long as 763 bytes), and must be processed by SASLprep
-Nonce qdtext or quoted-pair - MUST be less than 128 characters (which can be as long as 763 bytes)
-Software must be a UTF-8 sequence of less than 128 characters (which can be as long as 763 byes)
-The Password used in the HMAC key must be SASLprep processed
-remove quotes and trailing nulls from username, realm.  remove trailing nulls from password before forming MD5 hash for message integrity
-Errorcode Reason Phrase must be a UTF-8 sequence of less than 128 characters (which can be as long as 763 byes)
-need handling for 300 Try Alternate response - currently applications responsibility
-the following values should be configurable
 - Initial RTO (default 500ms)
 - Rc (default 7)
 - Rm (default 16)
-actual RTO should be calculated
-UDP retransmissions should stop if a hard ICMP error is seen
-DNS SRV Discovery - currently only does host record lookup (using ASIO) - _stun._udp, _stun._tcp, _stuns._tcp, _turn._udp, _turn._tcp, _turns._tcp

Client TODO
-----------
- rework synchronous sockets to use Asynchrous sockets to unify implementation better
- keepalive usage??
- add option to require message integrity? - depends on usage - ICE
- add ability to install a permission or binding without sending data

Client Notes
------------         
- retries should be paced at 500ms, 1000ms, 2000ms, etc. - after 442, 443, or 444 response - currently applications responsibility
- If a client receives a 437 allocation mismatch response to an allocate, then it should retry using a different client transport address - it should do this 3 times (this will likely remain the responsibilty of the reTurn client application)
- To prevent race conditions a client MUST wait 5 mins after the channel binding expires before attempting to bind the channel number to a different transport address or the transport address to a different channel number (within the same allocation?).


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
