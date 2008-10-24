/*
Copyright (c) 2007, Adobe Systems, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Adobe Systems, Network Resonance nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <assert.h>

#include <net/if.h>
#ifndef LINUX
#include <net/if_var.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#endif
#include <net/route.h>

/* IP */
#include <netinet/in.h>
#ifndef LINUX
#include <netinet/in_var.h>
#endif
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "stun.h"
#include "r_types.h"
#include "r_macros.h"
#include "r_errors.h"
#include "r_log.h"
#include "addrs.h"

static int
is_duplicate_addrs(nr_transport_addr addrs[], int count, int index, nr_transport_addr *addr)
{
    int i;
    int same_addr;
    int same_ifc;

    for (i = 0; i < count; ++i) {
        same_addr = ! nr_transport_addr_cmp(&addrs[i], addr, NR_TRANSPORT_ADDR_CMP_MODE_ALL);
        same_ifc = ! strncmp(addrs[i].ifname, addr->ifname, IFNAMSIZ);
        if (i == index) {
            assert(same_addr && same_ifc);
        }
        else if (same_addr && same_ifc)
            return 1;  /* duplicate */
    }

    return 0;
}

void
no_dups_test(nr_transport_addr addrs[], int count)
{
    int i;
    for (i = 0; i < count; ++i) 
      assert(!is_duplicate_addrs(addrs, count, i, &addrs[i]));
}

void
dup_addr_test()
{
   int count;

   nr_transport_addr addr1[4];
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr1[0]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr1[1]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr1[2]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr1[3]);
   strcpy(addr1[0].ifname, "en0");
   strcpy(addr1[1].ifname, "en0");
   strcpy(addr1[2].ifname, "en0");
   strcpy(addr1[3].ifname, "en0");

   count = 1;
   nr_stun_remove_duplicate_addrs(addr1, 0, &count);
   assert(count == 1);
   no_dups_test(addr1, count);

   count = 2;
   nr_stun_remove_duplicate_addrs(addr1, 0, &count);
   assert(count == 1);
   no_dups_test(addr1, count);

   count = 3;
   nr_stun_remove_duplicate_addrs(addr1, 0, &count);
   assert(count == 1);
   no_dups_test(addr1, count);

   count = 4;
   nr_stun_remove_duplicate_addrs(addr1, 0, &count);
   assert(count == 1);
   no_dups_test(addr1, count);

   nr_transport_addr addr2[4];
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr2[0]);
   nr_ip4_str_port_to_transport_addr("4.3.2.1", 5678, IPPROTO_UDP, &addr2[1]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr2[2]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr2[3]);
   strcpy(addr2[0].ifname, "en0");
   strcpy(addr2[1].ifname, "en0");
   strcpy(addr2[2].ifname, "en0");
   strcpy(addr2[3].ifname, "en0");

   count = 2;
   nr_stun_remove_duplicate_addrs(addr2, 0, &count);
   assert(count == 2);
   no_dups_test(addr2, count);

   count = 3;
   nr_stun_remove_duplicate_addrs(addr2, 0, &count);
   assert(count == 2);
   no_dups_test(addr2, count);

   count = 4;
   nr_stun_remove_duplicate_addrs(addr2, 0, &count);
   assert(count == 2);
   no_dups_test(addr2, count);

   nr_transport_addr addr3[2];
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr3[0]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr3[1]);
   strcpy(addr3[0].ifname, "en0");
   strcpy(addr3[1].ifname, "en0");

   count = 2;
   nr_stun_remove_duplicate_addrs(addr3, 0, &count);
   assert(count == 1);
   no_dups_test(addr3, count);

   nr_transport_addr loopback;
   nr_ip4_str_port_to_transport_addr("127.0.0.1", 5678, IPPROTO_UDP, &loopback);
   strcpy(loopback.ifname, "lo0");

   count = 1;
   nr_stun_remove_duplicate_addrs(&loopback, 0, &count);
   assert(count == 1);
   no_dups_test(&loopback, count);

   count = 1;
   nr_stun_remove_duplicate_addrs(&loopback, 1, &count);
   assert(count == 0);
   no_dups_test(&loopback, count);

   nr_transport_addr addr6[4];
   nr_ip4_str_port_to_transport_addr("4.3.2.1", 5678, IPPROTO_UDP, &addr6[0]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr6[1]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr6[2]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr6[3]);
   strcpy(addr6[0].ifname, "en0");
   strcpy(addr6[1].ifname, "en0");
   strcpy(addr6[2].ifname, "en0");
   strcpy(addr6[3].ifname, "en0");

   count = 2;
   nr_stun_remove_duplicate_addrs(addr6, 0, &count);
   assert(count == 2);
   no_dups_test(addr6, count);

   count = 3;
   nr_stun_remove_duplicate_addrs(addr6, 0, &count);
   assert(count == 2);
   no_dups_test(addr6, count);

   count = 4;
   nr_stun_remove_duplicate_addrs(addr6, 0, &count);
   assert(count == 2);
   no_dups_test(addr6, count);

   nr_transport_addr addr7[4];
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr7[0]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr7[1]);
   nr_ip4_str_port_to_transport_addr("4.3.2.1", 5678, IPPROTO_UDP, &addr7[2]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr7[3]);
   strcpy(addr7[0].ifname, "en0");
   strcpy(addr7[1].ifname, "en0");
   strcpy(addr7[2].ifname, "en0");
   strcpy(addr7[3].ifname, "en0");

   count = 2;
   nr_stun_remove_duplicate_addrs(addr7, 0, &count);
   assert(count == 1);
   no_dups_test(addr7, count);

   count = 3;
   nr_stun_remove_duplicate_addrs(addr7, 0, &count);
   assert(count == 2);
   no_dups_test(addr7, count);

   count = 4;
   nr_stun_remove_duplicate_addrs(addr7, 0, &count);
   assert(count == 2);
   no_dups_test(addr7, count);

   nr_transport_addr addr8[4];
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr8[0]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr8[1]);
   nr_ip4_str_port_to_transport_addr("1.2.3.4", 5678, IPPROTO_UDP, &addr8[2]);
   nr_ip4_str_port_to_transport_addr("4.3.2.1", 5678, IPPROTO_UDP, &addr8[3]);
   strcpy(addr8[0].ifname, "en0");
   strcpy(addr8[1].ifname, "en0");
   strcpy(addr8[2].ifname, "en0");
   strcpy(addr8[3].ifname, "en0");

   count = 2;
   nr_stun_remove_duplicate_addrs(addr8, 0, &count);
   assert(count == 1);
   no_dups_test(addr8, count);

   count = 3;
   nr_stun_remove_duplicate_addrs(addr8, 0, &count);
   assert(count == 1);
   no_dups_test(addr8, count);

   count = 4;
   nr_stun_remove_duplicate_addrs(addr8, 0, &count);
   assert(count == 2);
   no_dups_test(addr8, count);

}



int
main(argc, argv)
    int argc;
    char *const *argv;
{
    dup_addr_test();
    fprintf(stderr,"Success\n");
    return 0;
}
