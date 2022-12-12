
# Introduction

Use Gstreamer as the media stack for recon

Supports both audio and video

Tested on the following platforms:

  Debian 10 (buster)
    gstreamer 1.18.4-2 / gstreamermm 1.10.0+dfsg-3

  Debian 11 (testing/bookworm)
    gstreamer 1.20.3-2 / gstreamermm 1.10.0+dfsg-4

This implementation uses the Gstreamer webrtcbin component
and any available VP8 video codec.

# Quick start

  sudo apt install ... all the usual dependencies
  sudo apt install libgstreamer-plugins-bad1.0-dev
  vi build/debian.sh    (enable --with-gstreamer)
     or run ./configure ... --with-gstreamer ...
  make clean && make

# Troubleshooting

There is a guide to Gstreamer debugging features here:
https://gstreamer.freedesktop.org/documentation/gstreamer/running.html?gi-language=c

Set the environment variable GST_DEBUG_DUMP_DOT_DIR and the
process will automatically store snapshots of the pipeline into
DOT files.

For example, in gdb:
      set env GST_DEBUG_DUMP_DOT_DIR=/tmp

Use a command like this to convert the DOT files:

   dot -Tsvg /tmp/reCon-gst-graph.dot > /tmp/reCon-gst-graph.svg

# Dependencies

GStreamer and plugins
- must install the package gstreamer1.0-plugins-bad or
  equivalent package on other distributions for webrtcbin
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


