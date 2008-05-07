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


#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "nr_api.h"
#include "registry.h"
#include "stun.h"
#include "nr_socket_local.h"

static void check_request(nr_socket *sock, nr_transport_addr *from, char *buf, int len)
{
    nr_stun_server_ctx *ctx;

    assert(nr_stun_server_ctx_create("odd_test_vectors", sock, &ctx) == 0);
    assert(nr_stun_server_process_request(ctx, sock, buf, len, from, NR_STUN_AUTH_RULE_OPTIONAL) == 0);
}

int
main()
{
    nr_transport_addr from;
    nr_socket *sock;

/* this is a request that stund client generated */
char stund_0_96_request[] = {
0x00,0x01,0x00,0x08,0x01,0x7f,0x14,0x31,
0xbc,0x1d,0x73,0x2d,0x5b,0x07,0xbf,0x72,
0x97,0x69,0x64,0x30,0x00,0x03,0x00,0x04,
0x00,0x00,0x00,0x00, }; /* length 28 */


    NR_reg_init(NR_REG_MODE_LOCAL);

    nr_ip4_str_port_to_transport_addr("0.0.0.0", 0, IPPROTO_UDP, &from);
    nr_socket_local_create(&from, &sock);
    nr_socket_getaddr(sock, &from);

    check_request(sock, &from, stund_0_96_request, sizeof(stund_0_96_request));

    fprintf(stderr,"Success!\n");
    return 0;
}
