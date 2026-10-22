#!/usr/bin/python3
#
# Subscribe to a queue or topic and print the received messages to stdout
#
# Useful for testing messaging applications
#
# To install the dependencies:
#
#   Debian / Ubuntu:
#     apt install python3-qpid-proton
#
#   RPM:
#     dnf install qpid-python
#
# See README_AMQP_Apache_Qpid_Proton.txt

from __future__ import print_function, unicode_literals

import optparse
from proton import Message
from proton.handlers import MessagingHandler
from proton.reactor import Container
import sys

class MyReceiver(MessagingHandler):
    def __init__(self, url):
        super(MyReceiver, self).__init__()
        self.url = url

    def on_start(self, event):
        event.container.create_receiver(self.url)

    def on_message(self, event):
        print("received message: %s" % (event.message.body,))

    def on_disconnected(self, event):
        print("Disconnected")

parser = optparse.OptionParser(usage="usage: %prog [options]",
                               description="Receive messages from the supplied queue or topic address.")
parser.add_option("-a", "--address", default="localhost:5672/examples",
                  help="address to which messages are sent (default %default)")
opts, args = parser.parse_args()

try:
    Container(MyReceiver(opts.address)).run()
except KeyboardInterrupt: pass

