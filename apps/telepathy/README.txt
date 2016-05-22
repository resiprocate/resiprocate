
About
-----

This is a connection manager for the modular Telepathy framework used
on the Linux Desktop.

It uses the TelepathyQt API to interact with DBus.

It uses the high level reCon API from reSIProcate to co-ordinate
SIP and media streams.  reCon also supports conferencing.
The media stack from the sipXtapi project is used.

Although the reSIProcate project is very stable, this connection manager
is currently in an experimental state.  It should be seen as a
proof-of-concept and it is useful for interoperability testing with
other SIP devices and also with other Telepathy components.

It offers basic support for making and receiving audio calls.
There is currently no support for video, presence or chat messaging.

Telepathy already has a SIP connection manager, telepathy-rakia, based on
SofiaSIP.  Using the reSIProcate project with Telepathy offers
significant benefits:
- stable IPv6 support has been part of reSIProcate since the beginning
- excellent TLS support, including PFS, choice of ciphers and
  support for client certificate authentication
- thorough implementation of many newer RFCs including Outbound for
  stability in mobile and NAT environments (RFC 5626)
- many aspects of the stack can be configured through the
  UserAgentMasterProfile
- conferencing with up to 20 people
- Opus, the audio codec developed for the Internet
- G.722, wideband audio codec
- support for TURN relay servers for reliable NAT traversal
- support for SRTP and DTLS-SRTP media encryption
- an active project team
- compatibility with all the leading SIP server projects
- BSD licensed project widely used by commercial telephony products

Building and testing it
-----------------------

The easiest way to get all the dependencies and build tools is
to install them using the packages.  On a Debian or Ubuntu system,

  $ sudo apt-get build-dep resiprocate

It has been built and tested with Qt4 and TelepathyQt v0.9.6.1 on a
system running Debian jessie.

TelepathyQt includes a static library libtelepathy-qt4-service.
Linking against the static library leads to linker errors as described in
the bug report:
http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=801817

To work around that, TelepathyQt's build script was patched to build
libtelepathy-qt4-service as a shared library.  The modified version of the
package can be built from this branch:

https://github.com/dpocock/telepathy-qt-debian/tree/jessie-build-all-shared

Once the libtelepathy-qt*.deb packages have been built and installed,
the connection manager can be built as part of a normal reSIProcate
build.  From the top of the reSIProcate source tree, run the script

  $ build/debian.sh

and everything will be configured.  Then run

  $ make

To run the connection manager from the source tree, start it
from the command line:

  $ apps/telepathy/telepathy-resiprocate

All reSIProcate, sipXtapi and Qt logging appears on stdout and stderr

Once the process is running, open the account settings in a Telepathy
application such as GNOME Empathy and add the SIP account.

Important note about the audio device
-------------------------------------

As this code currently depends on the media stack from sipXapi, it is
using the /dev/dsp audio device from OSS.

If OSS support isn't available in your system or if the /dev/dsp device
node is used by another process, you may see errors like:

ERR |  | telepathy-resiprocate | USERAGENT |
| SipXHelper.cxx:47 | SIPxua:MpMedia:
MprFromInputDevice::doProcessFrame - Couldn't get device sample rate
from input device manager!  Device - "Unknown device" deviceId: 1

If you are using the PulseAudio sound system (it is installed by
default on many Linux systems), you can use the padsp utility
to simulate OSS device support:

  $ padsp ./telepathy-resiprocate

On Debian and Ubuntu systems, the padsp utility is in the package
called pulseaudio-utils, you can install it with:

  $ sudo apt-get install pulseaudio-utils

If you don't have PulseAudio:
a) make sure you have loaded the kernel module snd_pcm_oss, you
   may need to install a package such as
      apt-get install oss-compat
b) make sure your user has read/write access on /dev/dsp or make
   sure your user account is in a group that has access to /dev/dsp.
   On some systems, you simply need to add your user to the group "audio"
c) be careful if other applications are using /dev/dsp

This code is currently in a proof-of-concept state, it is known
that audio system support needs to be improved for a proper release.
This may be achieved through linking with libjingle or another API
instead of sipXtapi.


