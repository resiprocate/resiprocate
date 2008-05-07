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



static char *RCSSTRING __UNUSED__="$Id: stun_loopback_test.c,v 1.2 2008/04/28 18:21:31 ekr Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nr_api.h>
#include <nr_startup.h>
#include "nr_socket.h"
#include "async_timer.h"
#include "nr_socket_local.h"
#include "stun_client_ctx.h"
#include "stun_server_ctx.h"
#include "registry.h"
#include <assert.h>
#include "nr_crypto_openssl.h"
#include "stun_reg.h"

int ctr=0;
nr_transport_addr peer_addr;
static int done = 0;
static int auth_mode = NR_STUN_AUTH_RULE_SHORT_TERM;

void c_cb(int s, int how, void *cb_arg)
  {
    nr_stun_client_ctx *stun=cb_arg;
    UCHAR buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    int r;

    if(r=nr_socket_recvfrom(stun->sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    if(r=nr_stun_client_process_response(stun, buf2, len, &addr2)){
      fprintf(stderr,"Error processing STUN response: %s\n",nr_strerror(r));
      if(r==R_RETRY) {
        if(r=nr_stun_client_restart(stun)){
          fprintf(stderr,"Unable to retry STUN\n");
          exit(1);
        }
      }

    }

    NR_ASYNC_WAIT(s,NR_ASYNC_WAIT_READ,c_cb,stun);
  }

void s_cb(int s, int how, void *cb_arg)
  {
    nr_stun_server_ctx *stun=cb_arg;
    char buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    int r;

    if(r=nr_socket_recvfrom(stun->sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    if(r=nr_stun_server_process_request(stun, stun->sock, buf2, len, &addr2, auth_mode)){
      fprintf(stderr,"Error processing STUN request\n");
    }

    NR_ASYNC_WAIT(s, how, s_cb, cb_arg);
  }



int s_cb2(void *cb_arg,nr_stun_server_ctx *srvr,nr_socket *sock, nr_stun_server_request *req, int *err)
  {
    printf("STUN user cb fired: %s\n",(char *)cb_arg);

    return(0);
  }

void done_cb(int s, int how, void *cb_arg)
  {
    nr_stun_client_ctx *stun=cb_arg; 
    printf("STUN done_cb fired\n");
    if (stun->state != NR_STUN_CLIENT_STATE_DONE) {
        printf("Failed!!\n");
        exit(1);
    }
    done = 1;
  }

void usage(char *argv0)
  {
      fprintf(stderr,"usage: %s [-n|-s|-l]\n", argv0);
      fprintf(stderr,"        -n: no authentication\n");
      fprintf(stderr,"        -s: short-term authentication (default)\n");
      fprintf(stderr,"        -l: long-term authentication\n");
      exit(1);
  }

int main(int argc, char **argv)
  {
    int r;
    nr_socket *c_sock,*s_sock;
    int fd;
    nr_transport_addr c_addr,s_addr;
    nr_stun_client_ctx *c_stun;
    nr_stun_server_ctx *s_stun;
    char *user="user";
    Data pass={(UCHAR*)"pass",4};
    struct timeval tv;
    extern int NR_LOG_STUN;

    if (argc == 1) {
        auth_mode = NR_STUN_AUTH_RULE_SHORT_TERM;
    }
    else if (argc == 2) {
        if (!strcmp(argv[1], "-l"))
            auth_mode = NR_STUN_AUTH_RULE_LONG_TERM;
        else if (!strcmp(argv[1], "-s"))
            auth_mode = NR_STUN_AUTH_RULE_SHORT_TERM;
        else if (!strcmp(argv[1], "-n"))
            auth_mode = NR_STUN_AUTH_RULE_OPTIONAL;
        else
            usage(argv[0]);
    }
    else {
        usage(argv[0]);
    }

    nr_crypto_openssl_set();

    nr_app_startup("nr_socket_test",NR_APP_STARTUP_INIT_LOGGING|NR_APP_STARTUP_REGISTRY_LOCAL,&NR_LOG_STUN,0,0);
NR_reg_set_char("logging.stderr.enabled", 1);
NR_reg_set_char("logging.syslog.enabled", 1);
NR_reg_set_string("logging.syslog.facility.nr_socket_test.level", "debug");
NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
NR_reg_set_string("logging.stderr.facility.nr_socket_test.level", "debug");
NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
 
    NR_reg_set_string(NR_STUN_REG_PREF_SERVER_REALM, "REALM");

    NR_async_timer_init();
    gettimeofday(&tv,0);
    NR_async_timer_update_time(&tv);

    /* Set up the STUN server */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      10000,IPPROTO_UDP,&s_addr);
    if(r=nr_socket_local_create(&s_addr,&s_sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }
    if(r=nr_stun_server_ctx_create("SERVER",s_sock, &s_stun)){
      fprintf(stderr,"Couldn't create stun server\n");
      exit(1);
    }
    if(r=nr_stun_server_add_client(s_stun, "Test", user, &pass, s_cb2, "TestUser")){
      fprintf(stderr,"Couldn't add stun user\n");
      exit(1);
    }

    /* Set up the STUN client */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      9999,IPPROTO_UDP,&c_addr);
    if(r=nr_socket_local_create(&c_addr,&c_sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }
    if(r=nr_stun_client_ctx_create("TEST",c_sock,&s_addr,0,&c_stun)){
      fprintf(stderr,"Couldn't create STUN ctx\n");
      exit(1);
    }

    c_stun->params.stun_binding_request.username = user;
    c_stun->params.stun_binding_request.password = &pass;

    /* Start STUN */
    if (auth_mode == NR_STUN_AUTH_RULE_LONG_TERM) {
      if(r=nr_stun_client_start(c_stun, NR_STUN_CLIENT_MODE_BINDING_REQUEST_LONG_TERM_AUTH, done_cb, c_stun)){
        fprintf(stderr,"Couldn't start STUN\n");
        exit(1);
      }
    }
    else if (auth_mode == NR_STUN_AUTH_RULE_SHORT_TERM) {
      if(r=nr_stun_client_start(c_stun, NR_STUN_CLIENT_MODE_BINDING_REQUEST_SHORT_TERM_AUTH, done_cb, c_stun)){
        fprintf(stderr,"Couldn't start STUN\n");
        exit(1);
      }
    }
    else {
      if(r=nr_stun_client_start(c_stun, NR_STUN_CLIENT_MODE_BINDING_REQUEST_NO_AUTH, done_cb, c_stun)){
        fprintf(stderr,"Couldn't start STUN\n");
        exit(1);
      }
    }

    /* Now set an async cb */
    nr_socket_getfd(c_sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,c_cb,c_stun);
    nr_socket_getfd(s_sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,s_cb,s_stun);
    
    while(!done){
      int events;
      struct timeval towait={0,50};

      if(r=NR_async_event_wait2(&events,&towait)){
        if(r==R_EOD)
          break;
        
        if(r!=R_WOULDBLOCK){
          fprintf(stderr,"Error in event wait\n");
          exit(1);
        }
      }

      gettimeofday(&tv,0);
      NR_async_timer_update_time(&tv);
    }

    printf("Success!\n");

    exit(0);
  }
