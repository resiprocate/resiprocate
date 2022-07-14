
We have started introducing support for AMQP messaging.

AMQP allows us to send notifications about events to other
applications on topics and to receive notifications and commands
over queues.

Maintainer
----------

Daniel Pocock
daniel@pocock.pro
https://softwarefreedom.institute

Dependencies
------------

Apache Qpid Proton C++
Version tested: 0.22.0
License: Apache 2.0

Apache Qpid Proton Python (optional, for testing)
Version tested: 0.22.0

RabbitMQ (optional, can use other brokers too)
Version tested: 3.8.9

Installing dependencies and compiling reSIProcate with Qpid Proton
------------------------------------------------------------------

Debian / Ubuntu:

   sudo apt install \
      libqpid-proton11-dev \
      libqpid-proton-cpp11-dev

   ./configure ... --with-qpid-proton ...

RPM:

   sudo dnf install qpid-proton-cpp-devel

   ./configure ... --with-qpid-proton ...

Installing and configuring RabbitMQ
-----------------------------------

Debian / Ubuntu:

  sudo apt install rabbitmq-server
  sudo rabbitmq-plugins enable rabbitmq_amqp1_0
  sudo systemctl restart rabbitmq-server

RPM:

  sudo dnf install rabbitmq-server
  sudo rabbitmq-plugins enable rabbitmq_amqp1_0
  sudo systemctl restart rabbitmq-server

Installing and using the Python command line utilities
------------------------------------------------------

Debian / Ubuntu:
   sudo apt install python3-qpid-proton

RPM:
   sudo dnf install python-qpid

Example sending a JSON command to reConServer:

   Uncomment the BrokerURL in reConServer.config or any of the other
   reSIProcate applications, repro.config, registrationAgent.config, ...

   BrokerURL = amqp://localhost:5672//queue/sip.reconserver.cmd

   (re)start the reConServer

   Send the command:

   ./tools/send-cmd.py \
     -a localhost:5672/sip.reconserver.cmd \
     -m '{"command":"inviteToRoom","arguments":{"destination":"sip:cisco@10.1.2.3?transport=tcp","room":"room1"}}'

Example receiving messages from the reConServer queue or topic:

   Uncomment the EventTopicURL in reConServer.config or any of the other
   reSIProcate applications, repro.config, registrationAgent.config, ...

   EventTopicURL = amqp://localhost:5672//queue/sip.reconserver.events

   (re)start the reConServer

   Run the utility in the console, messages appear on stdout:

   ./tools/monitor-amqp-queue-topic.py \
     -a localhost:5672//queue/sip.reconserver.events
