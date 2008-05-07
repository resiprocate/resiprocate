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



static char *RCSSTRING __UNUSED__="$Id: transport_addr_reg_test.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

#include <csi_platform.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <assert.h>
#include "nr_api.h"
#include "transport_addr.h"
#include "transport_addr_reg.h"


static void
test(char *address, UINT2 port, char *ifname, int protocol)
{
    nr_transport_addr addr1;
    nr_transport_addr addr2;

    if (address == 0)
        address = "0.0.0.0";

    if (protocol == 0)
        protocol = IPPROTO_UDP;

    assert(nr_ip4_str_port_to_transport_addr(address, port, protocol, &addr1) == 0);
    if (ifname)
        strcpy(addr1.ifname, ifname);
    assert(nr_reg_set_transport_addr("transport_addr_reg_test", 0, &addr1) == 0);

    assert(nr_reg_get_transport_addr("transport_addr_reg_test", 0, &addr2) == 0);

    assert(! nr_transport_addr_cmp(&addr1, &addr2, NR_TRANSPORT_ADDR_CMP_MODE_ALL));
    assert(! strcmp(addr1.ifname, addr2.ifname));

#if 0
fprintf(stderr,"%s%s%s\n", addr1.as_string, (strlen(addr1.ifname)>0)?" on ":"", addr1.ifname);
#endif
}

int
main()
{
    assert(NR_reg_init(NR_REG_MODE_LOCAL) == 0);

    test(0, 0, 0, 0);
    test("1.2.3.4", 0, 0, 0);
    test(0, 4321, 0, 0);
    test("1.2.3.4", 4321, 0, 0);
    test(0, 0, "foo", 0);
    test("1.2.3.4", 4321, "bar", 0);
    test("1.2.3.4", 4321, "bar", IPPROTO_TCP);

    fprintf(stderr,"Success!\n");

    return 0;
}

