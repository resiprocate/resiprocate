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



static char *RCSSTRING __UNUSED__="$Id: nr_socket_nat_test.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

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
#include <registry.h>
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "nr_socket_nat.h"

int ctr=0;

nr_transport_addr addr2;
nr_transport_addr *prime_addr;

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
      if(r==R_WOULDBLOCK){
        if(prime_addr){
          fprintf(stderr,"Was R_WOULDBLOCK, ignoring...\n");
          nr_transport_addr_copy(&addr2,prime_addr);
          prime_addr=0;
          strcpy(buf2,"IGNORE THIS");
          len=strlen(buf2);
          prime_addr=0;
        }
      }
      else
        exit(1);
    }

    /* Force a terminator */
    buf2[len]=0;
    printf("Received %s from [%s]\n",buf2,addr2.as_string);

    if(len<10)
      abort();

    sprintf(ctrbuf,"%.4d",ctr);
    memcpy(buf2,ctrbuf,strlen(ctrbuf));

    ctr++;

    if(ctr>=50)
      exit(0);

    /* Re-arm the callback */
    NR_ASYNC_WAIT(s,NR_ASYNC_WAIT_READ,recv_cb,sock);    

    /* Send and receive something to ourselves */
    if(r=nr_socket_sendto(sock,buf2,len,0,&addr2)){
      fprintf(stderr,"Error in sendto\n");
      exit(1);
    }
  }

int main(int argc, char **argv)
  {
    int r;
    nr_socket *sock;
    nr_socket *sock2;
    char buf1[]="This is a test";
    int log_facility;
    NR_SOCKET fd;
    nr_transport_addr addr1;
    nr_transport_addr local_addr;
    nr_transport_addr external_addr;


    nr_app_startup("nr_socket_nat_test",NR_APP_STARTUP_REGISTRY_LOCAL,&log_facility,0,0);

    NR_reg_set_char("nr_socket_nat.NAT.map_address_independent",1);
    NR_reg_set_char("nr_socket_nat.NAT.filter_address_independent",0);
    NR_reg_set_string("nr_socket_nat.NAT.network","10.0.0.0");
    NR_reg_set_uint4("nr_socket_nat.NAT.netmask",0xffffff00);
    NR_reg_set_string("nr_socket_nat.NAT.external_addr","127.0.0.1");
 

    /* NATed sock */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("10.0.0.1")),
      9999,IPPROTO_UDP,&addr1);
        
    if(r=nr_socket_nat_create(&addr1,&sock)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }
    nr_socket_getaddr(sock,&local_addr);

    /* This is a cheat, because we know how the NAT behaves 
       and that it doesn't translate ports */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      9999,IPPROTO_UDP,&external_addr);

    

    /* Regular sock */
    nr_ip4_port_to_transport_addr(ntohl(inet_addr("127.0.0.1")),
      10000,IPPROTO_UDP,&addr2);
    if(r=nr_socket_local_create(&addr2,&sock2)){
      fprintf(stderr,"Couldn't create socket\n");
      exit(1);
    }
    

    printf("Private address: %s\n",local_addr.as_string);

    /* Send and receive something to ourselves, starting with
       the un-NATted to the NATed */
    prime_addr=&addr2;
    if(r=nr_socket_sendto(sock2,buf1,sizeof(buf1),0,&external_addr)){
      fprintf(stderr,"Error in sendto\n");
      exit(1);
    }

    
    /* Now set an async cb */
    nr_socket_getfd(sock,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,recv_cb,sock);
    nr_socket_getfd(sock2,&fd);
    NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,recv_cb,sock2);

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
