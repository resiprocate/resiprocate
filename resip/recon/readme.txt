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


Some Existing Limitations with sipX media Integration
-----------------------------------------------------
1.  In order get bridge mixer capabilities across all conversations, one sipX media 
    interface was used for all conversations created.  This has the following limitations:

    a.  The sipX compiled default is to allow 10 connections per media interface.  Thus there 
        is currently a limit of 7 remote participants in recon.  

	Note:  When Bridge In/Outputs are set to 10, 3 of the bridge connections are used for 
               the local speaker/mic, the file player and the tone player, so their are 7 left 
               for remote participants.

        The compile time flag: DEFAULT_BRIDGE_MAX_IN_OUTPUTS=10
        can be used to change this maximum - it must be defined for both the recon and the 
        sipXmediaLib projects.

    b.  The default topology media interface has one tone player and one file player, thus 
        only one media participant of each type can exist at a time.  For example, if you 
        create a media participant that is playing a file, and before it is finished, you 
        create a second media participant that is playing a file, then the output from the 
        second media participant will override the first and both media participants will 
        be destroyed when the second file has completed playing.

2.  The newer Topology Graph interface is used in recon to get Bridge Mixing support.  The
    following is a list of features that have either been deprecated or are not yet 
    implemented in this interface.

    a.  Streaming media from an http URL has been deperecated - thus http URLs are not 
        supported in the createMediaParticipant interface.
   
    b.  When playing media tones and files, the local-only and remote-only flags have not 
        yet been implemented, and will not currently work.



SRTP Implementation Notes
-------------------------

http://tools.ietf.org/html/draft-ietf-sip-dtls-srtp-framework
http://www.faqs.org/rfcs/rfc3711.html
http://tools.ietf.org/id/draft-ietf-mmusic-sdescriptions-12.txt

SDES Implementation Notes:
- default offer crypto suite is AES_CM_128_HMAC_SHA1_80 
- secure media required setting:
  enabled:  then SAVP transport protocol is signalled in SDP offers,
  disabled: then AVP transport portocol is signalled in SDP offers and encryption=optional attribute is added
- No f8 crypto suite - libsrtp limitation
- no MKI implementation
- no custom master key lifetime implementation
- no master key lifetime, re-keying when expired
- no Key Derivation Rate (KDR) implementation - libsrtp limitation
- no support for SDES SRTP Session parameters: Unencrypted/Unauthenticated, FEC_ORDER, FEC_KEY, WSH

DTLS Implementation Notes:
1. Only SHA-256 fingerprint is supported (not SHA-1, SHA-224, SHA-384, SHA-512, MD5 or MD2)
2. Passive side must do a STUN connectivity check - text in draft is inconsistent
3. Does not currently require that Identity header be present/validated.



Setting up build environment:
-----------------------------
1.  Go to directory where you want to create build env.
2.  svn checkout https://svn.resiprocate.org/rep/resiprocate/main resip
3.  svn checkout https://scm.sipfoundry.org/rep/sipX/main sipXtapi
4.  cd resip/contrib
5.  svn checkout https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl openssl

Note:  sipXtapi SVN repository has recently changed locations from: 
       https://scm.sipfoundry.org/rep/sipX/branches/sipXtapi     to 
       https://scm.sipfoundry.org/rep/sipX/main.  If you are migrating a local
       check out use the following command, or TortoiseSVN, to update your local 
       copy to the new location:
       svn switch https://scm.sipfoundry.org/rep/sipX/main

Note:  Ensure you use at least SVN revision 11413 of sipXtapi.

/resip/                  <- https://svn.resiprocate.org/rep/resiprocate/main
/resip/contrib/openssl   <- OpenSSL 1.0.1 or above
/resip/contrib/boost     <- BOOST 1.34.1 or above (required in this location for Windows builds only)
/sipXtapi                <- https://scm.sipfoundry.org/rep/sipX/branches/sipXtapi

Building recon on Windows
-------------------------
1.  Ensure the build environment is setup as indicated above.
2.  Use the recon_8_0.sln Visual Studio 2005 or recon_10_0.sln Visual Studio 2010
    solution file
3.  Open the sipXmediaAdapterLib project settings and enable the following defines:
    DISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY
    ENABLE_TOPOLOGY_FLOWGRAPH_INTERFACE_FACTORY
    by removing the 'xx' characters from the Preprocessor defines.
    You should do this for both Debug and Release project settings.
4.  Open the sipXmediaAdapterLib and sipXmediaLib project settings and add the 
    following define:  DISABLE_STREAM_PLAYER to the Preprocessor defines.
    You should do this for both Debug and Release project settings.
5.  Provide an include path to pcre for the sipXmediaLib projects by doing one 
    of the following: 
    - Modify your base Visual Studio settings for include paths - add an include 
      path to \resip\contrib\pcre 
    - Modify the Additional Include Directories settings of the sipXmediaAdapterLib, 
      sipXmedaLib and sipXportLib projects to include: ";..\..\resip\contrib\pcre"     
6.  Build solution.


Running on Windows
------------------
A note on testUA executable requirements:
By default you will be able to run testUA from the VS debugger, but if you decide
to run testUA.exe on another machine you will need the following:
- codec_*.dll from sipXtapi/sipXmediaLib/bin directory
- ca.pem file in working directory - contains acceptable root certificate authority (CA) 
  certificates for TURN over TLS 
- VS 2003/2005 - C-runtime libaries present on the destination system




Building sipXtapi on Generic Linux
----------------------------------
Note:  sipXtackLib is no longer required with the addition of the DISABLE_STREAM_PLAYER define
1.  Go to sipXtapi root
2.  To build sipXportLib:
    cd sipXtapi/sipXportLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage
    make
3.  To build sipXsdpLib:
    cd sipXtapi/sipXsdpLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage
    make
4.  To build sipXmediaLib:
    cd sipXtapi/sipXmediaLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage --enable-local-audio --disable-stream-player
    make
5.  To build sipXmediaAdapterLib:
    cd sipXtapi/sipXmediaAdapterLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage --enable-topology-graph --disable-stream-player
    make


Building recon on Generic Linux
-------------------------------
1.  Ensure the build environment is setup as indicated above.
2.  Build sipXtapi as described above first.
3.  If you don't need to cross compile openssl - proceed to step 4, otherwise:
    Go to resip/contrib/openssl
    ./Configure {place-target-platform-here} --openssldir=/usr enable-tlsext
    make depend
    make
4.  Go to resip/
5.  ./configure 
    - answer 'yes' to prompt to build reCon - Conversation Manager
    - make sure you enter the path to the dtls-srtp version of OpenSSL when it asks
      (ie: ....resip/contrib/openssl)
6.  make


Running testua on Generic Linux
-------------------------------
1.  Go to resip/resip/recon/test
2.  To run testUA ensure the codec plugins are in the same directory as the executable:
    cp ../../../../sipXtapi/sipXmediaLib/bin/*.so ./
3.  ca.pem file in working directory - contains acceptable root certificate authority 
    (CA) certificates for TURN over TLS     
4.  ./testUA



TODO List
---------
In order for recon to appeal to the widest audience possible, some changes
should be made in order to provide a better layer between the underlying
media stack (currently sipXtapi) and the Conversation Manager.  The following
task is required:

Provide a media access API/thin layer so that sipX API's are not accessed directly
from different areas in recon source code.  Currently sipX API's are accessed 
in the following locations:

    ConversationManager.cxx - contains main sipXmediaFactory and sipXmediaInterface - the interface into sipX library
      - createMediaInterface
      - setVolume
      - setMicrophoneGain
      - muteMicrophone
      - enableEchoCancel
      - enableAutoGainControl
      - enableNoiseReduction
 
    BridgeMixer.cxx - API's to control the sipX bridge mixing matrix
      - setMixWeightsForOutput
      - setMixWeightsForInput
 
    MediaResourceParticipant.cxx - API's to play tones, files, media
      - start/stopTone
      - start/stopAudio
      - playBuffer
      - createPlayer (deprecated in latest sipX)
 
    RemoteParticipantDialogSet - API's to create local socket/connection
      - create/deleteConnection
      - getCapabilities
      - getConnectionPortOnBridge
 
    RemoteParticipant.cxx - API's to start/stop RTP
      - setConnectionDestination
      - start/stopRtpSend
      - start/stopRtpReceive
      - isReceivingRtpAudio
      - isSendingRtpAudio    


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
