User Agent Tester (testUA) Instructions
=======================================

Author: Scott Godin (s g o d i n  a t  s i p s p e c t r u m  d o t  c o m)

TestUA is a test console for the recon (Conversation Manager) API.


Backgound Information on Conversation Manager
---------------------------------------------

The Conversation Manager is a VoIP API that allows conversations and participants
to be created.  Participants can then be placed into conversations.  In general
all participants belonging to the same conversation can be heard and hear each 
other.  A participants contribution level to a particular conversation can be 
modified such that they appear quiter or louder to other participants.

There are three types of participants:
1.  Local participant - this participant type consists of the default local speaker and 
    microphone of the computer that is running the testUA console.  Local
    participants are always only explicitly destroyed.
2.  Media participant - this participant type consists of an audio file, audio stream,
    or tone played out.  Media participants are destroyed when the file or tone output is
    complete.
3.  Remote Participant - this participant type uses audio from an external source.  The 
    current implementation uses SIP to create/connect to remote participants.  Remote
    participants are destroyed by the API user, or a "hangup" signal from the 
    remote party.

A typical phone conversation consists of 3 components: 1 conversation, 1 local participant
and 1 remote participant.



Startup Command Line Parameters
-------------------------------
Command line options are:
 -?, --?, --help, /? - display command line options

 -aa - enable autoanswer

  Enabling autoanswer will cause testUA to automatically answer any inbound SIP calls
  and place them in the lowest numbered conversation currently created.

 -a <IP Address> - bind SIP transports to this IP address

  In general the IP address to bind to is queried from the host OS.  This switch allows
  specification of the IP address for OS's that cannot be queried, or for machines that
  have mulitple NICs.

 -u <SIP URI> - URI of this SIP user

  This option is used to specify the SIP URI for this instance of testUA.  testUA uses
  this setting (-ip is not specified) in order to find the regisration server.  If 
  nothing is specified, then the default of sip:noreg@<ipaddress> will be used.

 -p <password> - SIP password of this this SIP user

  Use this switch in cases where the proxy digest challenges sip messaging.

 -nr - no registration, set this to disable registration with SIP Proxy

  By default, if a SIP uri is specified, testUA will attempt to register with it.  Use 
  this switch to disable this.

 -d <DNS servers> - comma seperated list of DNS servers, overrides OS detected list

  By default, testUA will query the OS for a list of DNS servers.  Use this option in 
  cases where the OS does not return any, or the correct values.

 -sp <port num> - local port number to use for SIP messaging (UDP/TCP)

  By default, testUA will use port 5062 for SIP messaging, use this switch to specify
  something different.

 -mp <port num> - local port number to start allocating from for RTP media

  By default, testUA will use media ports starting at 17384, use this switch to specify
  something different.

 -tp <port num> - local port number to use for TLS SIP messaging

  By default, testUA will listen for TLS connections on port 5063, use this switch to
  specify something different.  Note SIP certificiates must be present in executable 
  directory for windows hosts and ~/.sipCerts directory on linux hosts.

 -td <domain name> - domain name to use for TLS server connections

  By default, testUA will query the OS for a local hostname for TLS, use this switch
  to override the OS queried result.

 -nk - no keepalives, set this to disable sending of keepalives

  By default, testUA will enable UDP CRLF keepalives every 30 seconds and TCP keepalives 
  every 180 seconds.  Use this switch to disable CRLF keepalives.

 -op <SIP URI> - URI of a proxy server to use a SIP outbound proxy

  By default testUA does not use an outbound proxy.  Use this switch to route all 
  outbound, out-of-dialog requests through a fixed proxy despite the destination URI.

 -sm <Srtp|SrtpReq|SrtpDtls|SrtpDtlsReq> - sets the secure media mode

  By default, no secure media is offered in outbound SIP requests.  Use this option to
  change that behaviour.  Note:  Inbound secure media is always accepted.
  Srtp     - use SRTP with keying outside of media stream (SDES key negotiation)
             via SDP.  RTP/AVP profile is used, and transport capability of RTP/SAVP is 
             listed, in order to implement best-effort SRTP.  Note:  The crypo attribute
             is provided outside of the SDP capability, as this is required by SNOM for
             optional SRTP offers.
  SrtpReq  - use SRTP with keying outside of media stream (SDES key negotiation)
             via SDP.  RTP/SAVP profile is used to indicate that SRTP is mandatory.
  SrtpDtls - use SRTP with DTLS key negotiation.  RTP/AVP is use as a default, and a 
             transport capability of UDP/TLS/RTP/SAVP is listed, in order to impelement
             best-effort DTLS-SRTP.  
  SrtpDtlsReq - use SRTP with DTLS key negotiation.  UDP/TLS/RTP/SAVP profile is used to
             indicate that Dtls-Srtp use is mandatory.

 -nm <Bind|UdpAlloc|TcpAlloc|TlsAlloc> - sets the NAT traversal mode

  By default, no NAT traversal strategies are used.  Use this switch to specify one:  
  Bind - use Binding discovery on a STUN server, to discover and use "public" address 
         and port in SDP negotiations
  UdpAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over UDP and Allocate a UDP relay address and port to 
             use in SDP negotiations
  TcpAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over TCP and Allocate a UDP relay address and port to 
             use in SDP negotiations
  TlsAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over TLS and Allocate a UDP relay address and port to 
             use in SDP negotiations

 -ns <server:port> - set the hostname and port of the NAT STUN/TURN server

  If -nm switch is used then you MUST specify the STUN/TURN server name/address and port.

 -nu <username> - sets the STUN/TURN username to use for NAT server

  Use this option if the STUN/TURN server requires authentication.

 -np <password> - sets the STUN/TURN password to use for NAT server

  Use this option if the STUN/TURN server requires authentication.
  
 -nl - disables local audio support - removes requirement for local audio hardware.
       Note:  if local audio support is disabled, then local participants cannot
              be created.

 -l <NONE|CRIT|ERR|WARNING|INFO|DEBUG|STACK> - logging level

  By default the logging level is INFO, use this switch to change it.


Sample Command line:
  testUA -a 192.168.1.100 -u sip:1000@myproxy.com -p 123 -aa



Console Command Reference
-------------------------
Once the console is started, testUA will automatically register with a proxy server, 
if required.

When starting testUA, one Conversation (Handle=1) and one local participant (Handle=1) is 
automatically created for convienience.

The console then accepts the following commands:
createConversation:      <'createconv'|'cc'>
  Create a new empty conversation.

destroyConversation:     <'destroyconv'|'dc'> <convHandle>
  Destroys conversation and ends all participants that solely belong to this conversation.

joinConversation:        <'joinconv'|'jc'> <sourceConvHandle> <destConvHandle>
  Add all participants from source conversation to destination conversation
  and destroys source conversation.

createLocalParticipant:  <'createlocal'|'cl'>
  Creates a new local participant.

createRemoteParticipant: <'createremote'|'crp'> <convHandle> <destURI> [<'manual'>] 
  (last arg is fork select mode, 'auto' is default)
  Creates a new remote participant (outbound SIP call) in conversation specified.
  Dest URI must be provided.  When ForkSelectMode is set to auto the conversation 
  manager will automatically dispose of any related conversations that were created, 
  due to forking. 

createMediaResourceParticipant: <'createmedia'|'cmp'> <convHandle> <mediaURL> [<durationMs>]
  Creates a new media resource participant in the specified conversation.  Media is played 
  from a source specified by the url and may be a local audio file, audio file fetched via 
  HTTP or tones.  The URL can contain parameters that specify properties of the media 
  playback, such as number of repeats.  
  Media Urls are of the following format: 
       	"tone"|"file":<tone|filepath>[;duration=<duration>][;local-only]
                                     [;remote-only][;repeat][;prefetch]
  Tones can be any DTMF digit 0-9,*,#,A-D or a special tone: 
    	dialtone, busy, fastbusy, ringback, ring, backspace, callwaiting, holding, or 
        loudfastbusy
  Note: 'repeat' option only makes sense for file and http URLs
  Note2: 'prefetch' option only makes sense for http URLs
  Note3: audio files may be AU, WAV or RAW formats.  Audiofiles should be 16bit mono, 
         8khz, PCM to avoid runtime conversion.

destroyParticipant:      <'destroypart'|'dp'> <parthandle>
  Ends connections to the participant and removes it from all active conversations.

addPartcipant:           <'addpart'|'ap'> <convHandle> <partHandle>
  Adds a participant to an existing conversation.

removePartcipant:        <'removepart'|'rp'> <convHandle> <partHandle>
  Removes a participant from a conversation.  If the participant no longer exists in 
  any conversation, then they are destroyed (local participants exempt).  For a remote 
  participant this means the call will be released.

moveParticipant:         <'movepart'|'mp'> <partHandle> <srcConvHandle> <dstConvHandle>
  Removes participant from src conversation and adds them to the dst conversation. 

modifyParticipantContribution: <'partcontrib'|'pc'> <convHandle> <partHandle> <inputGain> 
                               <outputGain> (gain in percentage)
  Sets a participants input and output gain towards the specified conversation.  

outputBridgeMatrix:      <'bridgematrix'|'bm'>
  Outputs the sipX mixing bridge matrix for debugging purposes.

alertPartcipant:         <'alert'|'al'> <partHandle> [<'noearly'>] 
                         (last arg is early flag, enabled by default)
  Sends a 180 response to the far end.  If noearly is enabled then SDP is not sent in the 
  response.

answerParticipant:       <'answer'|'an'> <partHandle>
  Sends a 200 response to the far end.

rejectParticipant:       <'reject'|'rj'> <partHandle> [<statusCode>] 
  Sends the specied response code to the far end.  (default status code is 486)

redirectPartcipant:      <'redirect'|'rd'> <partHandle> <destURI>
  If unanswered - sends a 302 response to the far end with the destURI in the 
  contact header.  Otherwise sends a REFER request to the far end.

redirectToPartcipant:    <'redirectTo'|'rt'> <partHandle> <destPartHandle>
  Sends a REFER request to the far end with a 'replaces' header corresponding to the
  partHandle specified.

setSpeakerVolume:        <'volume'|'sv'> <volume>
setMicrophoneGain:       <'gain'|'sg'> <gain>
muteMicrophone:          <'mute'|'mm'> <'0'|'1'> (1 to enable/mute)
enableEchoCancel:        <'echocancel'|'aec'> <'0'|'1'> (1 to enable)
enableAutoGainControl:   <'autogain'|'agc'> <'0'|'1'> (1 to enable)
enableNoiseReduction:    <'noisereduction'|'nr'> <'0'|'1'> (1 to enable)

createSubscription:      <'subscribe'|'cs'> <eventType> <targetUri> <subTime> 
                                            <mimeType> <mimeSubType>
  Creates a SIP subscription to the targetURI of the corresponding eventType. 
  Expected mimeType and subType must be specified.

destroySubscription:     <'destsub'|'ds'> <subHandle>
  Unsubscribes an existing subscription.

setAutoAnswer            <'autoans'|'aa'> <'0'|'1'> (1 to enable (default)
  Enable this to have testUA automatically answer incoming calls and add to the
  lowest numbered created conversation.

setCodecs                <'setcodecs'|'sc'> <codecId>[,<codecId>]+ (comma separated list)
  Changes the default codec list/order.  Note a codec plugin must have been 
  present at startup time, in order to use that codec.  Unknown id's are 
  ignored.  Default is: 0,8,96,98,99,108,97,3,109 
  Acceptable sipX codec Ids are:
    0   - G.711 mu-law
    3   - GSM codec
    8   - G.711 a-law
    96  - Speex NB,  8,000bps
    97  - Speex NB,  5,950bps
    98  - Speex NB, 15,000bps
    99  - Speex NB, 24,600bps
    108 - Internet Low Bit Rate Codec - iLBC (RFC3951)
    109 - AVT/DTMF Tones, RFC 2833

setSecureMediaMode       <'securemedia'|'sm'> <'None'|'Srtp'|'SrtpReq'|'SrtpDtls'|'SrtpDtlsReq'>
  Allows changing the secure media mode at runtime.  Controls what is present
  for secure media in our SDP offers.  Note:  Inbound secure media is always accepted.
  Srtp     - use SRTP with keying outside of media stream (SDES key negotiation)
             via SDP.  RTP/AVP profile is used, and transport capability of RTP/SAVP is 
             listed, in order to implement best-effort SRTP.  Note:  The crypo attribute
             is provided outside of the SDP capability, as this is required by SNOM for
             optional SRTP offers.
  SrtpReq  - use SRTP with keying outside of media stream (SDES key negotiation)
             via SDP.  RTP/SAVP profile is used to indicate that SRTP is mandatory.
  SrtpDtls - use SRTP with DTLS key negotiation.  RTP/AVP is use as a default, and a 
             transport capability of UDP/TLS/RTP/SAVP is listed, in order to impelement
             best-effort DTLS-SRTP.  
  SrtpDtlsReq - use SRTP with DTLS key negotiation.  UDP/TLS/RTP/SAVP profile is used to
             indicate that Dtls-Srtp use is mandatory.

setNATTraversalMode      <'natmode'|'nm'> <'None'|'Bind'|'UdpAlloc'|'TcpAlloc'|'TlsAlloc'>
  Allows changing the NAT traversal mode at runtime.
  Bind - use Binding discovery on a STUN server, to discover and use "public" address 
         and port in SDP negotiations
  UdpAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over UDP and Allocate a UDP relay address and port to 
             use in SDP negotiations
  TcpAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over TCP and Allocate a UDP relay address and port to 
             use in SDP negotiations
  TlsAlloc - Use a TURN server as a media relay.  Communicate to the TURN
             server over TLS and Allocate a UDP relay address and port to 
             use in SDP negotiations

setNATTraversalServer    <'natserver'|'ns'> <server:port>
  Allows changing the STUN/TURN server hostname and port at runtime.

setNATUsername           <'natuser'|'nu'> <username>
  Allows changing the STUN/TURN username at runtime.

setNATPassword           <'natpwd'|'np'> <password>
  Allows chaning the STUN/TURN password at runtime.

startApplicationTimer:   <'starttimer'|'st'> <timerId> <durationMs> <seqNo>
  Test interface for application time API.

displayInfo:             <'info'|'i'>
  Display information about all of the currently created conversation handles, and
  participant handles.

exitProgram:             <'exit'|'quit'|'q'>
