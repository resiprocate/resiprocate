Main Web Site:
http://www.sipfoundry.org/repro/

For the latest documentation please see:  
http://wiki.resiprocate.org/wiki/index.php?title=Main_Page#Repro_SIP_Proxy_Server or
http://wiki.resiprocate.org/

**WARNING - command line options in version 1.8 are not backwards compatible with older releases.

Command line format is:

repro [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] 

Sample Command lines:
repro repro.config --RecordRouteUri=sip:proxy.sipdomain.com --ForceRecordRouting=true
repro repro.config /RecordRouteUri:sip:proxy.sipdomain.com /ForceRecordRouting:true

