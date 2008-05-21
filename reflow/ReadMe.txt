reflow (FlowManager)
====================

The flow manager is a layer that faciltates the flow of media to/from a media based application. 
It is responsible for any NAT traversal logic, and will eventually implement ICE as a Nat 
traversal solution.  

Currently the Flow Manager allows NAT traversal via static configuration only.

An application should create one instance of the FlowManager object.  Once an application 
determines that an audio media stream is needed it uses the following API call on the 
FlowManager object to create a MediaStream object.

  MediaStream* createMediaStream(MediaStreamHandler& mediaStreamHandler,
                                 const StunTuple& localBinding, 
                                 bool rtcpEnabled = true,
                                 Flow::NatTraversalMode natTraversalMode = Flow::NoNatTraversal,
                                 const char* natTraversalServerHostname = 0, 
                                 unsigned short natTraversalServerPort = 0, 
                                 const char* stunUsername = 0,
                                 const char* stunPassword = 0);

Possible NatTraversalModes are:

NoNatTraveral - just send media to/from  localBinding

StunBindDiscovery - send STUN bind request to stun server specified and fire an event when 
                    discovered reflexive mapping is retrieved

TurnUdpAllocation - send a TURN allocation request via UDP to the turn server specified and 
                    fire an event when the allocation is complete and allocated address is 
                    known

TurnTcpAllocation - send a TURN allocation request via TCP to the turn server specified and 
                    fire an event when the allocation is complete and allocated address is 
                    known - this allows us to tunnel media via TCP to the TURN server which 
                    is relayed via UDP to the peer

TurnTlsAllocation  - send a TURN allocation request via TLS to the turn server specified and 
                     fire an event when the allocation is complete and allocated address is 
                     known - this allows us to tunnel media via TCP to the TURN server which 
                     is relayed via UDP to the peer

MediaStreamHandler class passed in will receive event notifications:

  virtual void onMediaStreamReady(const StunTuple& rtpTuple, const StunTuple& rtcpTuple) = 0; 
  virtual void onMediaStreamError(unsigned int errorCode) = 0;

 

Each created Media Stream consists of one or two flows, depending on whether RTCP is enabled 
or not.  The Flow objects can be obtained from the MediaStream via the following MediaStream 
API's: 

  Flow* getRtpFlow(); 
  Flow* getRtcpFlow();

 
Each Flow can then be used to send/receive media after the onMediaStreamReady callback has 
been received.  The following Flow API's are available:

  bool isReady();  // will return true after onMediaStreamReady callback has fired

  /// Returns a socket descriptor that can be used in a select call
  /// WARNING - this descriptor should not be used for any other purpose
  /// - do NOT set socket options, or send, receive from this descriptor,
  /// instead use the Flow api's
  unsigned int getSelectSocketDescriptor(); 
  unsigned int getSocketDescriptor(); // returns the real socket descriptor - can be used to 
                                    // correlate callbacks

  /// Turn Send Methods
  void send(const char* buffer, unsigned int size);
  void sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, 
              unsigned int size); 

  /// Receive Methods
  asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, 
                           asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
  asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, 
                               char* buffer, unsigned int& size, unsigned int timeout);
  
  /// Used to set where this flow should be sending to
  void setActiveDestination(const char* address, unsigned short port); 

  const StunTuple& getLocalTuple();
  StunTuple getSessionTuple(); // returns either local, reflexive, or relay tuple depending 
                               // on NatTraversalMode
  StunTuple getRelayTuple();
  StunTuple getReflexiveTuple();



Using SRTP support in FlowManager
---------------------------------

DTLS-SRTP Support
-----------------

FlowManager::initializeDtlsFactory must be called during application 
initialization by passing in an AOR to use in the automatically generated 
client certificate that may be used with dtls-srtp.

The following API's on the Flow object are then used to control dtls-srtp:
  /// Starts the dtls client handshake process - (must call setActiveDestination first)
  /// Call this method if this client has negotiated the "Active" role via SDP
  void startDtlsClient(const char* address, unsigned short port);

  /// This method should be called when remote fingerprint is discovered
  /// via SDP negotiation.  After this is called only dtls-srtp connections
  /// with a matching fingerprint will be maintained.
  void setRemoteSDPFingerprint(const resip::Data& fingerprint);


SDES SRTP Support
-----------------

If using SDES srtp key negotiation, then the following two API's on the
MediaStream object are used in order to initialize the SRTP sessions before
sending or receiving on the RTP or RTCP flows.

   bool createOutboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);
   bool createInboundSRTPSession(SrtpCryptoSuite cryptoSuite, const char* key, unsigned int keyLen);



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

