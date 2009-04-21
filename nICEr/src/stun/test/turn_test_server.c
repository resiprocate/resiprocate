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



static char *RCSSTRING __UNUSED__="$Id: turn_test_server.c,v 1.2 2008/04/28 18:21:31 ekr Exp $";

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

int nr_xdump(char *name,UCHAR *data, int len);

int LOG_TURN_SERVER = 0;
time_t today;
char nonce[20];
Data user={(UCHAR*)"user",4};
Data pass={(UCHAR*)"pass",4};
int relays = 0;

typedef struct
{
      UINT2 port;
      UINT4 addr;
} StunAddress4;

typedef struct
{
      UCHAR pad;
      UCHAR family;
      StunAddress4 ipv4;
} StunAtrAddress4;

struct relay_info {
  nr_transport_addr relay_addr;
  int relay_port;
  nr_socket *relay_sock;
  StunAddress4 client_addr;
  nr_transport_addr remote_addr;
};

static void relay_cb(int s, int how, void *cb_arg);

static void s_cb(int s, int how, void *cb_arg)
  {
#if 0
    nr_socket *sock = (nr_socket*)cb_arg;
    UCHAR buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    int fd;
    struct sockaddr_in6 addr6;
    unsigned int namelen=sizeof(addr6);
    nr_transport_addr my_addr;
    int r;
    StunMessage req;
    StunMessage res;
    StunAddress4 from;
    struct relay_info *info;

    fprintf(stderr,"TURN_SERVER: SERVER CB\n");

    if(r=nr_socket_recvfrom(sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    memset(&from, 0, sizeof(from));
    from.addr = ntohl(addr2.u.addr4.sin_addr.s_addr);
    from.port = ntohs(addr2.u.addr4.sin_port);

    if ((r=nr_stun_parse_message((char*)buf2, len, &req))) {
      fprintf(stderr,"Error in nr_stun_parse_message\n");
      exit(1);
    }

    if ((r=nr_stun_process_request(&req, buf2, len, &addr2, 0, 0, &res))) {
        /* failure is expected because the server code doesn't support TURN */
        if (r!=R_FAILED) {
            fprintf(stderr,"Failed to process message!\n");
            exit(1);
        }
        fprintf(stderr,"TURN_SERVER: No problem with parse failure ... process TURN in test server code\n");
    }

    if (!nr_is_stun_message((char*)buf2, len)) {
        fprintf(stderr,"TURN_SERVER: not a STUN/TURN message\n");
        /* instead of sending to remoteAddress, just echo the content back
         * for the purposes of our tests */
        goto send;
    }

    res.hasFingerprint = 1;

    switch (req.msgHdr.msgType) {
    case AllocateRequestMsg:
       if (!req.hasNonce||!req.hasRealm||!req.hasUsername||!req.hasMessageIntegrity) {
           fprintf(stderr, "Received AllocateRequestMsg #1\n");
           assert(!req.hasMessageIntegrity);
/* TODO: what about username: does it go into the req or not? spec not clear */
           res.msgHdr.msgType = AllocateErrorResponseMsg;
           res.hasUsername = 0;
           res.hasNonce = 1;
           strcpy(res.nonce, "NONCE");
           res.hasRealm = 1;
           strcpy(res.realm, "REALM");
           res.hasErrorCode = 1;
           res.errorCode.errorClass = 4;
           res.errorCode.number = 35;
       }
       else {
           fprintf(stderr, "Received AllocateRequestMsg #2\n");
           assert(req.hasUsername);
           assert(req.hasRealm);
           assert(req.hasNonce);
           assert(req.hasMessageIntegrity);
           res.msgHdr.msgType = AllocateResponseMsg;
           res.hasUsername = 1;
           res.hasMessageIntegrity = 1;
           strcpy(res.username, req.username);
           res.hasXorMappedAddress = 1;

           nr_ip4_port_to_transport_addr(from.addr^(req.msgHdr.magicCookie), from.port^(req.msgHdr.magicCookie>>16), IPPROTO_UDP, &res.xorMappedAddress);

           nr_socket_getfd(sock, &fd);
           if(getsockname(fd,(struct sockaddr *)&addr6,&namelen)<0){
             fprintf(stderr,"Couldn't get socket address\n");
             exit(1);
           }
           nr_sockaddr_to_transport_addr((struct sockaddr *)&addr6,namelen,
                                         addr2.protocol,1,&my_addr);

           info = calloc(1,sizeof(*info));
           info->relay_port = 3478 + (++relays);
           nr_ip4_port_to_transport_addr(ntohl(my_addr.u.addr4.sin_addr.s_addr), info->relay_port, addr2.protocol, &info->relay_addr);

           if(r=nr_socket_local_create(&info->relay_addr,&info->relay_sock)){
             fprintf(stderr,"Couldn't create relay socket\n");
             exit(1);
           }

           /* Now set an async cb */
           nr_socket_getfd(info->relay_sock,&fd);

           NR_ASYNC_WAIT(fd,NR_ASYNC_WAIT_READ,relay_cb,info);
 
           res.hasRelayAddress = 1;
           nr_transport_addr_copy(&res.relayAddress, &info->relay_addr);

           info->client_addr.addr = from.addr;
           info->client_addr.port = from.port;

           fprintf(stderr,"TURN-SERVER: Allocating relay address %s to client %s\n", info->relay_addr.as_string, addr2.as_string);
       }
       break;

#if 0
//TODO:cleanup, obsolete
    case SendIndicationMsg:
       fprintf(stderr, "Received SendIndicationMsg\n");
       assert(req.hasRemoteAddress);

       /* pretend to send empty UDP packet to remoteAddress per the
        * TURN spec and to wait for a DataIndication resonse */
       fprintf(stderr, "Sending UDP packet to REMOTE-ADDRESS...\n");
       fprintf(stderr, "Waiting for response from REMOTE-ADDRESS...\n");

       /* ok, this is an indication, so formally there is no "response",
        * but we're going to send a data indication to it, so just pretend
        * that it's a formal response */
       res.msgHdr.msgType = DataIndicationMsg;
       res.hasRemoteAddress = 1;
       res.remoteAddress.family    = req.remoteAddress.family;
       res.remoteAddress.ipv4.addr = req.remoteAddress.ipv4.addr;
       res.remoteAddress.ipv4.port = req.remoteAddress.ipv4.port;

       break;
    case SetActiveDestRequestMsg:
       fprintf(stderr, "Received SetActiveDestRequestMsg\n");
       assert(req.hasRemoteAddress);
 
       res.msgHdr.msgType = SetActiveDestResponseMsg;
       break;
    case DataIndicationMsg:
assert(0);
       break;
#endif
    default:
       assert(0);
       break;
    }

   memset(&buf2, 0, sizeof(buf2));

   if ((r=nr_stun_encode_message(STUN_MODE_STUN, &res, (char*)buf2, STUN_MAX_MESSAGE_SIZE, &pass, (unsigned int*)&len))) {
      fprintf(stderr,"Error encoding TURN response\n");
      exit(1);
    }

 send:
   fprintf(stderr,"Sending response to %s\n", addr2.as_string);
   if(r=nr_socket_sendto(sock,buf2,len,0,&addr2)) {
      fprintf(stderr,"Error sending TURN response\n");
    }

    NR_ASYNC_WAIT(s, how, s_cb, cb_arg);
#else
UNIMPLEMENTED;
if (0) relay_cb(s,how,cb_arg); /* gets rid of compiler warning */
#endif
  }

static void relay_cb(int s, int how, void *cb_arg)
  {
#if 0
    struct relay_info *info = (struct relay_info *)cb_arg;
    nr_socket *sock = info->relay_sock;
    UCHAR buf1[4096];
    UCHAR buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    nr_transport_addr to;
    int r;
    nr_stun_client_data_indication_params params;
    StunMessage ind;
    StunMessage *ind2;
    StunAddress4 from;

    if(r=nr_socket_recvfrom(sock,buf1,sizeof(buf1),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    fprintf(stderr, "TURN-SERVER: Received relay message from client %s\n", addr2.as_string);
    
    memset(&ind, 0, sizeof(ind));
    memset(&from, 0, sizeof(from));
    from.addr = ntohl(addr2.u.addr4.sin_addr.s_addr);
    from.port = ntohs(addr2.u.addr4.sin_port);

    assert(info->client_addr.addr != 0 && info->client_addr.port != 0);

    if (from.addr == info->client_addr.addr && from.port == info->client_addr.port) {
        /* going from the client to the peer */
        fprintf(stderr, "TURN-SERVER: Message going from client to peer\n");

        if (!nr_is_stun_message((char*)buf1, len)) {
            fprintf(stderr,"not a STUN/TURN message\n");
            /* drop it on the floor */
            return;
        }

        if (nr_stun_parse_message((char*)buf1, len, &ind)) {
          fprintf(stderr,"Failed to process message!\n");
          exit(1);
        }

        if (ind.msgHdr.msgType != SendIndicationMsg) {
              fprintf(stderr,"Failed, wrong type of message!\n");
              exit(1);
        }

        if (!ind.hasRemoteAddress) {
              fprintf(stderr,"Failed, missing REMOTE-ADDR!\n");
              exit(1);
        }

        if (strlen(info->remote_addr.as_string)== 0) {
            nr_transport_addr_copy(&info->remote_addr, &ind.remoteAddress);
        }

        len = ind.data.length;
        memcpy(buf2, ind.data.data, len);

        nr_transport_addr_copy(&to, &info->remote_addr); 

        if (info->remote_addr.addr == 0) {
            fprintf(stderr, "TURN-SERVER: Setting remote peer address to %s\n", to.as_string);
        }
   }
   else {
        /* going from the peer to the client */
        fprintf(stderr, "TURN-SERVER: Message going from peer to client\n");
        if (info->remote_addr.addr == 0) {
            fprintf(stderr,"Don't know about peer yet, so ignoring unexpected packet\n");
            goto done;
        }
       
        nr_transport_addr_copy(&params.remote_addr, &info->remote_addr); 
        params.data.data = buf1;
        params.data.len = len;

        if (nr_stun_encode_data_indication(buf2, sizeof(buf2), (int*)&len, &params, (void*)&ind2)) {
              fprintf(stderr,"Failed, couldn't encode message!\n");
              exit(1);
        }

        nr_ip4_port_to_transport_addr(info->client_addr.addr, info->client_addr.port, IPPROTO_UDP, &to);
   }

   fprintf(stderr,"Sending response to %s\n", to.as_string);
   nr_xdump("DATA",buf2,len);
   fflush(stdout);

   if(r=nr_socket_sendto(sock,buf2,len,0,&to)) {
      fprintf(stderr,"Error sending TURN response\n");
   }

done:
    NR_ASYNC_WAIT(s, how, relay_cb, cb_arg);
#else
UNIMPLEMENTED;
#endif
  }


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

    nr_crypto_openssl_set();

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

    /* Set up the TURN server */
    nr_ip4_port_to_transport_addr(0x7f000001, 3478, IPPROTO_UDP, &my_addr);
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
