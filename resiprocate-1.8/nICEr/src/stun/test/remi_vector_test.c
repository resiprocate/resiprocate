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
#include "nr_crypto_openssl.h"

/*
	From: 	  remi.denis-courmont@nokia.com
	Subject: 	Re: [BEHAVE] STUN fingerprint and integrity tests
	Date: 	August 8, 2007 2:03:19 AM PDT
	To: 	  behave@ietf.org
*/


static int returnpassword(void *arg, nr_stun_message *msg, Data **password)
{
    *password = (Data*)arg;
    return 0;
}

int
main(int argc, char **argv)
{
#if 0
/* Also http://www.remlab.net/files/ietf/draft-denis-behave-rfc3489bis-test-vectors-00.txt */

char req[] =
"\x00\x01\x00\x44"
"\x21\x12\xa4\x42\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
"\x00\x24\x00\x04" "\x6e\x00\x01\xff"
"\x80\x29\x00\x08" "\x93\x2f\xf9\xb1\x51\x26\x3b\x36\x00\x06\x00"
 "\x09\x65\x76\x74\x6a\x3a\x68\x36\x76\x59\x20\x20\x20"
"\x00\x08\x00\x14" "\x62\x4e\xeb\xdc\x3c\xc9\x2d\xd8\x4b\x74\xbf\x85"
  "\xd1\xc0\xf5\xde\x36\x87\xbd\x33"
"\x80\x28\x00\x04" "\xad\x8a\x85\xff";

char resp[] =
"\x01\x01\x00\x3c"
"\x21\x12\xa4\x42\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
"\x80\x22\x00\x0b" "\x74\x65\x73\x74\x20\x76\x65\x63\x74\x6f\x72\x20"
"\x00\x20\x00\x08" "\x00\x01\xa1\x47\x5e\x12\xa4\x43"
"\x00\x08\x00\x14" "\xab\x4e\x53\x29\x61\x00\x08\x4c\x89\xf2\x7c\x69"
 "\x30\x33\x5c\xa3\x58\x14\xea\x90"
"\x80\x28\x00\x04" "\xae\x25\x8d\xf2";
#else
/* Also http://www.remlab.net/files/ietf/draft-denis-behave-rfc3489bis-test-vectors-01.txt */
UCHAR req[] =
     "\x00\x01\x00\x44"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x00\x24\x00\x04"
       "\x6e\x00\x01\xff"
     "\x80\x29\x00\x08"
       "\x93\x2f\xf9\xb1\x51\x26\x3b\x36"
     "\x00\x06\x00\x09"
       "\x65\x76\x74\x6a\x3a\x68\x36\x76\x59\x20\x20\x20"
     "\x00\x08\x00\x14"
       "\x62\x4e\xeb\xdc\x3c\xc9\x2d\xd8\x4b\x74\xbf\x85"
       "\xd1\xc0\xf5\xde\x36\x87\xbd\x33"
     "\x80\x28\x00\x04"
       "\xad\x8a\x85\xff";

UCHAR resp[] =
     "\x01\x01\x00\x3c"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x80\x22\x00\x0b"
       "\x74\x65\x73\x74\x20\x76\x65\x63\x74\x6f\x72\x20"
     "\x00\x20\x00\x08"
       "\x00\x01\xa1\x47\x5e\x12\xa4\x43"
     "\x00\x08\x00\x14"
       "\xab\x4e\x53\x29\x61\x00\x08\x4c\x89\xf2\x7c\x69"
       "\x30\x33\x5c\xa3\x58\x14\xea\x90"
     "\x80\x28\x00\x04"
       "\xae\x25\x8d\xf2";
#endif
    //char Username[] = "evtj:h6vY";
    Data Password = { (UCHAR*)"VOkJxbRl1RmTxUk/WvJxBt", sizeof("VOkJxbRl1RmTxUk/WvJxBt")-1 };
    nr_transport_addr from;
    nr_stun_message *rq;
    nr_stun_message *rs;
    nr_stun_message *ignore2;
    nr_stun_message_attribute *attr;

    NR_reg_init(NR_REG_MODE_LOCAL);
    nr_crypto_openssl_set();

    nr_ip4_str_port_to_transport_addr("127.0.0.1", 1234, IPPROTO_UDP, &from);

    assert(nr_stun_message_create2(&rq, req, sizeof(req)-1) == 0);
    assert(nr_stun_message_create(&ignore2) == 0);

    assert(nr_stun_decode_message(rq, returnpassword, &Password) == 0);
    assert(nr_stun_receive_message(0, rq)== 0);
    assert(nr_stun_process_request(rq,ignore2)== 0);

    assert(nr_stun_message_has_attribute(rq, NR_STUN_ATTR_FINGERPRINT, &attr));
    assert(attr->u.fingerprint.valid);
    assert(nr_stun_message_has_attribute(rq, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr));
    assert(attr->u.message_integrity.valid);

    assert(nr_stun_message_create2(&rs, resp, sizeof(resp)-1) == 0);
    assert(nr_stun_message_create(&ignore2) == 0);

    assert(nr_stun_decode_message(rs, returnpassword, &Password) == 0);
    assert(nr_stun_receive_message(rq, rs)== 0);
    assert(nr_stun_process_success_response(rs) == 0);

    assert(nr_stun_message_has_attribute(rs, NR_STUN_ATTR_FINGERPRINT, &attr));
    assert(attr->u.fingerprint.valid);
    assert(nr_stun_message_has_attribute(rs, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr));
    assert(attr->u.message_integrity.valid);

    fprintf(stderr,"Success!\n");
    return 0;
}
