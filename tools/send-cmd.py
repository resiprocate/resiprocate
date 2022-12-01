#!/usr/bin/python3

#
# Send a String message from command line to an AMQP queue or topic
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
#
# Copyright (c) 2022 Software Freedom Institute SA https://softwarefreedom.institute
# Copyright (c) 2022 Daniel Pocock https://danielpocock.com
#

from __future__ import print_function, unicode_literals

import optparse
from proton import Message
from proton.handlers import MessagingHandler
from proton.reactor import Container
import sys

class Send(MessagingHandler):
    def __init__(self, url, msg_body, wait_response):
        super(Send, self).__init__()
        self.url = url
        self.msg_body = msg_body
        self.msg_ready = True
        self.wait_response = wait_response

    def on_start(self, event):
        self.sender = event.container.create_sender(self.url)
        if self.wait_response:
            print("creating receiver for request/response operation")
            print("request/response requires recent version of qpid-proton client library")
            print("request/response is not supported in all AMQP brokers,")
            print("This has been tested successfully on Apache qpidd")
            self.receiver = event.container.create_receiver(self.sender.connection, None, dynamic=True)

    def on_sendable(self, event):
        if event.sender.credit and self.msg_ready:
            print ("sending : %s" % (self.msg_body,))
            #msg = Message(body=self.msg_body, inferred=True)
            if self.wait_response:
                if self.receiver.remote_source.address:
                    _reply_to = self.receiver.remote_source.address
                else:
                    print("request/response mode enabled but we don't have a reply-to address")
                    sys.exit(1)
            else:
                _reply_to = None
            if sys.version < '3':
                msg = Message(body=unicode(self.msg_body, "utf-8"), reply_to=_reply_to)
            else:
                msg = Message(body=self.msg_body, reply_to=_reply_to)
            event.sender.send(msg)
            self.msg_ready = False
            print("sent")

    def on_accepted(self, event):
        print("message confirmed")
        if self.wait_response:
            print("waiting for response on %s" % (self.receiver.remote_source.address,))
            # FIXME - add a timeout?
        else:
            event.connection.close()

    def on_message(self, event):
        print("response: %s" % (event.message.body,))
        event.connection.close()

    def on_disconnected(self, event):
        print("Disconnected")

parser = optparse.OptionParser(usage="usage: %prog [options]",
                               description="Send messages to the supplied address.")
parser.add_option("-a", "--address", default="localhost:5672/examples",
                  help="address to which messages are sent (default %default)")
parser.add_option("-m", "--message", default="Hello World",
                  help="message text")
parser.add_option("-r", "--response", default=False,
                  help="wait for a response", action="store_true")
opts, args = parser.parse_args()

try:
    Container(Send(opts.address, opts.message, opts.response)).run()
except KeyboardInterrupt:
    pass

