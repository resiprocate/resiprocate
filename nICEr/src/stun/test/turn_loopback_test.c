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



static char *RCSSTRING __UNUSED__="$Id: turn_loopback_test.c,v 1.2 2008/04/28 18:21:31 ekr Exp $";

#ifdef USE_TURN

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
#include "turn_client_ctx.h"
#include "registry.h"
#include "stun.h"
#include <assert.h>
#include "nr_crypto_openssl.h"

int LOG_TURN_LOOPBACK_TEST = 0;
time_t today;
char nonce[20];
char *user="user";
Data pass={(UCHAR*)"pass",4};
nr_transport_addr remote_addr;

#include "turn_test_server_util.c"


void c_cb(int s, int how, void *cb_arg)
  {
    nr_turn_client_ctx *turn=cb_arg;
    UCHAR buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    int r;

    r_log(LOG_TURN_LOOPBACK_TEST,LOG_DEBUG,"TURN_LOOPBACK_TEST: CLIENT CB");

    if(r=nr_socket_recvfrom(turn->sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    if(r=nr_turn_client_process_response(turn, buf2, len, &addr2)){
        /* probably a retransmit or something */
        fprintf(stderr,"Error processing TURN response\n");
    }

    NR_ASYNC_WAIT(s, how, c_cb, cb_arg);
  }

void done_cb(int s, int how, void *cb_arg)
{
    nr_turn_client_ctx *turn=cb_arg; 
 
    fprintf(stderr,"TURN done_cb fired\n");
    if (turn->state != NR_TURN_CLIENT_STATE_ALLOCATED)
        fprintf(stderr,"TURN FAILED (%d)\n", turn->state);
    else 
        fprintf(stderr,"Success! \n");
    
    exit(turn->state != NR_TURN_CLIENT_STATE_ALLOCATED);
}

int main(int argc, char **argv)
  {
    int r;
    nr_socket *c_sock,*s_sock;
    int fd;
    nr_turn_client_ctx *c_turn;
    nr_transport_addr c_addr,s_addr;
    struct timeval tv;

    nr_crypto_openssl_set();

    nr_app_startup("turn_loopback_test",NR_APP_STARTUP_INIT_LOGGING|NR_APP_STARTUP_REGISTRY_LOCAL,&LOG_TURN_LOOPBACK_TEST,0,0);
NR_reg_set_char("logging.stderr.enabled", 1);
NR_reg_set_char("logging.syslog.enabled", 1);
NR_reg_set_string("logging.syslog.facility.turn_loopback_test.level", "debug");
NR_reg_set_string("logging.stderr.facility.turn_loopback_test.level", "debug");
NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
NR_reg_set_string("logging.syslog.facility.turn.level", "debug");
NR_reg_set_string("logging.stderr.facility.turn.level", "debug");
 
    NR_async_timer_init();
    gettimeofday(&tv,0);
    NR_async_timer_update_time(&tv);

    time(&today);
    strcpy(nonce, ctime(&today));
    nonce[strlen(nonce)-1] = '\0';

    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      10000,IPPROTO_UDP,&s_addr);
    if(r=nr_socket_local_create(&s_addr,&s_sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }

    nr_ip4_port_to_transport_addr(ntohl(inet_addr("1.2.3.4")),
      10000,IPPROTO_UDP,&remote_addr);

    /* Set up the TURN client */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      9999,IPPROTO_UDP,&c_addr);
    if(r=nr_socket_local_create(&c_addr,&c_sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }

    if(r=nr_turn_client_ctx_create("TEST",c_sock, 0, &s_addr,0, &c_turn)){
      fprintf(stderr,"Couldn't create TURN ctx\n");
      exit(1);
    }

    /* Start TURN */
    if(r=nr_turn_client_allocate(c_turn, user, &pass, 111111, 2222, done_cb, c_turn)){
      fprintf(stderr,"Couldn't start TURN\n");
      exit(1);
    }

    /* Now set an async cb */
    nr_socket_getfd(c_sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,c_cb,c_turn);
    nr_socket_getfd(s_sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,s_cb,s_sock);
    
    while(1){
      int events;
      struct timeval towait={0,50};

      if(r=NR_async_event_wait2(&events,&towait)){
#if 0
        if(r==R_EOD)
          break;
        
        if(r!=R_WOULDBLOCK){
          fprintf(stderr,"Error in event wait\n");
          exit(1);
        }
#endif
      }

      gettimeofday(&tv,0);
      NR_async_timer_update_time(&tv);
    }

    printf("Success!\n");

    exit(0);
  }

#else
#include <stdio.h>
int main()
{
    fprintf(stderr,"TURN not implemented\n");
    return 1;
}
#endif /* USE_TURN */
