
# Introduction

Voice and video echo test

Demonstrates how to use GStreamer C++ bindings (GStreamerMM project)
with reSIProcate.

Currently supports regular RTP devices by using the GStreamer rtpbin element.
https://gstreamer.freedesktop.org/documentation/rtpmanager/rtpbin.html

Could be easily adapted for WebRTC using the GStreamer webrtcbin element.
https://gstreamer.freedesktop.org/documentation/webrtc/index.html
http://blog.nirbheek.in/2018/02/gstreamer-webrtc.html

See the configuration file for individual options.  In particular, you
can select one of these pipelines either through the configuration
file or by using the name of the pipeline as the SIP user address,
for example, dialing sip:h264v@192.168.1.10 will select h264v

h264avx - use libav decode and x264 encode
h264v - use VAAPI decode/encode
h264o - use openh264 decode and openh264 encode
h264m - use libav decode and openh264 encode
vp8 - use the vp8decode and vp8encode

# Help and feedback

https://list.resiprocate.org/mailman/listinfo/resiprocate-users
mailto:resiprocate-users@resiprocate.org

# Dependencies

GStreamer and plugins
https://gstreamer.freedesktop.org/
License: LGPL
Note: GPL and/or patent restrictions apply to some optional plugins
Debian/Ubuntu packages: https://packages.qa.debian.org/gstreamer1.0
Fedora/RHEL packages: https://src.fedoraproject.org/rpms/gstreamer1

GStreamerMM (C++ bindings)
https://wiki.gnome.org/Projects/gstreamermm
License: LGPL 2.1
Debian/Ubuntu packages: https://packages.qa.debian.org/gstreamermm-1.0
Fedora/RHEL packages: https://src.fedoraproject.org/rpms/gstreamermm

# Optional dependencies

gst-kurento-plugins (for the vp8parse element)
https://github.com/KurentoLegacy/gst-kurento-plugins
License: LGPL 2.1

OpenH264
http://www.openh264.org/
Note: it is necessary to rebuild gst-plugins-bad after install OpenH264


