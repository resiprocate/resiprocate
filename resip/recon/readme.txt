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


Setting up build environment:
-----------------------------
1.  Go to directory where you want to create build env.
2.  svn checkout https://svn.resiprocate.org/rep/resiprocate/branches/b-recon-20080504 resip
3.  svn checkout https://scm.sipfoundry.org/rep/sipX/branches/sipXtapi sipXtapi
4.  cd resip/contrib
5.  svn checkout https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl openssl

/resip-main/                <- https://svn.resiprocate.org/rep/resiprocate/main
/resip-main/contrib/openssl <- https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl
/main/contrib/boost_1_34_1  <- BOOST 1.34.1 (required in this location for Windows builds only)
/sipXtapi                   <- https://scm.sipfoundry.org/rep/sipX/branches/sipXtapi


Building dtls-srtp version of OpenSSL on Windows
------------------------------------------------
recon currently uses a branch of OpenSSL that contains modification to do dtls-srtp.

/resip-main/contrib/openssl <- https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl

You will need ActiveState Perl, available from http://www.activestate.com/ActivePerl - there
is a free version available for download.

To build openSSL for windows using VS2003:
1.  Use VS2003 Command Prompt window - Note: 0.9.8g will not build 
    Crypto ASM fns with VS2003 MASM - we will build openssl with no ASM
2.  From openssl root run: perl Configure VC-WIN32 enable-tlsext
3.  Run: perl util\mkfiles.pl >MINFO
4.  Run: perl util\mk1mf.pl no-asm debug VC-WIN32 > d32.mak
5.  Run: perl util\mk1mf.pl no-asm VC-WIN32 > 32.mak
6.  Run: nmake -f d32.mak
7.  Run: nmake -f 32.mak

To build openSSL for windows using VS2005:
1.  Use VS2005 Command Prompt window
2.  From openssl root run: perl Configure VC-WIN32 enable-tlsext
3.  Run: ms\do_masm
4.  Run: perl util\mkfiles.pl >MINFO
5.  Run: perl util\mk1mf.pl no-asm debug VC-WIN32 > d32.mak
6.  Run: perl util\mk1mf.pl no-asm VC-WIN32 > 32.mak
7.  Run: nmake -f d32.mak
8.  Run: nmake -f 32.mak


Building recon on Windows
-------------------------
1.  Ensure the build environment is setup as indicated above.
2.  Ensure you apply sipXtapi-10645-recon.patch file to the SVN checked out version of 
    sipXtapi before building
3.  Use the recon_7_1.sln Visual Studio 2003 or recon_8_0.sln Visual Studio 2005 
    solution file
4.  Open the sipXmediaAdapterLib project settings and enable the following defines:
    DISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY
    ENABLE_TOPOLOGY_FLOWGRAPH_INTERFACE_FACTORY
5.  Build solution.


Running on Windows
------------------
A note on testUA executable requirements:
By default you will be able to run testUA from the VS debugger, but if you decide
to run testUA.exe on another machine you will need the following:
- codec_*.dll from sipXtapi/sipXmediaLib/bin directory
- VS 2003/2005 - C-runtime libaries present on the destination system




Building sipXtapi on Generic Linux
----------------------------------
1.  Go to sipXtapi root
2.  Apply resip/resip/recon/sipXtapi-10645-recon.patch to sipXtapi
3.  To build sipXportLib:
    cd sipXtapi/sipXportLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage
    make
4.  To build sipXsdpLib:
    cd sipXtapi/sipXsdpLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage
    make
5.  To build sipXtackLib:
    cd sipXtapi/sipXtackLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage --disable-sipviewer
    make
6.  To build sipXmediaLib:
    cd sipXtapi/sipXmediaLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage --enable-local-audio
    make
7.  To build sipXmediaAdapterLib:
    cd sipXtapi/sipXmediaAdapterLib
    autoreconf -fi
    ./configure --prefix=/tmp/stage CXXFLAGS="-DDISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY -DENABLE_TOPOLOGY_FLOWGRAPH_INTERFACE_FACTORY"
    make


Building recon on Generic Linux
-------------------------------
1.  Ensure the build environment is setup as indicated above.
2.  Build sipXtapi as described above first.
3.  If you don't need to cross compile openssl - proceed to step 3, otherwise:
    Go to main/contrib/openssl
    ./Configure {place-target-platform-here} --openssldir=/usr enable-tlsext
    make depend
    make
4.  Go to main/
5.  ./configure 
    - answer 'yes' to prompt to build reCon - Conversation Manager
    - make sure you enter the path to the dtls-srtp version of OpenSSL when it asks
      (ie: ....main/contrib/openssl)
6.  make


Running testua
--------------
1.  Go to main/resip/recon/test
2.  To run testUA ensure the codec plugins are in the same directory as the executable:
    cp ../../../../sipXtapi/sipXmediaLib/bin/*.so ./
3.  ./testUA



TODO List
---------
In order for recon to appeal to the widest audience possible, some changes
should be made in order to provide a better layer between the underlying
media stack (currently sipXtapi) and the Conversation Manager.  The following
tasks are required:
1.  Re-write the SDP library.  The SDP library provides a semantic representation 
    of the information conveyed in the Session Description Protocol (SDP), 
    including ICE candidates and components.  It is currently created in sipX 
    style code and reside in the sipXtapi repository (sipXsdpLib).  This library
    should be port/rewritten in resip style containers.  Once this is complete we 
    can modify the offer/answer logic in recon to only be dependant on resip.
2.  Provide a media access API/thin layer so that sipX API's are not accessed directly
    from different areas in recon source code.  Currently sipX API's are access 
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
