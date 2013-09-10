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


static char *RCSSTRING __UNUSED__="$Id: ice_test.c,v 1.3 2008/04/28 19:37:00 ekr Exp $";

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
#include <nr_api.h>
#include <nr_startup.h>
#include <async_wait.h>
#include <async_timer.h>
#include <registry.h>
#include "ice_util.h"
#include "transport_addr.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "stun_client_ctx.h"
#include "stun_server_ctx.h"

#include "ice_ctx.h"
#include "ice_candidate.h"
#include "ice_handler.h"

#include "nr_crypto_openssl.h"

int done=0;
int mode=0;
#define MODE_OFFERER  1
#define MODE_ANSWERER 2
int stream_ct=1;
int dont_start=0;
int force_controlling=0;

void *reg_mode;
void *timer;
int signal_port=5160;
int local_port=0;
char *signal_host="127.0.0.1";
nr_socket *signal_socket;
nr_transport_addr local_addr;
nr_transport_addr peer_addr;
NR_SOCKET signal_fd;
char stream_infos[256][80] = { { 0 } };
int stream_infos_ct = 0;
nr_ice_peer_ctx *peers[256] = { 0 };
nr_ice_media_stream *streams[256] = { 0 };
int component_ids[256] = { 0 };
int impatient_mode=0;

typedef struct impatient_info_ {
  void *timer;
  nr_ice_cand_pair *chosen;
} impatient_info;

typedef struct test_ice_ctx_ {
  int index; /* HACK HACK HACK */
  char *label;
  nr_ice_ctx *ctx;
  nr_ice_peer_ctx *pctx;
  nr_ice_media_stream *streams[10];
  impatient_info impatient[10][10];
  int stream_ct;
  nr_ice_handler handler;
} test_ice_ctx;

test_ice_ctx *g_tctx;

char attr_buf[2048];



void nr_test_ice_impatient_timer_cb(int s, int how, void *cb_arg)
  {
    impatient_info *info=cb_arg;
    
    fprintf(stderr,"Impatient timer expired, choosing %s",info->chosen->as_string);
    nr_ice_candidate_pair_select(info->chosen);
    info->timer=0;
    info->chosen=0;
  }

/*** ice_handler impl */
  static int nr_test_ice_select_pair(void *obj,nr_ice_media_stream *stream, 
int component_id, nr_ice_cand_pair **potentials,int potential_ct)
  {
    int i;
    int stream_num=-1;
    nr_ice_cand_pair *chosen=0;
    int incomplete=0;

    test_ice_ctx *tctx=obj;

    for(i=0;i<tctx->stream_ct;i++){
      if(tctx->streams[i]==stream->local_stream){
        stream_num=i;
        break;
      }
    }
    assert(stream_num!=-1);
      
    fprintf(stderr,"nr_test_ice_select_pair called.... stream=%s:%d\n",stream->label,component_id);
    
    for(i=0;i<potential_ct;i++){
      fprintf(stderr,"  %s state=%d\n",potentials[i]->as_string,potentials[i]->state);
      fprintf(stderr,"      priority=%llu\n",potentials[i]->priority);
      fprintf(stderr,"          local %s priority %u type=%d\n",potentials[i]->local->addr.as_string,potentials[i]->local->priority,potentials[i]->local->type);
      fprintf(stderr,"          remote %s priority %u type=%d\n",potentials[i]->remote->addr.as_string,potentials[i]->remote->priority,potentials[i]->remote->type);
      if(potentials[i]->state==NR_ICE_PAIR_STATE_SUCCEEDED){
        if(!chosen)
          chosen=potentials[i];
      }
      else if((potentials[i]->state!=NR_ICE_PAIR_STATE_FAILED) && 
        (potentials[i]->state!=NR_ICE_PAIR_STATE_CANCELLED))
        incomplete++;
    }
    
    if(!chosen)
      return R_NOT_FOUND;

    if(incomplete){
      if(impatient_mode && chosen){
        if(tctx->impatient[stream_num][component_id].timer==0){
          tctx->impatient[stream_num][component_id].chosen=chosen;
          NR_ASYNC_TIMER_SET(2000,nr_test_ice_impatient_timer_cb,
            &tctx->impatient[stream_num][component_id],
            &tctx->impatient[stream_num][component_id].timer);
        }
      }
      return R_NOT_FOUND;
    }

    fprintf(stderr,"nr_test_ice_select_pair choosing: %s\n", chosen->as_string);
    if(tctx->impatient[stream_num][component_id].timer!=0){
      NR_async_timer_cancel(tctx->impatient[stream_num][component_id].timer);
      tctx->impatient[stream_num][component_id].chosen=0;
      tctx->impatient[stream_num][component_id].timer=0;
    }

    if(nr_ice_candidate_pair_select(chosen)){
      fprintf(stderr,"Couldn't select pair!!!\n");
      return R_NOT_FOUND;
    }

    return(0);
  }

static int nr_test_ice_stream_ready(void *obj, nr_ice_media_stream *stream)
  {
    fprintf(stderr,"nr_test_ice_stream_ready called.... stream=%s\n",stream->label);

    return(0);
  }

static int nr_test_ice_stream_failed(void *obj, nr_ice_media_stream *stream)
  {
    fprintf(stderr,"nr_test_ice_stream_failed called.... stream=%s\n",stream->label);
    
    fprintf(stderr,"FAILURE\n");
    exit(1);

    return(0);
  }

static void bail(int sig)
{
    int i;
    int fail = 0;

    if (stream_infos_ct == 0) {
       fprintf(stderr,"ERROR: never created any streams\n");
       exit(1);
    }

    for (i = 0; i < stream_infos_ct; ++i) {
        if (strlen(stream_infos[i]) > 0) {
            fail = 1;
            fprintf(stderr,"ERROR: never recieved data on %s\n", stream_infos[i]);
        }
    }

    exit(fail);
}

static void send_data(int a, int b, void *v)
{
    int i;
    int r;

    if(done)
      return;

    for (i = 0; i < stream_infos_ct; ++i) {
        if ((r=nr_ice_media_stream_send(peers[i], streams[i], component_ids[i], (UCHAR*)stream_infos[i], strlen(stream_infos[i]))))
            fprintf(stderr,"Error sending to stream\n");
    }

    NR_ASYNC_TIMER_SET(500,send_data,0,&timer);
}

static int nr_test_ice_completed(void *obj, nr_ice_peer_ctx *pctx)
  {
    nr_ice_media_stream *s1, *s2, *s3;
    int i;

    nr_ice_ctx_finalize(pctx->ctx,pctx);
    

    fprintf(stderr,"nr_test_ice_completed.... pctx=%s\n",pctx->label);

    /* In a real stack, this information would need to be compared
       against the original SDP to determine if an update was
       required */
    fprintf(stderr,"Selected local addresses\n");
    STAILQ_FOREACH_SAFE(s1, &pctx->peer_streams, entry, s2){
      nr_ice_component *c1,*c2;
      int comp_num=1;

      fprintf(stderr,"Stream %s\n",s1->label);

      STAILQ_FOREACH_SAFE(c1, &s1->components, entry, c2){
        
        fprintf(stderr,"  Component %d: %s\n",comp_num,c1->active->local->addr.as_string);
      }
    }
        
    if(mode==MODE_OFFERER){
        /* generate traffic that'll be echoed back */
        STAILQ_FOREACH_SAFE(s1, &pctx->peer_streams, entry, s2){
            for(i = 1; i <= s1->component_ct; i++) {
                s3 = s1->local_stream;
                streams[stream_infos_ct] = s3;
                peers[stream_infos_ct] = pctx;
                component_ids[stream_infos_ct] = i;
                sprintf(stream_infos[stream_infos_ct], "stream %p component %d", s3, i);
                stream_infos_ct++;
            }
        }

        send_data(0,0,0);
        
        NR_ASYNC_TIMER_SET(1000,send_data,0,&timer);
    }

    return(0);
  }

  static int nr_test_ice_msg_recvd(void *obj, nr_ice_peer_ctx *pctx, nr_ice_media_stream *stream, int component_id, UCHAR *msg, int len)
  {
    int r;
    int i;
    char buffer[80];

    fprintf(stderr,"Test message received stream=%s:%d len=%d\n",stream->label,component_id,len);

    if(mode==MODE_ANSWERER){
        /* merely echo back, unless we're done */
        if (len == 4 && !strncmp((char*)msg, "done", 4))
            done = 1;
        else if ((r=nr_ice_media_stream_send(pctx, stream, component_id, msg, len)))
        {
            if (r==R_BAD_ARGS) {
                /* the problem is that the component isn't active yet... */
            }
            else {
                fprintf(stderr,"Error sending to stream\n");
                exit(1);
            }
        }
    }
    else {
        sprintf(buffer, "stream %p component %d", stream, component_id);

        for (i = 0; i < stream_infos_ct; ++i) {
            if (!strncmp(stream_infos[i], (char*)msg, len) && strlen(stream_infos[i])==len) {
                strcpy(stream_infos[i], "");
                break;
            }
        }

        if (i >= stream_infos_ct) {
            fprintf(stderr, "ERROR: didn't find %.*s\n", len, msg);
            exit(1);
        }

        done = 1; 
        for (i = 0; i < stream_infos_ct; ++i) {
            if (strlen(stream_infos[i]) > 0) {
                done = 0;
            }
        }

        if (done) {
            if ((r=nr_ice_media_stream_send(pctx, stream, component_id, (UCHAR*)"done", 4)))
                fprintf(stderr,"Error sending done to stream\n");
        }
    }

    if (done) {
        nr_ice_ctx *ictx=pctx->ctx;
              
        nr_ice_peer_ctx_destroy(&pctx);
        nr_ice_ctx_destroy(&ictx);
    }

    return(0);
  }


nr_ice_handler_vtbl test_handler_vtbl={
  nr_test_ice_select_pair,
  nr_test_ice_stream_ready,
  nr_test_ice_stream_failed,
  nr_test_ice_completed,
  nr_test_ice_msg_recvd
};


static int create_test_ice_ctx(char *label,int streams,UINT4 flags,test_ice_ctx **tctxp)
  {
    test_ice_ctx *tctx=0;
    int r;
    int i;
    
    tctx=RCALLOC(sizeof(test_ice_ctx));

    tctx->label=r_strdup(label);
    if(r=nr_ice_ctx_create(label,flags,&tctx->ctx))
      nr_verr_exit("Couldn't create ICE ctx");
    
    for(i=0;i<stream_ct;i++){
      char buf[1024];

      snprintf(buf,1024,"%s:%d",label,i);

      if(r=nr_ice_add_media_stream(tctx->ctx,buf,1,&tctx->streams[tctx->stream_ct++]))
        nr_verr_exit("Couldn't create media stream");
    }
    
    if(r=nr_ice_peer_ctx_create(tctx->ctx,&tctx->handler,label,&tctx->pctx))
      nr_verr_exit("Couldn't create peer ctx");

    tctx->handler.vtbl=&test_handler_vtbl;
    tctx->handler.obj=(void *)tctx;
    *tctxp=tctx;

    return(0);
  }
      

int write_attributes(test_ice_ctx *ctx, char *out, int maxlen)
  {
    int r;
    char **attrs;
    int attrct;
    int i,j;

    if(r=nr_ice_get_global_attributes(ctx->ctx,&attrs,&attrct))
      nr_verr_exit("Couldn't get global attributes");
    for(j=0;j<attrct;j++){
      snprintf(out,maxlen,"%s\n",attrs[j]);
      maxlen-=strlen(out); out+=strlen(out);
      RFREE(attrs[j]);
    }
    RFREE(attrs);

    for(i=0;i<ctx->stream_ct;i++){
      if(r=nr_ice_media_stream_get_attributes(ctx->streams[i],&attrs,&attrct))
        nr_verr_exit("Couldn't get attribute strings");

      snprintf(out,maxlen,"\n");
      maxlen-=strlen(out); out+=strlen(out);      

      for(j=0;j<attrct;j++){
        snprintf(out,maxlen,"%s\n",attrs[j]);
        maxlen-=strlen(out); out+=strlen(out);
        RFREE(attrs[j]);
      }

    }
    RFREE(attrs);
 
    return(0);
  }

// buf had better be null terminated
int _read_attributes(char **bufp,char **attrs,int *attr_ct,int maxct)
  {
    int ct=0;
    char *ptr;
    char *buf=*bufp;

    if(!buf)
      return(R_EOD);

    if(strlen(buf)==0)
      return(R_EOD);

    while(buf){
      ptr=strchr(buf,'\n');
      
      if(ptr)
        *ptr++=0;
    
      if(ct>=maxct)
        nr_verr_exit("Too many attrs");
      
      if(strlen(buf)==0)
        break;

      attrs[ct++]=r_strdup(buf);
      
      buf=ptr;
    }
    
    *attr_ct=ct;
    *bufp=ptr;

    return(0);
  }
int read_attributes(test_ice_ctx *ctx, char *buf)
  {
    int r;
    char *attrs[50];
    int attrct;
    int i=0,j;

    if(r=_read_attributes(&buf,attrs,&attrct,50))
      nr_verr_exit("Error reading global attributes");

    if(r=nr_ice_peer_ctx_parse_global_attributes(ctx->pctx,attrs,attrct))
      nr_verr_exit("Couldn't parse global attributes");
    
    for(j=0;j<attrct;j++)
      RFREE(attrs[j]);

    while(1){
      attrct=0;

      r=_read_attributes(&buf,attrs,&attrct,50);
      
      if(r==R_EOD)
        break;

      if(r)
        nr_verr_exit("Error reading stream attributes");
      
      if(r=nr_ice_peer_ctx_parse_stream_attributes(ctx->pctx,ctx->streams[i],attrs,attrct))
        nr_verr_exit("Couldn't parse peer attributes");
      
      for(j=0;j<attrct;j++)
        RFREE(attrs[j]);

      i++;
    }

    for(j=0;j<attrct;j++)
      RFREE(attrs[j]);
    
    return(0);
  }

void signalling_cb(int s, int how, void *cb_arg)
  {
    test_ice_ctx *ctx=cb_arg;
    char buf[2048];
    int len;
    int r;

    fprintf(stderr,"Signalling CB fired\n");
    
    if(r=nr_socket_recvfrom(signal_socket,buf,sizeof(buf),(size_t *)&len,0,&peer_addr))
      nr_verr_exit("Couldn't read from signalling socket");

    buf[len] = '\0';
    fprintf(stderr,"RECEIVED ATTRIBUTES=====\n%s=====\n",buf);

    if(r=read_attributes(ctx,buf))
      nr_verr_exit("Error reading attributes");
    
    if(mode==MODE_ANSWERER){
      write_attributes(ctx,attr_buf,sizeof(attr_buf));
      fprintf(stderr,"SENDING ATTRIBUTES=====\n%s=====\n",attr_buf);
      if(r=nr_socket_sendto(signal_socket,attr_buf,strlen(attr_buf)+1,0,&peer_addr))
        nr_verr_exit("Couldn't send signalling");
    }
    

    fprintf(stderr,"Signalling complete\n");

    fprintf(stderr,"=========================================================\n");
    fprintf(stderr,"Kicking off ICE %s\n",ctx->label);
    
    if(force_controlling)
      ctx->pctx->controlling=1;

    if(r=nr_ice_peer_ctx_pair_candidates(ctx->pctx))
      nr_verr_exit("Couldn't pair candidates");

    if(!dont_start){
      nr_ice_peer_ctx_dump_state(ctx->pctx,stderr);

      if(r=nr_ice_peer_ctx_start_checks(ctx->pctx))
        nr_verr_exit("Couldn't start checks");
    }
  }

void initialized_cb(int s, int how, void *cb_arg)
  {
    int r;
    char **attrs=0;
    int attrct;
    int i,j;
    test_ice_ctx *ctx=cb_arg;

    fprintf(stderr,"ICE initialized, ctx=%s\n",ctx->label);

    fprintf(stderr,"ICE attribute exchange for ctx %s\n",ctx->label);

    if(r=nr_ice_get_global_attributes(ctx->ctx,&attrs,&attrct))
      nr_verr_exit("Couldn't get global attributes");
    for(j=0;j<attrct;j++){
      fprintf(stderr,"  ATTR:%s\n",attrs[j]);
      RFREE(attrs[j]);
    } 
    RFREE(attrs);

    for(i=0;i<ctx->stream_ct;i++){
      if(r=nr_ice_media_stream_get_attributes(ctx->streams[i],&attrs,&attrct))
        nr_verr_exit("Couldn't get attribute strings");
      
      fprintf(stderr,"STREAM %d: %d attributes\n",i,attrct);
      
      for(j=0;j<attrct;j++){
        fprintf(stderr,"  ATTR:%s\n",attrs[j]);
        RFREE(attrs[j]);
      }
    }
    RFREE(attrs);
 
    if(mode==MODE_OFFERER){
      write_attributes(ctx,attr_buf,sizeof(attr_buf));
      fprintf(stderr,"SENDING ATTRIBUTES=====\n%s=====\n",attr_buf);
      if(r=nr_socket_sendto(signal_socket,attr_buf,strlen(attr_buf)+1,0,&peer_addr))
        nr_verr_exit("Couldn't send signalling");
    }

    NR_ASYNC_WAIT(signal_fd,NR_ASYNC_WAIT_READ,signalling_cb,g_tctx);

  }

void timed_exit(int s, int how, void *cb_arg)
  {
    printf("Success!\n");
    exit(0);
  }
    
int main(int argc, char **argv)
  {
    int r;
    struct timeval tv;
    int c;
    extern char *optarg;
    UINT4 flags=0;

    reg_mode=NR_REG_MODE_LOCAL;

    while((c=getopt(argc,argv,"oas:pACT:g:RI"))!=-1){
      switch(c){
        case 'o':
          mode=MODE_OFFERER;
          flags |= NR_ICE_CTX_FLAGS_OFFERER;
          break;
        case 'a':
          flags |= NR_ICE_CTX_FLAGS_ANSWERER;
          mode=MODE_ANSWERER;
          break;
        case 'A':
          flags |= NR_ICE_CTX_FLAGS_AGGRESSIVE_NOMINATION;
          break;
        case 's':
          stream_ct=atoi(optarg);
          break;
        case 'p':
          dont_start=1;
          break;
        case 'C':
          force_controlling=1;
          break;
        case 'I':
          impatient_mode=1;
          break;
        case 'g':
          signal_host=r_strdup(optarg);
          break;
        case 'R':
#ifdef NO_REG_RPC
          nr_verr_exit("Remote registry mode not supported");
#else
          reg_mode=NR_REG_MODE_REMOTE;
#endif
          break;
        default:
          nr_verr_exit("invalid arg");
      }
    }

    nr_crypto_openssl_set();

#ifdef WIN32
    {
      WSADATA wsaData;
      WORD wVersionRequested = MAKEWORD( 2, 2 );
      int err = WSAStartup( wVersionRequested, &wsaData );
    }
#endif

    if(r=NR_reg_init(reg_mode))
      nr_verr_exit("Couldn't initialize registry");

    r_log_init();
    
    switch(mode){
      case MODE_OFFERER:
        if(r=nr_ip4_port_to_transport_addr(ntohl(inet_addr(signal_host)),signal_port,IPPROTO_UDP,&peer_addr))
          nr_verr_exit("Couldn't create peer addr");

        if(r=nr_ip4_port_to_transport_addr(INADDR_ANY,0,IPPROTO_UDP,&local_addr))
          nr_verr_exit("Couldn't create local addr");
        if(r=nr_socket_local_create(&local_addr,&signal_socket))
          nr_verr_exit("Couldn't create signalling socket");
        break;
      case MODE_ANSWERER:
        if(r=nr_ip4_port_to_transport_addr(ntohl(inet_addr(signal_host)),signal_port,IPPROTO_UDP,&local_addr))
          nr_verr_exit("Couldn't create local addr");
        break;
      default:
        nr_verr_exit("Must specify a mode!");
    }

    if(r=nr_socket_local_create(&local_addr,&signal_socket))
      nr_verr_exit("Couldn't create signalling socket");
    nr_socket_getfd(signal_socket,&signal_fd);
    
if(reg_mode==NR_REG_MODE_LOCAL){
#if 0
NR_reg_set_char("logging.stderr.enabled", 1);
NR_reg_set_char("logging.syslog.enabled", 1);
NR_reg_set_string("logging.syslog.facility.nr_ice_test.level", "debug");
NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
NR_reg_set_string("logging.stderr.facility.nr_ice_test.level", "debug");
NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
#endif
#if 1
NR_reg_set_string("ice.stun.server.0.addr","192.168.223.2");
//NR_reg_set_string("ice.stun.server.0.addr","192.168.1.105");
//NR_reg_set_string("ice.stun.server.0.addr","64.69.76.23");
//NR_reg_set_string("ice.stun.server.0.addr","4.5.6.7");
//NR_reg_set_string("ice.stun.server.0.addr","198.144.201.197");
NR_reg_set_uint2("ice.stun.server.0.port",3478);
NR_reg_set_string("ice.stun.server.1.addr","192.168.1.105");
NR_reg_set_uint2("ice.stun.server.1.port",3478);
#endif
#if 0
NR_reg_set_string("ice.turn.server.0.addr","127.0.0.1");
NR_reg_set_uint2("ice.turn.server.0.port",3478);
NR_reg_set_bytes("ice.turn.server.0.username",(UCHAR *)"user",4);
NR_reg_set_bytes("ice.turn.server.0.password",(UCHAR *)"pass",4);
#endif
NR_reg_set_uchar("ice.pref.type.srv_rflx",100);
NR_reg_set_uchar("ice.pref.type.peer_rflx",105);
NR_reg_set_uchar("ice.pref.type.prflx",99);
NR_reg_set_uchar("ice.pref.type.host",125);
NR_reg_set_uchar("ice.pref.type.relayed",126);
#ifdef WIN32
NR_reg_set_uchar("ice.pref.interface.Local_Area_Connection", 255);
NR_reg_set_uchar("ice.pref.interface.Wireless_Network_Connection", 254);
#elif defined(LINUX)
NR_reg_set_uchar("ice.pref.interface.eth0", 255);
NR_reg_set_uchar("ice.pref.interface.eth1", 254);
//NR_reg_set_char("ice.suppress.interface.eth0",1);
#else  /* MacOSX, FreeBSD */
NR_reg_set_uchar("ice.pref.interface.rl0", 255);
NR_reg_set_uchar("ice.pref.interface.wi0", 254);
NR_reg_set_uchar("ice.pref.interface.lo0", 253);
NR_reg_set_uchar("ice.pref.interface.en1", 252);
NR_reg_set_uchar("ice.pref.interface.en0", 251);
NR_reg_set_uchar("ice.pref.interface.ppp", 250);
NR_reg_set_uchar("ice.pref.interface.ppp0", 249);
NR_reg_set_uchar("ice.pref.interface.en2", 248);
NR_reg_set_uchar("ice.pref.interface.en3", 247);
NR_reg_set_uchar("ice.pref.interface.em0", 251);
NR_reg_set_uchar("ice.pref.interface.em1", 252);
#endif
NR_reg_set_uint4("stun.client.retransmission_timeout", 100);
//NR_reg_set_uint4("stun.client.retransmission_timeout", 1000);
}

    NR_async_timer_init();
    gettimeofday(&tv,0);
    NR_async_timer_update_time(&tv);
    
    create_test_ice_ctx("OCTX",1,flags,&g_tctx);

    /* Now initialize ICE */
    if(r=nr_ice_initialize(g_tctx->ctx,initialized_cb,g_tctx)){
      if(r!=R_WOULDBLOCK)
        nr_verr_exit("Couldn't initialize ICE");
    }

#ifndef WIN32
    signal(SIGALRM, bail);
//    alarm(120);
#endif

    while(1){
      int events;
      struct timeval towait={0,10000};

      if(r=NR_async_event_wait2(&events,&towait)){
        if(r==R_EOD)
          break;
        
        if(r!=R_WOULDBLOCK){
          fprintf(stderr,"Error in event wait\n");
          exit(1);
        }
      }

      if(done){
        /* Done is set, exit in one second */
        NR_ASYNC_TIMER_SET(1000,timed_exit,0,0);
      }

      gettimeofday(&tv,0);
      NR_async_timer_update_time(&tv);
    }

    fprintf(stderr,"Success!\n");
    
    exit(0);
  }


    
  


