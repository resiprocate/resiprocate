# Change Log

## 1.13.2 Changes
* DNS fixes
  * ares__send_query: make sure if a query goes over TCP that it times out correctly
  * fix DNS parsing bug if NULLs are embedded into domain name
  * if we fail to parse a DNS record when adding to the cache, then avoid caching it
  * if a DNS record ends up in cache with no destination records, then ensure we don't try to access non-existing record
  * ensure items removed from cache are properly removed from LRU list
  * fix memory leak in DNSStub logic
  * ares_parse_a_reply - bring over a leak fix from c-ares
  * ares_send.c - bring over fix from c-ares - returns correct error code when we are out of servers, and avoids extra allocations
  * RRList::update - make sure we don't use ULONG_MAX as TTL
  * AresDns - added helper method dnsRRTypeToString
  * DnsResult - reduce number of log statements leaving detail the same (or better)
  * DnsAAAARecord and DnsHostRecord - throw if name fails to parse
  * RRList - cleanup DNSCache logging - reduce complexity and ensure cached failure lookups are logged as well
  * DnsStub - fix bug in typeToData
  * DnsStub - ensure we won't infinite loop on CName lookups
  * ares read_tcp_data and ares__send_query - if malloc fails don't continue to use memory
* update linux CI to use ubuntu-22.04, since 20.04 will be decommissioned in April 2025
* bump contrib\srtp\CMakeLists.txt cmake_minumum_required version to match base resip CMakeList.txt
* Fix inconsistent include guards in p2p/DictionaryValue.hxx
* Replace set with unordered_set in BasicDomainMatcher and ExtendedDomainMatcher
* Use find() instead of count() for element existence checks in STL containers
* Optimize locking in DnsInterface::isSupported()
* Early return in ExtendedDomainMatcher::isMyDomain() when mDomainSuffixList is empty, preventing unnecessary domain case conversions
* Apply fixes from static code analysis (Coverity)
  * fix some potential resource/memory leaks
  * fix some potential unintialized access issues
  * fix some large object copies
* Fix NameAddr parsing bug when angle brackets are included in quoted parameters
* If SSL_read fails after SSL_pending returns > 0, then keep bytes from original read if error code is retryable
* Optimize lock scopes for improved efficiency
* Improve logging in DialogUsageManager::sendUsingOutboundIfAppropriate


## 1.13.1 Changes
* various build system and compilation fixes
* add Windows CI script for GitHub actions
* fix assert in TransactionState that is possible if client sends an ACK to a response on a non-INVITE transaction
* allow recon(sipXtapi) on windows to find codec dll's in same directory as the executable
* ensure a STUN error attribute is at least minimum size   


## 1.13 Changes

### General
* switch to CMake build system (supports both linux and Windows builds)
  * CMake is the new supported/maintained build system for Visual Studio
    * older VS2017 files have been remove entirely, VS2019 and VS2022 files remain in place temporarily during this transition
* added OpenSSL 3 Support (OpenSSL 1.1 is minimum supported version, Windows build still uses OpenSSL 1.1)
* add new media engine options to recon: Kurento, G-Streamer, and LibWebRTC
* now requires C++11 compiler support
* replace std::auto_ptr with std::unique_ptr
* replace resip::SharedPtr and boost::shared_ptr with std::shared_ptr
* replace boost::bind with std::bind or lambda expressions
* replace pcre with std::regex
* remove boost dependency for reTurn, reflow, resip/recon: adapt to use C++ 11 API
* use Nuget on Windows build for OpenSSL (zeroc.openssl), BerkeleyDb (only needed for repro) and Boost (only needed by Rend)
* pcre and fmt libraries are no longer required and have been removed from the contrib folder
* removed many compiler warnings (most related to 64 bit builds)
* see http://www.sipspectrum.com/blog/updating-applications-to-resip-113 for help in porting applications

### rutil
* replace Lock and RWMutex with std::mutex 
* ThreadIf: migrate to C++11 std::thread
* replace HashMap with C++11 std::unordered_map
* XMLCursor: fix parsing bug when space is used before tag closing slash, ie: <unknown />
* Log: add support for logging method name using __func__ (C++11)
* FileSystem: fix directory iteration
* Log.cxx: log logger initialization with repository details
* ServerProcess: reduce default sleep duration from 1000ms to 50ms, make it configurable
* AbstractFifo: adapt to use steady_clock and wait_until instead of wait_for
* add generalized DSO plugin and Python plugin APIs
* Stun.cxx - ensure we have a enough bytes for a full stun header before casting
* ConfigParse: add method getConfigValue for vector<int>
* add a public getter for the percent of maximum tolerance for Congestion Manager
* store original ttl in DnsResourceRecord objects and provide accessor, output ttl in dump methods, output individual record tll in DNS cache dump
* avoid crashing if DNS lookup returns unhandled Resource Record (RR) type
* avoid crash on malformed SOA response
* ares: update ares_expand_name with some changes cherry picked from c-ares version from earlier this year.  Contains a number of name parsing fixes.
* add support for constructing resip::Data from a std::string_view when using C++17 or higher
* override ==, !=, and += operators for resip::Data to enable seamless comparison and concatenation with std::string and std::string_view
* switch Data::data_type to use size_t instead of uint32_t to be inline with C++ API size type

### stack
* update embedded STUN support to be closer to RFC5389/RFC8489 compliance
* tests: remove use of old SipStack::buildFdSet/process API in various applications and unit tests
* fix potential crash in presence subscription handling if there is an error parsing a notify body
* DialogInfoContents: add support for shared appearance namespace items in DialogInfoContents - from RFC 7463
* make Tuple::isPrivateAddress compatible with Shared Address Space(RFC 6598)
* Transport: make SIPMessageLoggingHandler thread safe to unset or replace at runtime
* add MediaControlContents for RFC 5168 keyframe requests
* DateCategory: make DayOfWeek in Date header optional on both parse and encoded
* fix a bug in TCP/TLS/WS/WSS transports where a CRLF pong could potentially be misinterpreted as a CRLFCRLF ping, and throw off the framing for the stream
* "Retry-After" header in 503 responses uses wrong values (incorrect units)
* ParameterTypes: p_text must be quoted in RFC 3326, boolean in RFC 3840
* ensure UDP and DTLS transports can handle message sizes up to 64KB
* add options to test stack allowing TLS and DTLS transports to be exercised
* add TrickleIceContents for RFC 8840 trickle ICE used by WebRTC media stacks and Kurento
* SdpContents: add makeIceFragment to create trickle ICE fragment associated with the session
* SdpContents: add isTrickleIceSupported
* SipStack, Transport: allow multiple SipMessageLoggingHandlers to be added
* ungreylist a DNS entry when whitelisted
* fail to parse headers containing an empty Call-Id - RFC3261 ABNF does not allow an empty Call-ID
* allow multiple identity headers, ala RFC8224, avoids rejecting messages from endpoints that have implemented STIR/SHAKEN and may be inserting more than one Identity header
* allow ;'s in Uri embedded headers to be unescaped if Uri is surrounded by angle brackets/quotes
* modify NameAddr to handle embedded double-quotes and backslashes in the DisplayName
* improve p_received handling without value
* improve duplicate parameters handling in ParserCategory, skip duplicate parameter-names when parsing
* enhance TLS transport selection logic when multiple TLS transports are added
* consider `gr` param when comparing URIs
* consider parameters in Uri less-than operator
* allow DNS responses who's case is different from the request to still matchup and be used

### dum
* append stale flag to authenticate header appropriately
* DialogSet: If a REFER or NOTIFY request has a To tag, but it doesn't match an existing dialog, then return a 481
* Dialog: ensure that a REFER request with no subscription that arrives in a non-INVITE session dialog, is responded to with a 603 (doesn't assert)
* add more options for setting the Reason header ((RFC3326) for inclusion in BYE and CANCEL requests sent by InviteSessions
* ensure we always use the configured keepalive time for a flow timer if it is less than the Flow-Timer value from an outbound registration
* ensure if we get a 422 to a provideOffer reINVITE, and sessionRefresh can get called, that we don't send a session refresh via an UPDATE, use reINVITE and send previously proposed SDP
* ensure stray acks (potentially ACK retransmits from previous offer/answer exchange) don't interfere with current re-invite transaction
* fix potential crash in UAS subscriptions 
* fix scenario when Prack is enabled and application sends 2 provisionals quickly
* allow ACK to be handled in the SentReInvite state
* allow defining custom status codes and messages for authentication errors (userAuthInfo class)

### repro
* add new boolean option AllowWildcardCertificates, helps making repro work with secure Twilio trunks
* add new AllowInDialogImpersonationWithinRealm setting that allows repro to authenticate in an environement where 302 redirects are used
* ResponseContext: add the possibility of creating a derived class to implement custom logic for forwardBestResponse()
* fix best response code to be 500 (vs 480) if 503 is received, following RFC 3261

### reTurn
* asio updated to version 1.18.1
* fix static initialization issue that can cause a startup deadlock
* TurnAsyncSocket - allow software attribute to be customized
* fix the async client to choose the DNS result corresponding to local endpoint protocol type
* add EchoServer application that can be used to echo RTP/RTCP data back at sender for TURN server load testing
* add TurnLoadGenClient that can be used to load test a TURN server
* fix issues in TurnAsyncSocket with creating a second allocation after ending a previous one

### reflow
* update to libsrtp2 v2.3.0
* fix memory leak in reflow if received data fifo queue is full
* vastly improve Flow logging, so you can now tie all logs related to a single flow together, log SIP CallId in Flow logs
* fix bug where a 10061 error on UDP receive could cause receive process to stop

### recon
* add new media engine options to recon: Kurento, G-Streamer, and LibWebRTC
* added a new AutoHold setting allowing you to disable recon's auto hold functionality
* added basic ability for applications to run timers:  see createApplicationTimer
* allow any SIP header to be specified via extra headers parameter to createRemoteParticipant
* add new ForkSelectAutomaticEx mode supporting auto destruction and auto-cancel
* add new virtual method on ConversationManager: onMediaResourceParticipantFailed
* added extra protection during shutdown to ensure new conversations or participants cannot be created
* added new virtual method ConversationManager::getBufferFromMediaResourceCache to allow media resource cache implementation be be overriden
* ConversationManager: added onParticipantConnectedConfirmed callback for UAS calls when ACK arrives
* allow 3xx redirect code to be specified
* add ability to set what is considered as a success during a refer transfer (ie: redirectParticipant API), options: Trying (Notify/100), Ringing (Notify/18x), Connected (Notify/2xx)
* RemoteParticipant: add support for INFO method
* RemoteParticipant: add handling for media_control from RFC 5168
* add support for Session Mode IMs (ie: a session that is only used for IM, not audio or video)
* RemoteParticipant: add method requestKeyframeFromPeer for SIP INFO Media Control keyframe requests
* if there are no conversation profiles defined, still respond to OPTIONS messages (instead of crashing)
* allow ServerAuthManager to be disabled to save on lookups and logic when recon server is not required to challenge inbound requests at all
* onParticipantTerminated callback extended to report a reason, which includes the reason header text from a received BYE message (if present)
* onParticipantDestroyed callback extended to indicate the type of participant that was destroyed
* sipXtapi: FlowManagerSipXSocket: fix returns on write methods.  sipXtapi expects num bytes written as the return, not 0, 0 looks like a failure to sipXtapi
* sipXtapi: fix for intermittent crash on sipX remote participant destruction 
* sipXtapi: SipXRemoteParticipant::adjustRTPStreams: fix bug in mediaDirection logic
* sipXtapi: new createSharedMediaInterfaceConversation API to allow move advanced mixing scenarios
* sipXtapi: reworked DTMF and Play finished notifications from sipX to be more efficient - no need to walk conversations to find participant now
* sipXtapi: fix a bug with PLAY_FINISHED notification when in sipXConversationMediaInterfaceMode
* sipXtapi: add new UserAgentMasterProfile setting to allow disabling of DTMF digit logging for security purposes, ie: credit card entry
* sipXtapi: add ability to specify the following parameters on the recording Uri: ;append, ;duration=<ms>, ;silencetime=<ms>
* sipXtapi: added ConversationManager::onParticipantVoiceActivity virtual callback
* sipXtapi: add support to recon to control recording file format
* sipXtapi: added enableExtraPlayAndRecordResources setting to allow a 2nd File player and 2nd recorder per media interface
* sipXtapi: add ability for recon application to create a media participant that plays audio from an audio buffer
* sipXtapi: add ability to record to a CircularBuffer as opposed to a file on the filesystem (can be used to integrate with a speech recognition system)
* sipXtapi: add support for 2 channel recording
* sipXtapi: modify requirement to specify double slash on local file uri's, allows proper access to playing back fileshare URIs
* sipXtapi: support sipXtapi media resource playback from X milliseconds into file/buffer
* MOHParkServer bug fix:  if sip: is already provided on URI from configuration, don't re-add

### apps
* reConServer: MyConversationManager: add support for multiple rooms, identified by user-part in the URI
* reConServer: add AMQP support with Qpid Proton
* repro, reTurn, apps/reConServer, apps/registration-agent: add config option LogMessageStructure for JSON CEE
* repro, reTurn, apps/reConServer, apps/registration-agent: add config option LoggingInstanceName


## 1.12 Changes
* added Visual Studio 2017 Solutions and Project files
* added OpenSSL 1.1.1 Support
* added LibreSSL Support
* added new reConServer application - B2BUA using recon 
* rutil: fix bug in SHA1 implementation
* resip/stack: Uri: Fix potential out of bounds read
* resip/stack: SdpContents: Reject malformed IP Address
* rutil: ares: Fix heap overflow in ares_mkquery like c-ares does
* resip/stack: Fix: Adding too many media connections may lead to memory exhaustion.
* rutil: DnsUtil: Fix potential out of bounds access
* rutil: cares use only: Missing handling of timers can cause 100% load in dns-thread if no answer is received from server
* resip/dum: added support for reception of UPDATE (no SDP / target refresh) requests in early UAS dialog states
* resip/dum: be tolerant of illegal subsequent SDP's in UAC PRACK flows as recommended by RFC6337 Section 3.1.1 and 3.1.2
* resip/stack: ensure the buffered producer state machine fifo in the transports is flushed when operations are completed, so that getNextProcessTime API's will behave correctly
* resip/stack: make DNS greylist duration configurable
* repro: expose a new config setting in repro for DNS greylist duration 
* resip/dum: MasterProfile: added a removeSupportedMethod to remove a specific SIP method from thos available to profile.
* resip/stack: fix: if IP Address routing fails over UDP and also fails over TCP, such that both UDP and TCP tuples are greylisted, then revert to using UDP (instead of TCP) until the greylisted entries expire
* resip/dum: Added BaseUsage::postDum convenience method
* resip/dum: modified DialogUsageManager::requestMergedRequestRemoval to not bother queuing a delayed merge request removal if the stack is shutting down.  Helps to be less sensitive the order of object deletion at app exit time
* rutil: ParseBuffer: Do not read past the buffer.
* resip/stack: Security.cxx: Add support for ip address in subjectAltName
* resip/dum: fix crash in ServerSubscription::shouldDestroyAfterSendingFailure
* repro/resip/stack: fix an uncaught exception when loading certificates that can cause repro no to start is_directory can throw.  This can happen when interrogating a broken link in linux.
* repro: fix presence server so that NOTIFY messages are not sent out for publication refreshes that don't update the presence document
* resip/stack: fix for WSS transports when reading large messages
* resip/stack: fix for potential crash if Connection::processReads deletes the connection
* resip/stack: fix NameAddr operator== so that an AllContact NameAddr is NOT equal to an empty NameAddr
* rutil: Fix XMLCursor memory leak when ctor throws
* repro: add new repro setting (AllowInboundFlowTokensForNonDirectClients) that works in-conjunction with RRTokenHack / EnableFlowTokens setting that allows inbound flow tokens to be used for connections that come from non-directly connected clients
* resip/stack: fixed an issue where the stack could crash if a TCP based sender sends a Content-Length that is less that the actual SIP body transmitted 
* return: update ASIO drop from version 1.10.6 to 1.12.1
* resip/dum/TlsPeerAuthManager and repro/monkeys/CertificateAuthenticator: add more debug logging
* resip/dum repro: certificate authentication: async DB lookups 
* resip/stack/repro: More flexible domain matching is now supported 
* resip/stack: TuSelector: allow TUs to be inserted at front of list
* resip/stack: Added ability to add non standard child elements to the Dialog package document Dialog element.
* repro: update for qpid-prton 0.22.0


## 1.11 (Beta) Changes
* resip/stack: implement  http://tools.ietf.org/html/rfc3261#section-18.1.1, paragraph 8 (Server error conditions)
* resip/stack: added TransactionUser::removeDomain API
* rutil: fixed issue with DNS logic if we switch from DNS over UDP to DNS over TCP, which can occur when the DNS result is greater than 512 bytes
* resip/stack: fixed up RFC4373 requires parameter - it is an exists parameter and not a Data parameter
* resip/stack: added the ability to retrieve a list of simple presence info from a GenericPidfContents
* rutil: improve the log output of DNS records
* resip/recon: make sure we look at rinstance header, if it is present, when looking up the incoming conversation profile
* resip/recon: added logic to detect if a TCP connection is terminated.  This makes TCP registration errors detected much faster.
* recon/MOHServer: multi-tenancy support.  Allow registering more than one MOH AOR with it's own music file
* recon/MOHServer: added certificate path setting
* repro: added route order to key, so that you can have multiple routes with the same matching pattern and different destinations, as long as they use a different order value
* resip/stack: added a reloadDnsServers API to SipStack
* repro: added a Reload DNS Servers button to repro settings GUI
* resip/stack: added ECONNABORTED to Windows TCP connection timeout processing for WSAPoll implementation
* repro: changes to allow repro to run with outbound support behind a NAT device.  Use configured transport specific record-routes in all cases
* resip/stack: update TlsConnection.cxx to add TLS SNI extension in Client Hello
* resip/dum: added new method to ClientRegistration class so that applications can check if a registration operation is currently in progress or not
* resip/recon: send presence mechanism through PUBLISH message
* resip/recon: send/receive text message with MESSAGE request
* return/resip/repro: Use SSLv23 instead of TLSv1 when using OpenSSL for better TLS version capability 
* resip/dum: added ability for DialogUsage's (ie: InviteSessionHandle) to query the route set from the stored Dialog Info
* resip/dum: added missing UAC_SentUpdateEarlyGlare state to the InviteSession::isEarly implementation
* repro: added support for HOMER / HEP
* resip/stack: added the ability to update QOS to a new value on existing stack managed sockets, without needing to restart the SipStack.  Note:  DNS sockets are not currently supported
* resip/stack: fixed bug with SipStack::removeTransport method - transport selector maps could get out of sync and cause existing transports to not get used correctly after a runtime removal
* resip/stack: added DialogInfoContents for handling of dialog-info+xml bodies
* resip/dum: fixes to DUM's DialogEventStateManager implementation
  * fixed a bug in DialogEventStateManager that could tag as call as replaced in a dialog info event, even though it will get denied because it is in the incorrect state
  * fixed up DialogEventStateManager so that the onConfirmed callback will occur for any mid-dialog SDP changes
  * added accessor to DialogEventStateManager to be able return the DialogStateInfos for a particular Uri, instead of returning all DialogInfo's in memory
  * added missing DialogEventStateManager onTerminated callback for when we reject a UAS call
* reflow resip/recon: add ForceCOMedia for NAT users / comedia behavior
* repro: only bind HTTP and command ports to localhost by default
* resip/recon: ensure G.722 sample rate is advertised as 8000 in local capabilities
* resip/recon: warn about peers offering G.722 with sample rate other than 8000
* resip repro: add AssumeFirstHopSupportsFlowTokens option
* resip/stack resip/dum: InteropHelper: add option to enable/disable adding rport to Via
* repro: add config option AddViaRport
* resip/stack: fix for TCP timeout handling, ensure TransportFailure code is used so that the next DNS entry will be tried
* resip/stack: removed use of USE_DNS_VIP preprocessor define and use a runtime setting instead. Runtime setting is a new parameter passed into one of the two SipStack constructors.
* resip/recon: UserAgent: add method getConversationProfileByMediaAddress
* rutil: GenericIPAddress: implement stream operator
* reTurn: StunTuple: add method getSockaddr
* reflow: add RTCPEventLoggingHandler API
* rutil: OpenSSLInit: fix code to free compression methods
* resip/dum: UserProfile: log a warning when no digest credentials available
* ensure 503 and 408 blacklist and greylist logic is working correctly for Client Invite transactions on unreliable transports
* repro: RegSyncServer: send reg sync XML messages to AMQP topic with Qpid Proton
* resip/stack: DnsResult: add some debug messages when transport not supported
* repro: make sure we create a DiaglogUsageManager if PresenseServer is enabled, could crash on startup otherwise
* repro: RFC3326 (The Reason Header Field) Support in repro
  * When a forked call is answered by one leg the other legs are canceled.  These cancel messages will now go out with a Reason header on them:  Reason: SIP;cause=200;text=Call completed elsewhere
  * Repro will copy reason headers from Cancel message it receives from the caller to the Cancel messages that are sent to each leg.
  * If one of the call legs returns a 6xx global response code, then the other call legs are canceled.  This CANCEL message will contain a reason header with the 6xx cause code and reason text in the reason header.
  * resip/dum: fix to ensure Privacy headers don't grow with each mid-dialog re-invite (if present)
* repro / reTurn: Use systemd to restart if process stops unexpectedly


## 1.10.2 Changes
* changes to support Windows 10 IOT ARM (RaspPi2)
* VS2015 build support
* cleanup some warnings
* rutil: fix ARM / GCC 6 build issue with signed char[]
* resip/stack: fix issue with ACK/non-200 transport selection for client transactions over WSS transports
* repro: SQL backends: singleResultQuery: distinguish error from empty result set
* repro: fix for OutboundTargets added by LocationServer - first parameter should be an AOR and not an instance Id
* resip/dum: added convenience API's to DialogUsageManager for ending all ServerSubsciptions and ServerPublications to aid in graceful DUM shutdown
* repro: fix for OutboundTargets added by LocationServer - first parameter should be an AOR and not an instance Id
* resip/dum: fixed issue with DUM ServerInviteSession not automatically re-issuing an INVITE after received a 491. Also added tfdum test.
* resip/stack: fix InternalTransport::bind to work when binding to port 0 when using an IPv6 interface
* repro: update drop of GeoIP library to latest 1.6.9 version
* return/recon: update ASIO drop from 1.10.1 to 1.10.6
* resip/dum: ServerSubscription: fix reason text in responses


## 1.10.1 Changes
* resip/stack: TLS: make the path argument of the Security class constructor more versatile
* rutil, dum: RADIUS: support for radcli instead of freeradius-client
* build: add more Visual Studio artifacts


## 1.10.0 Changes
* repro: added a full presence server
* resip/stack: WebSocket authentication may now use URL parameters instead of cookies
* repro: added support for PostgreSQL and indexed database definitions
* repro: can include multiple files into repro.config using Include
* resip/stack: TLS: perfect forward secrecy (PFS) support using DH or ECDH
* resip/stack: TLS compatibility and cipher strength improvements
* resip/stack: TLS: support for passphrases used to encrypt private key files for domain certificates
* rutil: better Syslog support (now sets severity correctly)
* rutil: allow customization of assert() behavior using resip_assert(), improves diagnosis of crashes
* resip/stack: added Poll(WSAPoll) implementation for event loop - allows Windows to support thousands of TCP connections instead of only ~60 (FD_SETSIZE)
* resip/stack: added ability to remove transports at runtime
* resip/stack: added netns (linux network namespace) support to stack


## 1.9.11 Changes
* resip/stack: change TLS cipher suites to use -COMPLEMENTOFDEFAULT
* repro: change TLS cipher suites to use -COMPLEMENTOFDEFAULT
* repro: CommonNameMappings: detect repeated common names in config file
* resip/stack: fix buffer management bug in WebSocket transport
* rutil: XMLCursor: support whitespace before first XML element
* resip/stack: Helper: fix for an auth validation issue
* reTurn: add psql-user-extract script
* resip/dum: DialogUsage: added logic to avoid trapping if mAppDialogSet is 0
* resip/dum: DialogSet: Added handling for non-invite dialogs
* resip/stack: ConnectionBase: increase max headers from 256 to 1024
* rutil/dns/ares: fix for uninitialized pointer
* rutil/dns/ares: remove cache-sensitive logic from caching algorithm
* rutil/stun: Qualify bind to prevent clang from using std::bind
* resip/stack/SdpContents: add some static codec definitions and new methods


## 1.9.10 Changes
* resip/stack: support for OpenSSL versions that don't have all option names
* resip/stack: lower severity of some log messages when TLS connections close
* resip/stack: make ensureWriteable more tolerant during TLS handshakes, resolves crashes
* resip/stack: add support for CA root cert bundle files
* resip/stack: use StrongestSuite as default cipher list
* resip/dum: challenge with correct realm when from and req URI domains don't match
* rpm: update Wants and After details in unit files


## 1.9.9 Changes
* resip/stack: only support SSL_OP_SAFARI_ECDHE_ECDSA_BUG as configuration string with compatible OpenSSL versions


## 1.9.8 Changes
* resip/stack: add static fields on BaseSecurity for setting OpenSSL CTX options
* resip/stack: explicitly disable SSLv2 and SSLv3 by default when using SSLv23
* resip/stack: allow application to provide OpenSSL certificate verification callback
* repro: change default OpenSSL method from TLSv1_method to SSLv23_method (enables TLS v1.1 and v1.2)
* repro: add configuration options OpenSSLCTXSetOptions and OpenSSLCTXClearOptions to control OpenSSL CTX options
* repro: add configuration OpenSSLCipherList to manually configure the OpenSSL cipher list


## 1.9.7 Changes
* rpm: Indicate that b2bua package is obsolete
* resip/stack: TlsConnection: correctly handle OpenSSL error queue after SSL_read
* resip/stack: fix for forced target routing in TransactionState - thanks to John Gregg for this
* rutil: enhance command line parsing exceptions with additional details - thanks to John Gregg for initial contribution
* rutil: if a v6 address is passed to the GenericIPAddress sockaddr constructor, ensure the entire v6 address is copied over properly.  Note:  sockaddr is smaller than sockaddr_in6 - Thanks to Bjorn A for this fix
* rutil: accept case insensitive log level strings
* resip/dum: fix obscure bug in basicClient where if we don't use a threaded stack processing does not function correctly - required PollGrp to be passed to SipStack constructor
* resip/stack: fix for a use-after-free bug when adding multi-headers to a list that has been copied, and then one of the headers is accessed (but not necessarily parsed) - thanks to Byron Campen for this fix!
* resip/stack: Added accessor for TransactionUser FIFO so to obtain stats
* resip/stack: additional OpenSSL cleanup fn - reordered functions to match order used in this post: http://openssl.6102.n7.nabble.com/Cleanup-procedure-missing-some-calls-td37441.html
* reTurn: modified asio and boost include file ordering to avoid multiply defined symbol errors on linking
* resip/recon: remove extra sleep definitions
* resip/stack: fix a bug with Keepalive processing that causes transaction state leakage and potential traps
* rutil: syslog: use LOG_PID, avoid sending redundant data in log string.


## 1.9.6 Changes
* resip/recon: fixes a bug that makes it impossible to use codecs with RTP clock rates other than 8000, makes Opus support possible
* autotools: minor tweaks to reduce warnings during build/linking


## 1.9.5 Changes
* resip/recon: do not send DTMF as inband audio, can be troublesome as sipXtapi echoes it back to caller (note: requires latest sipXtapi code with startChannelTone patch)
* resip/recon: SipXHelper: map log entries from sipXtapi to reSIProcate logger
* resip/recon: safety checks when using dynamic_cast with ConversationProfile
* resip/recon: avoid changing payload IDs in the default ConversationProfile


## 1.9.4 Changes
* resip/recon: ensure libraries link with sipXtapi libs
* resip/recon: fix for case where DialogSet Profile is not an instance of ConversationProfile
* resip/recon: per-participant tone support
* resip/recon: clarify DTMF API: tones represented by event code, API-user durations in milliseconds


## 1.9.3 Changes
* reTurn: TurnAsyncSocket: fix for sending when not using TURN
* resip/stack: added allowed events to 489 errors
* resip/stack: Support for dtmf-relay content parsing for SIP INFO messages
* resip/recon: call onDtmfEvent when SIP INFO received, just as for RFC2833 DTMF
* resip/recon: call acceptNIT()/rejectNIT() after handling INFO message
* resip/recon: support for changing internal sample rate of sipXtapi
* resip/recon: allow UserProfile and extension headers to be specified when creating a new remote participant


## 1.9.2 Changes
* rutil/dns: fix for Windows DNS Server use.  Ignore Link Local DNS Server entries.
* rutil: add extra ParseBuffer constructor for const char*
* rutil: Data: ust strncmp in comparisons with const char*


## 1.9.1 Changes
* adapt to work with asio v1.10.1 (still works with v1.4.x too)
* fix for spinning in DnsThread
* ensure mPollGrp is set before accessing it in AresDnsPollItem::resetPollGrp
* fix RegSyncClient so that it detects breaks in the socket connection to the server - by using keepalives
* made EGAIN and EWOULDBLOCK processing the same in a few additional spots
* add WinLeakCheck header to some new Websocket class files to aid in leak detection on windows
* stop trying to write cert files in various places
* repro:
* repro/WebAdmin: catch and log exceptions when starting fix issue with CommandServerList being deleted when not newed if startup fails due to transport exception fixed up StackStatistics handling for when multiple command servers are running (ie: v4 and v6 CommandServers)
* repro: ensure we use the ServerText config setting to set the server text headers in requests that traverse repro
* use threadsafe variant of localtime (localtime_r) on non-windows based platforms


## 1.9.0 Changes

### Highlights
* Session/registration accounting
* SIP over WebSocket support for WebRTC, with cookie authentication option
* Python scripting for repro routes
* repro dynamically-loaded routing plugins
* UAS PRACK
* Improvements to daemon processes for UNIX/Linux users
* Android support
* Multiple users in reTurn
* Many stability/bug fixes
 
### General Changes
* added CAJUN (header file only library) source to contrib for JSON encoding/decoding support - now also a Debian/Ubuntu/Fedora package
* added last release of GPLed opensigcomp project to contrib tree, since it can't be found on internet any longer
* updated libSRTP drop to latest 1.4.4 version
* updated to asio 1.4.8

### Build system
* VS2010 project file enhancements
  * Modified project settings to allow x86 and x64 builds in the same source tree
  * Output Directory for all projects and for all configuration/platform combinations changed to $(SolutionDir)$(Platform)\$(Configuration)\
  * Intermediate Directory for all projects changed to $(Platform)\$(Configuration)\
  * Added _CRT_SECURE_NO_WARNINGS to projects where it was still missing in order to reduce the amount of warnings. There are less than 30 warnings now, so one can concentrate on those warnings that might really matter instead of just ignoring a large warning list
  * Removed Manifest Tool Command Line additional option "validate_manifest" for [repro-x64-Debug-SSL]. This switch makes the MT.exe only parse the input but does not generate output and can lead to a build error. 
  * Removed all Property Pages “Upgrade from VC 7.1”. The sole purpose of this is to introduce the preprocessor directive "_VC80_UPGRADE=0x0710" which has no practical use at all
  * There’s a bug in Visual Studio 2010 when Visual Studio 2012 is installed also. As a result, the Setup projects detect a dependency on the .NET Framework for a C++ project even if it does not use .NET. It can be cured by adding “<AddAdditionalExplicitAssemblyReferences>false</AddAdditionalExplicitAssemblyReferences>” to the project file. After doing so I could remove the incorrectly added LaunchCondition in the setup projects. Details can be found here: http://connect.microsoft.com/VisualStudio/feedback/details/746328/after-installing-vs-11-rc-vs-10-setup-projects-require-the-net-framework 
  * Added a new file VisualStudioReadme.htm with some information about the Visual Studio build
* add 64-bit VS2010 project settings for reTurn
* added configure-android.sh for building with Android NDK
* rend: added autotools support
* rpm: various fixes for RHEL6 and Fedora builds

### rutil
* avoiding truncating the log file
* add directory create API to rutil/FileSystem
* WIN32 - added better logging to WinCompat::determineSourceInterfaceWithoutIPv6
* Data: add fromHex method
* Data: import capacity of pre-allocated buffers
* Data: improve buffer size allocation in base64encode
* dns/ares: add support for DNS server discovery on Android
* add support for Android logging / logcat utility
* add FileSystem::Exception, check stat() return value
* FileSystem: check results of readdir calls
* Add support for FreeRADIUS client (radiusclient-ng is deprecated but still supported for now)
* Tuple: provide method for copying out sockaddr
* ConfigParse: add support for reading sets
* ConfigParse: give precedence to options on command line
* ConfigParse: reject duplicate values from any given config source
* ConfigParse: throw exceptions rather than immediately exiting on error
* added preprocessor conditional compile option for Windows source interface selection to avoid using adapter addresses tagged as Transient.  Transient addresses include those that are used for Virtual IP addresses and tagged with SkipAsSource (also used in Windows Clustering solutions)
* added Sha1 implementation for when OpenSSL is not available
* ServerProcess: change logfile and PID file ownership when dropping privileges
* WIN32 - WinCompat::determineSourceInterfaceWithoutIPv6: throw transport exception if new fails or if second call to GetIpAddrTable fails
  * have seen one case where 2nd call to GetIpAddrTable returned 232 (ERROR_NO_DATA) for some unexplained reason, and Contact and Via headers would end up getting populated with 0.0.0.0 address
* WIN32 - WinCompat: make sure all IPHLPAPI fns are run from the runtime loaded library
* WIN32 - WinCompat: move static Mutex outside of WinCompat::instance() fn definition to avoid race conditions
* MD5 code: more checks for big endian environments (gcc __BYTE_ORDER__)
* Data::sizeEqualCaseInsensitiveTokenCompare: Fixing a goof that was causing later code to operate on unaligned data as if it were aligned, which was causing the code to malfunction on many non-x86 platforms.
* Android cross-compile: fix compile issues
* fix Data for big endian builds
* DnsUtil - handle potential truncation in gethostname()
* fix for build on GNU HURD
* DnsUtil: support HURD net device interface names with leading slash
* FileSystem: add support for full filename in iterator, fix problem with using stat() on relative path
* fixed up AresDns::checkDnsChange to be able to compare IPv6 DNS servers instead of just V4 servers
* WIN32 - ares_init: modified API used to retrieve DNS servers for Windows systems from GetNetworkParams to GetAdapterAddresses (supported on Windows XP / Server 2003 and above)
  * will fallback to GetNetworkParams if unable to retrieve servers via GetAdapterAddresses (this will happen on Windows 98 and Server 2000 machines)
  * GetNetworkParams issues: 
    * does not support IPv6
    * was exhibiting a MS bug that could cause 0 DNS servers to be returned if many ares instances where init'd at the same time (ie: could happen when creating many SipStack instances at the same time)
  * more efficient lookup when ARES_FLAG_TRY_NEXT_SERVER_ON_RCODE3 ares option is used - no need to lookup the physical address of each DNS server separately 
* RADIUS: correctly distinguish access denied from server error
* RADIUS: don't behave badly if multiple calls to init() are made

### stack
* added WS (WebSocket) and WSS (Secure WebSocket) transport support for WebRTC compatibility 
  * added WebSocket handshake cookie parsing and validation
* added build-in "cause" parameter parsing for Reason header (RFC3326) parsing
* added missing header parameters for RFC4244 and RFC4244-bis (index, rc, mp, and np)
* define max message size in a variable, allow compile time default with RESIP_SIP_MSG_MAX_BYTES and runtime config setting
* SipMessage: range check on addHeader
* added a MD5 hash comparison equals operator to Content class - needed by upcoming UAS Prack implementation
* TcpBaseTransport: ensure outgoing TCP-based connections originate from configured interface address
* DTLS support for certificate chains
* TLS: more verbose certificate errors
* add support for more aggressive garbage collection, MinimumGcHeadroom
* added option to control if the DNS resolver will allow sip:<ip-address> to resolve to UDP, TCP, or TLS, in that order of priority (existing behaviour) - vs - UDP only
* Helper::getClientPublicAddress and isClientBehindNAT - don't create a Tuple out of a hostname if it isn't an IP address - can trap
* don't assert in Tuple constructors that take printable addresses, since these addresses can come from the wire or from configuration
* SigComp - fix memory leak in ConnectionBase: use delete[] for arrays
* ensure we can send a 100 Trying for NonInviteTransaction retransmissions after one has already been sent - previously an assert would be triggered
* Fixed issues with pre-mature TCP socket teardown: treat EGAIN and EWOULDBLOCK the same
* fixed incorrect placement of #ifndef RESIP_USE_STIL_STREAMS
* If TimerCleanup fires before Timer D for a cancelled ClientInvite transaction then we try to log out mNextTransmission regardless of the transaction state.  However the transaction state may have advanced to the Completed state and mNextTransmission will be null, so we shouldn't be trying to log it
* Tuple: fix mask comparison for some platforms with 64 bit long
* remove use of RSA_generate_key for newer OpenSSL versions
* Android cross-compile: fix compile issues / fcntl.h
* fix an issue with TCP connections on Windows when TCP socket buffer fills up.  Connection would have been terminated before - we were not properly handling WSAWOULDBLOCK response.  We were relying on errno / getErrno to be accurate a number of calls after the write failure.
* fix support for intermediate certificate chains
* fix some potential mem leaks found by coverity in DtlsTransport::_read

### dum
* Added full support for PRACK - previously only supported UAC PRACK, UAS PRACK Support is added in this release
* added in support for UAC Prack to be able to send an offer in the first PRACK response (must call provideOffer from onAnswer callback)
* TlsPeerAuthManager: add support for statically configured mappings of TLS common names to permitted From: addresses
* ServerAuthManager: add new option to not challenge third parties ("From" header not a local domain)
* add additional accessor to ClientPagerMessage to retrieve the stored request message as a SharedPtr instead of a reference.  This allows the caller to adorn the message and then call DialogUsageManager::send safely (if queueing and/or message bodies are not desired when sending a SIP MESSAGE request)
* resip ClientPagerMessage - modified to allow onSuccess and onFailure callbacks even if queue of message contents is not used - allows you to call ClientPagerMessage::getMessageRequest, build the request yourself (even one without a body), send using DUM::send and still have your callbacks invoked
* Added ServerSubscriptionHandler::onNotifyAccepted callback.
* Profile: allow AllowEvents to be an advertised capability
* remove randomized addition of time to stale call timer - not sure why this was there in the first place
* RADIUSServerAuthManager: allow RADIUS client configuration file to be specified
* Deprecated addTransport on DialogUsageManager object:  SipStack has the exact same interface
* dum/test/basicClient - use stack in multi-threaded mode
* dum/test/basicClient - use EventStackThread instead of InterruptableStackThread - enables Epoll support on platforms that support it
* dum/test/basicClient - enabled UAS PRACK
* add support for statically configured auth realm
* InviteSession: fixed recursive calls to refer (w/ sessionToReplace) when using referCommand and InviteSessionReferExCommand.
* TlsPeerAuthManager: explicitly skip authorization logic if TLS mode is optional or transport is not TLS
* ClientRegistration: modified whenExpires to return the number of seconds when the registration will actually expire, as opposed to the number seconds until our next refresh message is supposed to go out
* ClientRegistration: ensure that we don't try to unregister if ClientRegistration::end is called and we never successfully registered (or all current registrations are currently expired)
*  also includes case where we need to wait for an operation to finish before we can end 
* ClientRegistration: no-op subsequent calls to ClientRegistration::end after initial call
* fixed issue with over active 1xx retransmissions
* ClientRegistration: If we are unregistering all contacts as the initial registration message then don't bother storing in mMyContacts - allows add Binding to be called later to add a registration
* fixed update glare handling for UAC prack
* fixed missing break in ClientInviteSession::dispatchReceivedUpdateEarly 
* RADIUSServerAuthManager: cease mangling usernames passed to RADIUS
* Security: stop trying to write PEM file in addDomainCertPEM
* dum/test/basicClient - ensure shutdown works if initial registration attempt is in progress
* fixed some transaction state memory leaks in DUM - resulting from race conditions in DUM Cancel handling
  * Race#1 - DialogSet.cxx - if Dialogset is still around, but dialog is gone (potentially BYE'd).  Need to respond to CANCEL in order for transaction in stack to go away
  * Race#2 - ServerAuthManager - if we are waiting for an async user auth info to arrive on an INVITE with credentials when a CANCEL arrives, we did not correctly respond to the CANCEL
* DialogUsageManager - needed a change to handle the User Auth info arriving after destroying the feature chain due to a CANCEL 

### repro
* added WS (WebSocket) and WSS (Secure WebSocket) transport support for WebRTC compatibility 
  * add specific support for authenticating users based on HTTP cookies authenticated by a shared secret/HMAC scheme
* Add registration and call/session accounting
  * configured/enabled via configuration file
  * Session Accounting - When enabled resiprocate will push a JSON formatted events for sip session related messaging that the proxy receives, to a persistent message queue that uses berkeleydb backed storage.
  * The following call events are logged:
    * SessionCreated - INVITE passing authentiation was received
    * SessionRouted - received INVITE was forward to a target
    * SessionRedirected - session was 3xx redirected or REFERed
    * SessionEstablished - there was 2xx answer to an INVITE (only generate for first 2xx)
    * SessionCancelled - CANCEL was received
    * SessionEnded - BYE was received from either end
   * SessionError - a 4xx, 5xx, or 6xx response was sent to invitor
  * Registration Accounting - When enabled resiprocate will push a JSON formatted events for every registration, re-registration, and unregistration message received to a persistent message queue that uses berkeleydb backed storage.
  * The following registration events are logged:
    * RegistrationAdded - initial registration received
    * RegistrationRefreshed - registration refresh received / re-register
    * RegistrationRemoved - registration removed by client / un-register
    * RegistrationRemovedAll - all contacts registration remove / unregister received
  * Consuming Accounting Events: Users must ensure that the message queues are consumed, or they will grow without bound.  A queuetostream consumer process is provided, that will consume the events from the message queue and stream them to stdout.  This output stream can be consumed by linux scripting tools and converted to database records or some other relevant representation of the data.  
* add plugin DSO support (non-Windows platforms only)
* add support for using Python scripts to perform routing
* made static instance of the GeoIP library
  * allows static geoIPLookup method for other components of repro to access geoip library
  * reduces memory in cases where multiple instances of GeoProximityTargetSorter are needed (take care if creating multiple instances since static initialization is not mutexed)
* print MaxMind GeoIP database information to resip logs
* StaticRoute: allow routing of messages when CertificateAuthenticator passed
* CertificateAuthenticator: add support for statically configured mappings of TLS common names to permitted From: addresses
  * add CommonNameMappings config option to load CN mappings from a text file
* Added options to disable DIGEST challenge of third parties in DUM auth when mutual TLS in use
* added support for WebAdmin and CommandServer binding to specific IP addresses
* enable IPv6 by default
* allow specification of arbitrary domain cert/key PEM files
* make the CertificatePath optional with no default
* WIN32 -delay load libmysql.dll so that it is not required unless MySQL is enabled in repro 
* config: support non-consecutive transport numbering in advanced transport spec
* move authenticator creation code into a common factory class
* add TlsTrustedPeers config option
* send an alert to syslog when config parsing fails
* configure User-Agent header for registrar, set default value with PACKAGE_VERSION
* add TCPConnectionGCAge config option for garbage collection
* add support for TCPMinimumGCHeadroom
* add RADIUS support
* add config option ChallengeThirdPartiesCallingLocalDomains
* add StaticRealm config option to always challenge using a specified realm value
* reset logger / rotate log files when HUP received
* added ability to change log level from web admin
* take HttpAdminPassword out of repro.config, use a htdigest file
* rpm: configure repro and reTurn not to fork under systemd in Fedora
* move TLS client auth feature/monkey after DIGEST, so that TLS client auth can know if the peer passed DIGEST
* CertificateAuthenticator: explicitly skip authorization logic if TLS mode is optional or transport is not TLS
* fixed CommandServer being created even though listener bind failed
* fix bug that causes assert if you try to configure repro registrar and authentication disabled 
* don't strip authorization headers if the request is going to spiral / loop back to us, only strip if routing outside our domain 
* if we receive a response that has messed with the vias and we don't have a stored bestResponse then send a 500 
* make sure multiple RegSync Servers can be added and dispatched to -fixes repro bug when starting both IPv4 and IPv6 regsync servers
* fixup reprocmd so that it works properly for commands with arguments ie: SetCongestionTolerances - now requires a '/' in front of the actual command
* respond with an error code when invalid pages requested
 
 
### reTurn
* reTurn Server: improved diagnostics in RequestHandler - print senders tuple with errors
* reTurn Server: detect port in use errors when creating relay
* reTurn Server: TurnAllocation/UdpRelayServer - added flags so that some errors only log a Warning level once, then will log at Debug level after - this helps to avoid flooding logs with Warning level messages
* reTurn Server: track allocations per connection - reduces map sizes and lookups when TCP/TLS client connections are used
* reTurn Server: read user/password data from a file specified by UserDatabaseFile
* reTurn Server: optimization to not calculateHmacKey multiple times
* reTurn Server: if return config file location contains a base path, then append this path to other filename settings if an absolute path isn't already specified
* reTurn Server: short term authentication makes no sense for a TURN server, since credential information needs to be exchanged out of band (ie. as with ICE) - remove short term auth option from return server - Long Term authentication is enabled by default
* reTurn Server: listen on IPv6 as well as IPv4
* reTurn Server: more verbose warning when user or realm unrecognised
* reTurn Server: add support for TLS private key in standalone file
* reTurn Server: periodically check for updates to the user database file
* reTurn Server: support for hashed passwords in user database file
* reTurn Server: add support for configuring software name header in STUN packets
* reTurn Server: reload users.txt and reset logger on HUP signal
* reTurn Client API enhancements 
  * upped receive buffer size from 2048 to 4096
  * OS level Udp socket receive buffer size set to 66560
  * TurnAsyncSocketBase - use dispatch instead of post for send API for increased efficiency
  * TurnAsyncSocket - new client side APIs
    * setLocalPassword for checking integrity of incoming STUN messages
    * connectivityCheck for ICE connectivity checks 
    * setOnBeforeSocketClosedFp for QOS cleanup (windows)
    * send API now split into send(To)Framed and send(To)Unframed
    * Queue of guards modified to use a weak functor template instead
    * onBindFailure and onBindSuccess now return the Tuple that failed or succeeded
    * onChannelBindRequestSent - new
    * onChannelBindSuccess - new
    * onChannelBindFailure - new
  * DataBuffer improvements
  * StunMessage - added ice attributes
* TestRtpLoad change to use OS selected port for relay port instead of hardcoded 2000
* reTurn Server: fix bugs with port allocation logic 
  * properly detect wrap around when end of range is max ushort = 65535
  * properly allocate even or odd port after wrap around from end of range
* TCP/TLS Server classes - ensure we try to accept future connections if we receive an error indicating we ran out of file descriptors
* TLS/UDP Servers classes - throw on errors in constructor
* fixup stunTestVectors.cxx now that ice parameters are parsed
* Client API: fix for requestTimeout - handle case where close is called in handlers and it is invalidated
* stunTestVectors: correct test case for network byte order
* reTurn Server: only try to start TLS transport when port is non-zero

### recon
* modified recon/reflow DtlsSocket to work with DTLS-SRTP from OpenSSL 1.0.1 - patched version of OpenSSL no longer required
* modified DTLS-SRTP fingerprint to be SHA-256 instead of SHA-1 for better web-rtc interop
* recon: look for dynamic codec modules in default location if none are statically linked
* improved error logging in RemoteParticipant::adjustRTPStreams
* reflow: emit warning when socket is not available

### reflow
* reflow: ensure proper initialization of policy structure

### tfm (repro)
* added VS2010 project files
* added ability for tfm repro to be run in interactive mode with a -i command line option
* added ability for tfm repro to be able to disable digest authentication if required for some test cases
* enabled over 60 additional tfm repro test cases that required digest auth to be disabled 
* enabled record routing on repro test instance in tfm repro and tfm dum
* modified tfm/Sequence and tfm/SequenceSet to support newer versions of boost 
* disable Invite loop test since all platforms will fail when record-routing is enabled - it causes  the message size to exceed 8k
* update root CA cert and domain cert used in tfm repro TLS testing - old ones had expired 
* fixup some test cases  testAttendedExtensionToExtensionTransfer, testBlindTransferExtensionToExtensionHangupImmediately, testConferenceConferencorHangup  
* fix for testAck200ReflectedAsInvite test case - reflected INVITE wasn't tracked and it would use first request in map - which would sometime be the wrong invite 

### tfm (dum)
* added VS2010 project files 
* added 24 new tests cases to tfm dum for PRACK testing (dumTests.cxx)
* modified tfm dum TestSipEndPoint::Prack to be able to send PRACKS from TestSipEndpoint
* modified tfm dum TestSipEndPoint::Invite to be able to enable PRACK on outbound invites 
* modified tfm dum to be able to store relrespinfo on 18xrel for generating PRACKs
* added interactive mode (-i) flag to tfmdum so that tests can be run either automated or with a test selector menu 
* fixed up TFM DUM test cases that required digest to be disabled and ones that required record-routing to be turned on, and ones that were looking for rinstance parameter (instance id is now used) 

### nICEr
* Fix some 32/64 issues and an edge case where no ICE attributes are provided
* Numerous improvements and fixes from Mozilla WebRTC project