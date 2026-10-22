#!/bin/bash
set -e
set -x

# see the man page for mib2c for details about the logic,
# in particular, the choices between mib2c.scalar.conf and
# alternatives

# zless /usr/share/snmp/mibs-downloader/mibrfcs/rfc4780.txt.gz

#SETS="sipCommonCfgEntry sipCommonSummaryStatsEntry"
#SETS="sipCommonCfgTable"
SETS=applTable

export MIBDIRS="`net-snmp-config --default-mibdirs`:/var/lib/mibs/ietf/"
#export MIBS="+SIP-COMMON-MIB"
export MIBS="+NETWORK-SERVICES-MIB"

for SET in ${SETS}
do
  #mib2c -c mib2c.scalar.conf ${SET}
  #mib2c -c mib2c.int_watch.conf ${SET}
  mib2c -c mib2c.mfd.conf ${SET}
  mib2c -c mfd-makefile.m2m ${SET}
  mib2c -c subagent.m2c ${SET}
done


