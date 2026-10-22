
SNMP for reSIProcate
--------------------

apps/registration-agent has a very basic SNMP proof-of-concept
providing two simple variables, registrationsConfigured
and registrationsFailed from a custom MIB, REG-AGENT-MIB.txt

The proper way to do SNMP for SIP is described in RFC 4780
https://www.rfc-editor.org/rfc/rfc4780
and it is based on the NETWORK-SERVICES-MIB, RFC 2788
https://www.rfc-editor.org/rfc/rfc2788

Other references:
/usr/share/snmp/mibs-downloader/mibrfcs
/var/lib/mibs/ietf
http://www.net-snmp.org/

Running multiple SIP processes on a single host
-----------------------------------------------

It is very common to have multiple SIP processes on the same host.
For example, a single host may have two SIP proxy processes and a
SIP B2BUA.

Normally, this would present challenges for SNMP: each process
would need to use a different OID prefix, a different SNMP port
or some other permutation that allows them to coexist.  RFC 4780
authors have anticipated this problem and embraced the use of
the NETWORK-SERVICES-MIB and the applTable.

Multiple SIP processes can run on the same host and share a single
instance of the SNMP agent daemon.  Each process creates a row for
itself in the applTable with a distinct, dynamically generated
per-process applIndex.

Entries in the SIP tables are keyed by the applIndex.

applIndex values are dynamic.  Users of the SNMP data must
discover the current applIndex at runtime and be alert to the
possibility that it may change each time processes are restarted.

Code generation
---------------

The Net-SNMP library provides code generation tools.  The
proof-of-concept described the use of these tools in
apps/registration-agent/README_SNMP.txt under the
heading `Development process'.  The instructions refer to
the custom REG-AGENT-MIB which contains a single object.

The full SIP MIB requires a number of tables.
Additional steps are required for generating the table code
and customizing it.

In particular, the proof of concept uses the mib2c.scalar.conf

Tables require the mib2c.mfd.conf instead.

For an initial exploration of the the SIP tables, we have run
mib2c.mfd.conf twice, with and without enabling the example code
and committed the generated code without any modification.  It is
possible to explore the impact of the example code with the following
diff:

  diff -u snmp/applTable snmp/applTable-examples

Obtaining applIndex for a process
---------------------------------

When a reSIProcate-based process starts, it needs an applIndex
to be used in applTable and each of the SIP tables.

Use register_int_index() to get applIndex values.  This ensures
each reSIProcate-based process receives a distinct applIndex.

It can be configured in the method applTable_container_load


OID values in public use
------------------------

The registration-agent proof of concept uses the prefix

  .1.3.6.1.4.1.8072.9999

     4 = private org, 8072 = Net-SNMP, 9999 = experimental

RFC 4780 recommends

  .1.3.6.1.2.1.9998

     2 = IAB / IETF approved standards

and this is appropriate for processes that attempt to implement
the official MIB.  Due to the applTable from NETWORK-SERVICES-MIB,
multiple processes on the same host can use the same OID prefix,
even if some of them are not based on reSIProcate.

Kamailio appears to be implementing something very similar to
the standard under their own OID prefix.  They have taken the RFC 4780
MIBs and extended and customized them:
https://github.com/kamailio/kamailio/blob/master/src/modules/snmpstats/mibs

  .1.3.6.1.4.1.34352

     34352 = Kamailio

Using statistics from the stack in SNMP
---------------------------------------

Basic statistics required by RFC 4780 are in resip::StatisticsMessage

SipStack::setExternalStatsHandler can set a single handler for
statistics.  Some applications, like repro, already relying on this
feature for other purposes.
Therefore:
a) keep multiple instances of ExternalStatsHandler in a list, or
b) call the SNMP handler directly from within the StatisticsManager

SNMP sub-agent thread, architecture
-----------------------------------

Running the sub-agent in its own thread avoids blocking the
SipStack thread.

Some aspects of the sub-agent are common to all processes based
on reSIProcate.

Individual applications may wish to extend the sub-agent to add
support for optional or custom MIBs.  This could be done by
sub-classing the sub-agent thread or it could be implemented
with a container of extension classes managed by the sub-agent thread.

In the latter case, each component of the application adds its
own extension to the sub-agent dynamically.

SNMP sub-agent thread, latency considerations
---------------------------------------------

The proof-of-concept includes a thread class with hard-coded configuration
to run the sub-agent.  It is derived from the code generator.

The thread currently blocks in select() for up to 1 second.

The resip::StatisticsManager has a default interval of 60 seconds
between generating each StatisticsMessage.

This means that values obtained by SNMP would not be up to date.

To ensure SNMP responses contain fresh results, a few strategies are
possible:

a) StatisticsManager::setInterval() with a lower value, or
b) sub-agent calls SipStack::pollStatistics() before responding to a query, or
c) sub-agent bypasses the StatisticsManager and obtains values directly
   from SipStack::mTransactionController (only in SipStack thread),
   this could be achieved by removing the select() call in the SNMP thread
   and integrating it into the SipStack's poll mechanism.

