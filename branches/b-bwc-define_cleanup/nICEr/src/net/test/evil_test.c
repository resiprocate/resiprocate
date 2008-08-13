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


static char *RCSSTRING __UNUSED__="$Id: evil_test.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

#include <string.h>
#include <csi_platform.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#endif
#include <assert.h>
#include "nr_api.h"
#include <nr_startup.h>
#include <async_wait.h>
#include <async_timer.h>
#include <registry.h>
#include "transport_addr.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "nr_socket_evil.h"


nr_socket *test_socket;
nr_socket *signal_socket;

char *server_host="10.0.0.1";
int server_port=5160;
nr_transport_addr server_addr;

char *client_host="192.168.0.10";
int client_port=23433;
nr_transport_addr client_addr;

char *test_name;
int evil_test_sendto;
int evil_test_recvfrom;

int failed = 0;
int succeeded = 0;

void
nr_socket_evil_test_reset()
{
    evil_test_sendto = 0;
    evil_test_recvfrom = 0;
}

#define EXPECT_SEND (1<<0)
#define EXPECT_RECV (1<<1)
#define EXPECT_NOTHING (0)

void
nr_socket_evil_test_results(int flags, nr_transport_addr *addr)
{
    if (flags & EXPECT_SEND) {
        if (!evil_test_sendto) {
            fprintf(stderr,"%s: packet dropped during send to %s\n", test_name, addr->as_string);
            ++failed;
        }
        else ++succeeded;
        if ((flags & EXPECT_RECV) && !evil_test_recvfrom) {
            fprintf(stderr,"%s: packet dropped during recv from %s\n", test_name, addr->as_string);
            ++failed;
        }
        else ++succeeded;
    }
    else {
        if (evil_test_sendto) {
            fprintf(stderr,"%s: packet passed during send to %s\n", test_name, addr->as_string);
            ++failed;
        }
        else ++succeeded;
        if (evil_test_recvfrom) {
            fprintf(stderr,"%s: packet passed during recv from %s\n", test_name, addr->as_string);
            ++failed;
        }
        else ++succeeded;
        if (flags & EXPECT_RECV) {
            /* can't have recv w/o send since data is echoed back */
            abort();
        }
    }
}

int nr_socket_evil_test_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *to)
  {
      size_t len2;
      evil_test_sendto++;

      /* implement loopback */
      if(nr_socket_recvfrom(signal_socket,(void*)msg,len,&len2,flags,to))
        abort();

      if (len2 > 0)
          evil_test_recvfrom++;

    return 0;
  }

int nr_socket_evil_test_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *addr)
  {
    /* echo back the buffer that was passed in, pretending to echo back
     * from whatever address we received on */
    *len = maxlen;
    return 0;
  }

static nr_socket_vtbl nr_socket_evil_test_vtbl={
  0,
  nr_socket_evil_test_sendto,
  nr_socket_evil_test_recvfrom,
  0,
  0,
  0 
};

#define START_TEST()              start_test((char*)__FUNCTION__)
#define FILTER(_reg,_str)         filter((_reg),(_str))
#define CREATE_EVIL()             create_evil()
#define TEST(_flags)              test((_flags))
#define TESTTO(_flags,_a,_p)      testto((_flags),(_a),(_p))
#define FAIL(_args)               do { printf _args ; printf("\n"); abort(); } while (0)

static void
start_test(char *function)
{
    test_name = function;

    if(NR_reg_del("net.socket.evil"))
      FAIL(("NR_reg_del"));
}

static void
filter(char *reg, char *str)
{
    char *child = strrchr(reg, '.') + 1;
    if (!strcasecmp(child, "action")) {
      if(NR_reg_set_string((reg),(str)))
        FAIL(("NR_reg_set_string('%s','%s')",(reg),(str)));
    } else if (!strcasecmp(child, "address")) {
      if(NR_reg_set_string((reg),(str)))
        FAIL(("NR_reg_set_string('%s','%s')",(reg),(str)));
    } else if (!strcasecmp(child, "port")) {
      if(NR_reg_set_uint2((reg),(UINT2)atoi(str)))
        FAIL(("NR_reg_set_uint2('%s','%s')",(reg),(str)));
    } else if (!strcasecmp(child, "netmaskbits")) {
      if(NR_reg_set_uchar((reg),(UCHAR)atoi(str)))
        FAIL(("NR_reg_set_uchar('%s','%s')",(reg),(str)));
    } else
        FAIL(("Unrecognized child '%s'", child));
}

static void
create_evil()
{
    if(nr_socket_evil_create2(test_socket, &signal_socket))
      FAIL(("nr_socket_evil_create2"));
}

static void
test(int flags)
{
    nr_socket_evil_test_reset();
    if(nr_socket_sendto(signal_socket,"hello",5,0,&server_addr))
     FAIL(("nr_socket_sendto %s",server_addr.as_string));
    nr_socket_evil_test_results(flags,&server_addr);
}

static void
testto(int flags, char *addr, int port)
{
    nr_transport_addr toaddr;
    nr_socket_evil_test_reset();
    if(nr_ip4_port_to_transport_addr(ntohl(inet_addr(addr)),port,IPPROTO_UDP,&toaddr))
      FAIL(("nr_ip4_port_to_transport_addr"));
    if(nr_socket_sendto(signal_socket,"hello",5,0,&toaddr))
     FAIL(("nr_socket_sendto %s",toaddr.as_string));
    nr_socket_evil_test_results(flags,&toaddr);
}

static void test_pass_all()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.recv.0.action","pass");
   CREATE_EVIL();
   TEST(EXPECT_SEND|EXPECT_RECV);
}

static void test_pass_send_only()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   CREATE_EVIL();
   TEST(EXPECT_SEND);
}

static void test_pass_recv_only()
{
   START_TEST();
   FILTER("net.socket.evil.recv.0.action","pass");
   CREATE_EVIL();
   TEST(EXPECT_NOTHING);
}

static void test_pass_addr()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.send.0.address","1.2.3.4");
   FILTER("net.socket.evil.recv.0.action","pass");
   FILTER("net.socket.evil.recv.0.address","1.2.3.4");
   CREATE_EVIL();
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 4321);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 4321);
   TEST(EXPECT_NOTHING);
}

static void test_pass_send_addr_only()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.send.0.address","1.2.3.4");
   CREATE_EVIL();
   TESTTO(EXPECT_SEND, "1.2.3.4", 1234);
   TESTTO(EXPECT_SEND, "1.2.3.4", 4321);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 4321);
   TEST(EXPECT_NOTHING);
}

static void test_pass_recv_addr_only()
{
   START_TEST();
   FILTER("net.socket.evil.recv.0.action","pass");
   FILTER("net.socket.evil.recv.0.address","1.2.3.4");
   CREATE_EVIL();
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 1234);
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 4321);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 4321);
   TEST(EXPECT_NOTHING);
}

static void test_pass_addr_port()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.send.0.address","1.2.3.4");
   FILTER("net.socket.evil.send.0.port","1234");
   FILTER("net.socket.evil.recv.0.action","pass");
   FILTER("net.socket.evil.recv.0.address","1.2.3.4");
   FILTER("net.socket.evil.recv.0.port","1234");
   CREATE_EVIL();
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 1234);
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 4321);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 1234);
   TEST(EXPECT_NOTHING);
}

static void test_pass_addr_mask()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.send.0.address","1.2.0.0");
   FILTER("net.socket.evil.send.0.netmaskbits","16");
   FILTER("net.socket.evil.recv.0.action","pass");
   FILTER("net.socket.evil.recv.0.address","1.2.0.0");
   FILTER("net.socket.evil.recv.0.netmaskbits","16");
   CREATE_EVIL();
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 1);
   TESTTO(EXPECT_NOTHING, "1.0.255.0", 1);
   TESTTO(EXPECT_NOTHING, "1.1.0.0", 1);
   TESTTO(EXPECT_NOTHING, "1.3.0.0", 1);
   TESTTO(EXPECT_NOTHING, "1.4.0.0", 1);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 1);
   TEST(EXPECT_NOTHING);
}

static void test_pass_addr_mask_port()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","pass");
   FILTER("net.socket.evil.send.0.address","1.2.0.0");
   FILTER("net.socket.evil.send.0.port","1234");
   FILTER("net.socket.evil.send.0.netmaskbits","16");
   FILTER("net.socket.evil.recv.0.action","pass");
   FILTER("net.socket.evil.recv.0.address","1.2.0.0");
   FILTER("net.socket.evil.recv.0.port","1234");
   FILTER("net.socket.evil.recv.0.netmaskbits","16");
   CREATE_EVIL();
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.255.4", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.0.4", 1234);
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 4321);
   TESTTO(EXPECT_NOTHING, "1.0.255.0", 1234);
   TESTTO(EXPECT_NOTHING, "1.1.0.0", 1234);
   TESTTO(EXPECT_NOTHING, "1.3.0.0", 1234);
   TESTTO(EXPECT_NOTHING, "1.4.0.0", 1234);
   TESTTO(EXPECT_NOTHING, "127.0.0.1", 1234);
   TEST(EXPECT_NOTHING);
}

static void test_drop_all()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","drop");
   FILTER("net.socket.evil.recv.0.action","drop");
   CREATE_EVIL();
   TEST(EXPECT_NOTHING);
}

static void test_drop_addr_mask_port()
{
   START_TEST();
   FILTER("net.socket.evil.send.0.action","drop");
   FILTER("net.socket.evil.send.0.address","1.2.0.0");
   FILTER("net.socket.evil.send.0.port","1234");
   FILTER("net.socket.evil.send.0.netmaskbits","16");
   FILTER("net.socket.evil.send.1.action","pass");
   FILTER("net.socket.evil.recv.0.action","drop");
   FILTER("net.socket.evil.recv.0.address","1.2.0.0");
   FILTER("net.socket.evil.recv.0.port","1234");
   FILTER("net.socket.evil.recv.0.netmaskbits","16");
   FILTER("net.socket.evil.recv.1.action","pass");
   CREATE_EVIL();
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 1234);
   TESTTO(EXPECT_NOTHING, "1.2.255.4", 1234);
   TESTTO(EXPECT_NOTHING, "1.2.0.4", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 4321);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.255.4", 4321);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.0.4", 4321);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.0.255.0", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.1.0.0", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.3.0.0", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.4.0.0", 1234);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "127.0.0.1", 1234);
   TEST(EXPECT_SEND|EXPECT_RECV);
}

static void test_always_pass()
{
   START_TEST();
   CREATE_EVIL();
   /* override registry settings */
   nr_socket_evil_always_pass(signal_socket);
   TESTTO(EXPECT_SEND|EXPECT_RECV, "1.2.3.4", 4321);
   TEST(EXPECT_SEND|EXPECT_RECV);
}

static void test_always_drop()
{
   START_TEST();
   CREATE_EVIL();
   /* override registry settings */
   nr_socket_evil_always_drop(signal_socket);
   TESTTO(EXPECT_NOTHING, "1.2.3.4", 4321);
   TEST(EXPECT_NOTHING);
}


static void run_tests()
{
    test_pass_all();
    test_pass_send_only();
    test_pass_recv_only();
    test_pass_addr();
    test_pass_send_addr_only();
    test_pass_recv_addr_only();
    test_pass_addr_port();
    test_pass_addr_mask();
    test_pass_addr_mask_port();
    test_drop_all();
    test_drop_addr_mask_port();
    test_always_pass();
    test_always_drop();
}

int main(int argc, char **argv)
  {
    int r;

    if(r=NR_reg_init(NR_REG_MODE_LOCAL))
      nr_verr_exit("Couldn't initialize registry");

    r_log_init();

    if(r=nr_ip4_port_to_transport_addr(ntohl(inet_addr(server_host)),server_port,IPPROTO_UDP,&server_addr))
          nr_verr_exit("Couldn't create server addr");

    if(r=nr_ip4_port_to_transport_addr(ntohl(inet_addr(client_host)),client_port,IPPROTO_UDP,&client_addr))
          nr_verr_exit("Couldn't create client addr");

    if(r=nr_socket_create_int((void*)0, &nr_socket_evil_test_vtbl, &test_socket)) 
          nr_verr_exit("Couldn't create test socket");

    setvbuf(stdout, 0, _IONBF, BUFSIZ);

    run_tests();

    if (failed > 0 || succeeded == 0) {
        fprintf(stderr,"Failed (%d succeeded, %d failed)\n", succeeded, failed);
        exit(1);
    }

    fprintf(stderr,"Success! (%d succeeded)\n", succeeded);
    exit(0);
  }


    
  


