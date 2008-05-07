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



static char *RCSSTRING __UNUSED__="$Id: turn_test_echo_server.c,v 1.2 2008/04/28 18:21:31 ekr Exp $";

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


int LOG_TURN_SERVER = 0;
time_t today;
char nonce[20];
Data user={(UCHAR*)"user",4};
Data pass={(UCHAR*)"pass",4};

#include "turn_test_server_util.c"

void
usage()
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "./turn_server\n");
    exit(1);
}

int main(int argc, char **argv)
  {
    int r;
    nr_transport_addr my_addr;
    nr_socket *my_sock;
    int fd;
    struct timeval tv;

    if (argc != 1) 
        usage();

    nr_app_startup("turn_server",NR_APP_STARTUP_INIT_LOGGING|NR_APP_STARTUP_REGISTRY_LOCAL,&LOG_TURN_SERVER,0,0);
NR_reg_set_char("logging.stderr.enabled", 1);
NR_reg_set_char("logging.syslog.enabled", 1);
NR_reg_set_string("logging.syslog.facility.turn_client.level", "debug");
NR_reg_set_string("logging.stderr.facility.turn_client.level", "debug");
NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
NR_reg_set_string("logging.syslog.facility.turn.level", "debug");
NR_reg_set_string("logging.stderr.facility.turn.level", "debug");
 
    NR_async_timer_init();
    gettimeofday(&tv,0);
    NR_async_timer_update_time(&tv);

    /* Set up the TURN client */
    nr_ip4_port_to_transport_addr(ntohl(0), 3478, IPPROTO_UDP, &my_addr);
    if(r=nr_socket_local_create(&my_addr,&my_sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }

    /* Now set an async cb */
    nr_socket_getfd(my_sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,s_cb,my_sock);
 
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
