# Change Log

## 1.14.0 Changes

### General/Build
* Update local ASIO drop from v1.18.1 to v1.36.0, with compatibility updates for Asio 1.34 and later
* Update Windows CMake builds from OpenSSL 1.1.1q (zeroc nuget package) to OpenSSL 3.5.5 (openssl-native nuget package)
* Resolve /w3 and SDL compilation errors/warnings on Visual Studio 2022 builds
* Fix MSVC compile warning C4819
* Add C++20 compatibility fixes
  * Fix potential ambiguity of the comparison operator for BranchParameter
  * Fix deprecated implicit this captures and dangling pointer captures
  * Fix warnings about operations on enums of different types
  * Remove operations on volatile variables
  * Fix weak_bind constructor definition
* Fix GCC compiler warning: ISO C++ forbids converting a string constant to 'char*'
* Fix usage of `<cctype>` functions
* Remove custom definition of strncasecmp
* Fix test crashes in release builds
* Remove NDEBUG for tests to improve test reliability
* Expose `config.h` to library users
* Fix git command execution error when the build directory is not under the source directory
* Add OpenHarmony (ohos) compile documentation

### Security
* Use CSPRNG for security-sensitive SIP/STUN/TURN tokens (CWE-338)
  * Use crypto RNG when generating certificates and nonce
  * Use crypto RNG to generate various IDs that are supposed to be unique
  * Improve transaction ID uniqueness
* Authenticate STUN Binding responses (CWE-345)
* Add Digest Authentication Support for SHA256 and SHA512/256 algorithms
* Add new DigestStream class, capable of MD5, SHA1, SHA256, SHA512 and SHA512/256 hash algorithms
* TLS handshake fixes
  * Avoid using undefined client verification mode in TLS connection
  * Disable TLS compression by default
  * Correct TLS logging
* Allow applications to get TLS connection/handshake errors
* Propagate SSL Certificate Validation errors to ConnectionTerminated event/message
* Cleanup SecurityAttributes
* WinSecurity: expand constructors to allow setting of defaultPrivateKeyPassPhrase and DHParamsFilename
* WinSecurity: perform normal preload process so that domain certificates can be loaded

### DNS
* DNS Cleanup and Fixes
  * Various DNS crash fixes
  * resip-ares DNS fixes
* Expose already existing possibility to change the DNS servers via the DnsStub API
* Expose `ARES_FLAG_NOCHECKRESP` ares flag in resip — can be used to force ares to use the hosts file as fallback when DNS lookup fails

### rutil
* Data class: add `prefixNoCase` and `postfixNoCase` methods, with unit tests
* Mark `Data` move constructor, assignment, and `takeBuf` as noexcept
* Fix `Data::npos` incompatibility with `std::string` and `string_view`
* rutil/Log: Selectively enable FQDN resolution upon Logger initialization
* Add option to enable monotonic clock (`_RESIP_MONOTONIC_CLOCK`)
* StatisticsManager: when a new poll interval is set, recalculate nNextPoll immediately so the new interval takes effect without waiting for the previous poll time to pass
* Log filename in exception in `Data::fromFile`
* Reduce log level of stats messages

### stack
* Add SIP Diversion header (RFC 5806) support
* Add `Tuple::isSpecialPurposeAddress()` per RFC 6890
* Add support for iteration over SIP headers
* Add new parameters and parsing for RFC 8224 Identity headers
  * Add IdentityCategory header tests
  * `Helper::getResponseCodeReason`: add SIP response code reasons for RFC 8224 defined responses
* Add Digest Authentication Support for SHA256 and SHA512/256 (RFC 7616)
* Symmetric Connection support
  * New transport flag: `RESIP_TRANSPORT_FLAG_SYMMETRIC_CONNECTIONS`
  * Use `SO_REUSEPORT` on Linux for symmetric connections
* Expose `Helper::getSdpRecurse` for use cases where modifying SdpContents is needed
* Ensure reason-text complies with RFC 3326 formatting
* `resip/stack`: consider known parameters when comparing URIs
* Add `getByIndex` method to `ParserContainer`
* `resip/stack`: downgrade duplicate parameter-name log to INFO level
* Fix failure ACK routing over TLS when stack has multiple TLS transports
* Make RRDecorator logic safer
* Refine ConnectionTerminated error notifications
* `ConnectionManager::gc`: safeguard against underflows in flowtimer connection cleanup logic
* Protect against time calculation underflows in `ConnectionManager::gc`
* Fix NameAddr parsing bug when angle brackets are included in quoted parameters
* Log message parsing context in case of SIP message parsing errors
* Log transaction state when receiving incoming messages
* Improve logging in `ConnectionBase` by logging out mWho tuple
* Do not inherit transport handles on Windows
* Deprecate old `Pidf.hxx`/`Pidf.cxx` in favor of the newer and more flexible `GenericPidfContents` class

### dum
* Session Timer improvements
  * Improve Session Timer support when acting as refresher
  * Allow remote support for Session Timers to be determined in Supported or Requires headers
  * Handle OPTIONS separately in session timer logic
  * Add support for session refresh using OPTIONS (RFC 4028)
* Fix repeated 200 OK retransmission on overlapping/glare re-INVITE
  * Correlate the 2xx ACK to mInvite200 to break the retransmission loop
* Refactor SIP capability handling in InviteSession
* Prevent crashes if a UAS Invite Dialog ends up in the same DialogSet as a UAC Invite Dialog
* Add `ServerRegistration::reject` overload that accepts a specific SipMessage (with Contents object)
* Allow applications to see low-level connection error information
* HttpProvider abstract base class: add `ConnectTimeoutSeconds` and `RequestTimeoutSeconds` get request timeout options
* `HttpGetMessage`: extend with status, headers, and userData fields
* Improve logging in `DialogUsageManager::sendUsingOutboundIfAppropriate`
* Log connection terminate message contents in DialogUsageManager
* Improve logs in DUM for easier tracking of errors
* Remove unused members from DialogUsageManager

### repro
* Add REST API alongside existing HTML WebAdmin interface
* Support Public IP/Port use in transport RecordRoutes
* RFC 5626 Outbound fix: do not remove contact registration on locally generated 408 timeouts
* Do not strip PAssertedIdentityHeaders in IsTrustedNode processor if proxy has PAssertedIdentityProcessing disabled

### reTurn
* Implement real HMAC when `USE_SSL` isn't defined; add MESSAGE-INTEGRITY-SHA256 (RFC 8489)
* Improve reTurn nonce generation
  * Use crypto random for private part of nonce
  * Incorporate sender's protocol, IP, and port into nonce so nonce is unique per sender
* Do not respond to responses (server side correctness fix)

### recon
* Fix race condition crash
* Ensure BridgeMixerWeights are recalculated when setting record channel number
* Disable stream player in SipXMediaResourceParticipant (stream player is not supported in sipX)

### sdpcontainer / resipmedia
* Move SdpContainer (previously used only in recon) to new `resipmedia` library for broader reuse
* Rename `SdpHelperResip` to `SdpHelper`
* CMake: build resipmedia by default with a separate enable flag, independent of recon
* Add support for `rtcp-mux` attribute in sdpcontainer
* Add support for SDP transport protocol `PROTOCOL_TYPE_UDP_TLS_RTP_SAVPF`
* Add RFC 7714 crypto suites (WebRTC DTLS-SRTP) to `SdpCryptoSuiteType`
* Add `SdpHelper` unit tests and fix issues with SDP capability negotiation handling
* `sdpcontainer::Sdp`: add `getMediaLineAtIndex` API
* `sdpcontainer::SdpMediaLine`: add more crypto type definitions


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
* bump contrib\srtp\CMakeLists.txt cmake_minimum_required version to match base resip CMakeLists.txt
* Fix inconsistent include guards in p2p/DictionaryValue.hxx
* Replace set with unordered_set in BasicDomainMatcher and ExtendedDomainMatcher
* Use find() instead of count() for element existence checks in STL containers
* Optimize locking in DnsInterface::isSupported()
* Early return in ExtendedDomainMatcher::isMyDomain() when mDomainSuffixList is empty, preventing unnecessary domain case conversions
* Apply fixes from static code analysis (Coverity)
  * fix some potential resource/memory leaks
  * fix some potential uninitialized access issues
  * fix some large object copies
* Fix NameAddr parsing bug when angle brackets are included in quoted parameters
* If SSL_read fails after SSL_pending returns > 0, then keep bytes from original read if error code is retryable
* Optimize lock scopes for improved efficiency
* Improve logging in DialogUsageManager::sendUsingOutboundIfAppropriate
* Allow applications to see low level connection error information, including TLS handshake failure errors
  * add transport FailureReason and FailureSubCode FailureString and AdditionalFailureStrings to ConnectionTerminated messaging
  * make sure we store certificate validation errors in mFailureSubCode
  * make sure we get the client cert validation error in scenarios where a client cert isn't available for querying
  * reduce number of log statements during SSL/TLS errors - but maintain same level of information
  * Propagate SSL Certificate Validation errors to ConnectionTerminated event/message (AdditionalFailureStrings)
* fixed 2 memory leaks from SSL_get_peer_certificate calls (DtlsSocket and TlsConnection - missing X509_free calls)
* remove error log in BaseSecurity::hasCert when first connecting to a new Tls domain


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
* add new AllowInDialogImpersonationWithinRealm setting that allows repro to authenticate in an environment where 302 redirects are used
* ResponseContext: add the possibility of creating a derived class to implement custom logic for forwardBestResponse()
* fix best response code to be 500 (vs 480) if 503 is received, following RFC 3261

### reTurn
* asio updated to version 1.18.1
* fix static initialization issue that can cause a startup deadlock
* TurnAsyncSocket - allow software attribute to be customized
* fix the async client to choose the DNS result corresponding to local endpoint protocol type
* add EchoServer application that can be used to echo RTP/RTCP data back at sender for TURN server load testing
* add TurnLoadGenClient that can be used to load test a TURN server
