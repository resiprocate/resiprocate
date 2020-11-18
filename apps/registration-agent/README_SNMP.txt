
The registrationAgent operates as an SNMP agentx subagent

It will connect to the SNMP master agent using the configured socket,
the default is /var/agentx/master

Some notes are presented to assist with using and developing for SNMP

Dependencies
------------

On a Debian system, ensure that the required packages are installed.

  sudo apt install snmpd snmp snmp-mibs-downloader libsnmp-dev libsnmp-perl

snmpd daemon (master agent) configuration
-----------------------------------------

Edit /etc/snmp/snmpd.conf
uncomment:
  rocommunity public  localhost
or add a similar line permitting the hosts who will query the agent.

Add something like this:
  agentxsocket /var/run/snmp-agentx-master
  agentxperms      0770 0770 nobody Debian-snmp

Those lines are necessary because the default socket is only accessible
by processes running as root and there is also a bug in the snmpd
agentxperms DIRPERMS parameter:
https://lists.opensuse.org/opensuse-bugs/2016-08/msg00847.html

If you want to connect to the agent from other hosts, edit /etc/default/snmpd
and set the bind address to 0.0.0.0

After doing that, it is necessary to reload the service:
  sudo systemctl reload snmpd

If using a UNIX domain socket, to ensure the subagent can connect to the agent,
the user running the subagent process must be added to the Debian-snmp group.
If you will test the process by running it in your own user account, you can
do that with the command:

  sudo addgroup $USER Debian-snmp

or if you run the service using the unit file from the package:

  sudo addgroup registration-agent Debian-snmp

Edit registrationAgent.config.  Uncomment SNMPMasterSocket.
Make sure the value of SNMPMasterSocket matches the agentxsocket
value in /etc/snmp/snmpd.conf, for example:

  SNMPMasterSocket = /var/run/snmp-agentx-master

When you start the registrationAgent now, it should successfully connect
to the master agent.

Testing it
----------

Here are some examples of snmp commands to query the agent:

  snmpwalk -c public -v 1  localhost .1.3.6.1.4.1.8072.9999
  snmpget -c public -v 1 -Onv localhost .1.3.6.1.4.1.8072.9999.1.1.2.0

and if the MIB file is installed:

  snmpget -c public -v 1 -Onv localhost \
     REG-AGENT-MIB::reSIProcate.registrationAgent.registrationsFailed.0

Nagios monitoring
-----------------

Here is an example Nagios command definition to query the SNMP sub-agent
and raise a critical error if any registrations are in the failed state.

define command {
        command_name    check_snmp_sip_registrations_failed
        command_line    /usr/lib/nagios/plugins/check_snmp -H '$HOSTADDRESS$' -o REG-AGENT-MIB::reSIProcate.registrationAgent.registrationsFailed.0 -c :0
}

Development process
-------------------

1. Create an MIB file or make the necessary changes to the REG-AGENT-MIB file

2. Check the file, for example, smilint and snmptranslate are useful tools
   to verify that an MIB is valid and also verify your environment:

      smilint -l 4 -s apps/registration-agent/REG-AGENT-MIB

      export MIBDIRS="`net-snmp-config --default-mibdirs`:`readlink -f apps/registration-agent`"
      export MIBS="+REG-AGENT-MIB"

      snmptranslate -Tp -IR reSIProcate

3. Create the agent code

      cd apps/registration-agent/mib2c-skeleton

      mib2c -c mib2c.scalar.conf reSIProcate
      mib2c -c mfd-makefile.m2m reSIProcate
      mib2c -c subagent.m2c reSIProcate

4. Commit your changes to the MIB file

5. Commit the updated files in the mib2c-skeleton (these are never compiled,
   they are simply kept in the repository for reference purposes)

6. Review the changes to reSIProcate.c against the previous commit.
   Manually apply the same changes to the SNMP_reSIProcate.cxx file.

7. When you run configure, ensure that --enable-snmp is set.
 
