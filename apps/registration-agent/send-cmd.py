#!/usr/bin/python3

#
# Send a String message from command line to an AMQP queue or topic
#
# Useful for testing messaging applications
#

# To install the dependencies:
#
#   apt-get install -t jessie-backports python3-qpid-proton
#

from __future__ import print_function, unicode_literals

import optparse
from proton import Message
from proton.handlers import MessagingHandler
from proton.reactor import Container
import sys

class Send(MessagingHandler):
    def __init__(self, url, msg_body):
        super(Send, self).__init__()
        self.url = url
        self.msg_body = msg_body

    def on_start(self, event):
        event.container.create_sender(self.url)

    def on_sendable(self, event):
        if self.msg_body is not None:
            print("on_sendable !")
            print ("sending : %s" % (self.msg_body,))
            #msg = Message(body=self.msg_body, inferred=True)
            if sys.version < '3':
                msg = Message(body=unicode(self.msg_body, "utf-8"))
            else:
                msg = Message(body=self.msg_body)
            event.sender.send(msg)
            self.msg_body = None

    def on_accepted(self, event):
        print("message confirmed")
        event.connection.close()

    def on_disconnected(self, event):
        print("Disconnected")

parser = optparse.OptionParser(usage="usage: %prog [options]",
                               description="Send messages to the supplied address.")
parser.add_option("-a", "--address", default="localhost:5672/examples",
                  help="address to which messages are sent (default %default)")
parser.add_option("-m", "--message", default="Hello World",
                  help="message text")
opts, args = parser.parse_args()

try:
    print (type(opts.message))
    Container(Send(opts.address, opts.message)).run()
except KeyboardInterrupt: pass

