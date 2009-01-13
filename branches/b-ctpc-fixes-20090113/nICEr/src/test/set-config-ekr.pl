#!/usr/bin/perl

require 'test-config.pl';
require 'test-utils.pl';

# Registry settings from ice_test.c

NR_reg_set_char("logging.stderr.enabled", 1);
NR_reg_set_string("logging.stderr.level", "notice");
NR_reg_set_char("logging.syslog.enabled", 1);
NR_reg_set_string("logging.syslog.level", "notice");
if(0){
NR_reg_set_string("logging.syslog.facility.nr_ice_test.level", "debug");
NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
NR_reg_set_string("logging.stderr.facility.nr_ice_test.level", "debug");
NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
}
#NR_reg_set_string("ice.stun.server.0.addr","stun.briank.com");
# must be an IP address
NR_reg_set_string("ice.stun.server.0.addr","192.168.223.2");

#NR_reg_set_string("ice.stun.server.0.addr","192.168.1.105");
#NR_reg_set_string("ice.stun.server.0.addr","64.69.76.23");
#NR_reg_set_string("ice.stun.server.0.addr","4.5.6.7");
NR_reg_set_uint2("ice.stun.server.0.port",3478);
#NR_reg_set_string("ice.turn.server.0.addr","127.0.0.1");
#NR_reg_set_uint2("ice.turn.server.0.port",3478);
NR_reg_set_uchar("ice.pref.type.srv_rflx",100);
NR_reg_set_uchar("ice.pref.type.peer_rflx",105);
NR_reg_set_uchar("ice.pref.type.host",126);
NR_reg_set_uchar("ice.pref.type.relayed",90);
if ($IS_WIN32) {
NR_reg_set_uchar("ice.pref.interface.Local_Area_Connection", 255);
NR_reg_set_uchar("ice.pref.interface.Wireless_Network_Connection", 254);
}
elsif ($IS_LINUX) {
NR_reg_set_uchar("ice.pref.interface.eth0", 255);
NR_reg_set_uchar("ice.pref.interface.eth1", 254);
NR_reg_set_char("ice.suppress.interface.eth0", 1);
}
else {    # MacOSX, FreeBSD
NR_reg_set_uchar("ice.pref.interface.rl0", 255);
NR_reg_set_uchar("ice.pref.interface.wi0", 254);
NR_reg_set_uchar("ice.pref.interface.lo0", 253);
NR_reg_set_uchar("ice.pref.interface.en1", 252);
NR_reg_set_uchar("ice.pref.interface.en0", 251);
NR_reg_set_uchar("ice.pref.interface.ppp", 250);
NR_reg_set_uchar("ice.pref.interface.ppp0", 249);
NR_reg_set_uchar("ice.pref.interface.en2", 248);
NR_reg_set_uchar("ice.pref.interface.en3", 247);
}
NR_reg_set_uint4("stun.client.retransmission_timeout", 100);
#NR_reg_set_uint4("stun.client.retransmission_timeout", 10000);


