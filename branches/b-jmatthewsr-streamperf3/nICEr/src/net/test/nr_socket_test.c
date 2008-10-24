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



static char *RCSSTRING __UNUSED__="$Id: nr_socket_test.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

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
#include "nr_socket_local.h"

int ctr=0;
nr_transport_addr addr;

void recv_cb(int s, int how, void *cb_arg)
  {
    nr_socket *sock=cb_arg;
    char buf2[4096];
    char ctrbuf[10];
    size_t len;
    nr_transport_addr addr2;
    int r;

    if(r=nr_socket_recvfrom(sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    /* Force a terminator */
    buf2[len]=0;
    printf("Received %s\n",buf2);

    if(len<10)
      abort();

    sprintf(ctrbuf,"%.4d",ctr);
    memcpy(buf2,ctrbuf,strlen(ctrbuf));

    ctr++;

    if(ctr>=50)
      return;

    /* Re-arm the callback */
    NR_ASYNC_WAIT(s,NR_ASYNC_WAIT_READ,recv_cb,sock);    

    /* Send and receive something to ourselves */
    if(r=nr_socket_sendto(sock,buf2,len,0,&addr)){
      fprintf(stderr,"Error in sendto\n");
      exit(1);
    }
  }

int main(int argc, char **argv)
  {
    int r;
    nr_socket *sock;
    char buf1[]="This is a test";
    int log_facility;
    NR_SOCKET fd;

    nr_app_startup("nr_socket_test",0,&log_facility,0,0);

    /* Fake up an address */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      9999,IPPROTO_UDP,&addr);
    
    /* sockaddr to send to */
    if(r=nr_socket_local_create(&addr,&sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }

    /* Send and receive something to ourselves */
    if(r=nr_socket_sendto(sock,buf1,sizeof(buf1),0,&addr)){
      fprintf(stderr,"Error in sendto\n");
      exit(1);
    }

    /* Now set an async cb */
    nr_socket_getfd(sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,recv_cb,sock);

    while(1){
      int events;

      if(r=NR_async_event_wait(&events)){
        if(r=R_EOD)
          break;
        
        fprintf(stderr,"Error in event wait\n");
        exit(1);
      }
    }

    printf("Success!\n");

    exit(0);
  }
