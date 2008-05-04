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
5.  svn checkout https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/srtp srtp
6.  svn checkout https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl openssl

/resip-main/                <- https://svn.resiprocate.org/rep/resiprocate/main
/resip-main/contrib/srtp    <- https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/srtp
/resip-main/contrib/openssl <- https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl
/main/contrib/boost_1_34_1  <- BOOST 1.34.1 (required in this location for Windows builds only)
/sipXtapi                   <- https://scm.sipfoundry.org/rep/sipX/branches/sipXtapi


Build Notes
-----------

-You must build the sipXmediaAdpapterLib with the following defines
-DDISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY
-DENABLE_TOPOLOGY_FLOWGRAPH_INTERFACE_FACTORY
-Apply the sipXtapi-10645-recon.patch file to the SVN checked out version of 
 sipXtapi before building


Building dtls-srtp version of OpenSSL on Windows
------------------------------------------------
/resip-main/contrib/openssl <- https://svn.resiprocate.org/rep/resiprocate/contrib/dtls-srtp/openssl

recon currently uses a branch of OpenSSL that contains modification to do dtls-srtp.
To build openSSL for windows:
1.  Use VS2003 Command Prompt window - Note: 0.9.8g will not build 
    Crypto ASM fns with VS2003 MASM - since we use static libs and until we upgrade 
    all projects to VS2005 - we will build openssl with no ASM
2.  From openssl root run: perl Configure VC-WIN32 enable-tlsext
3.  Run: perl util\mkfiles.pl >MINFO
4.  Run: perl util\mk1mf.pl no-asm debug VC-WIN32 > d32.mak
5.  Run: perl util\mk1mf.pl no-asm VC-WIN32 > 32.mak
6.  Run: nmake -f d32.mak
7.  Run: nmake -f 32.mak


Running on Windows
------------------
A note on testUA executable requirements:
By default you will be able to run testUA from the VS debugger, but if you decide
to run testUA.exe on another machine you will need the following:
- codec_*.dll from sipXtapi/sipXmediaLib/bin directory
- VS 2003 - C-runtime libaries present on the destination system


Building OpenSSL for Generic Linux
----------------------------------
1.  Go to resip/contrib/openssl
2.  ./Configure linux-generic32 --openssldir=/usr enable-tlsext
3.  make depend
4.  make

Building libSRTP for Generic Linux
----------------------------------
1.  Go to resip/contrib/srtp
2.  ./configure
3.  make

Building base resiprocate libraries for Generic Linux
-----------------------------------------------------
1.  Go to resip/
2.  ./configure - select options as desired
    defaults are good: just be sure to point openssl path to:
    {localtion of build env}/resip/contrib/openssl
3.  make
4.  make reTurn

Building reTurn
---------------
1.  Go to resip/
2.  make reTurn

STEPS BELOW ARE STILL A WORK IN PROGRESS!!!

Building reflow
---------------
1.  Go to resip/reflow
2.  make

Building recon
--------------
...


