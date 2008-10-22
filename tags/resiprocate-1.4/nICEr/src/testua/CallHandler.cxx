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


#include  "CallHandler.hxx"

// ICE
extern "C" {
#include <nr_api.h>
#include <sys/queue.h>
#include <async_wait.h>
#include <async_timer.h>
#include <registry.h>
#include <nr_startup.h>
}

#include "ice_ctx.h"
#include "ice_reg.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static int initialized=0;



/*** ice_handler impl */
/* Only return if all candidates have been checked */
  static int nr_ua_ice_select_pair(void *obj,nr_ice_media_stream *stream, 
  int component_id, nr_ice_cand_pair **potentials,int potential_ct)
  {
    int i;
    int chosen=0;
    fprintf(stderr,"nr_ua_ice_select_pair called.... stream=%s:%d\n",stream->label,component_id);

    for(i=0;i<potential_ct;i++){
      fprintf(stderr,"  %s state=%d\n",potentials[i]->as_string,potentials[i]->state);
      if(potentials[i]->state==NR_ICE_PAIR_STATE_SUCCEEDED){
        if(!chosen){
          nr_ice_candidate_pair_select(potentials[i]);
          chosen=1;
        }
      }
      else if((potentials[i]->state!=NR_ICE_PAIR_STATE_FAILED) && 
        (potentials[i]->state!=NR_ICE_PAIR_STATE_CANCELLED)){
        return(R_NOT_FOUND);
      }
    }
    
    if(!chosen)
      return(R_NOT_FOUND);
    return(0);
  }


static int nr_ua_ice_stream_ready(void *obj, nr_ice_media_stream *stream)
  {
    fprintf(stderr,"nr_ua_ice_stream_ready called.... stream=%s\n",stream->label);

    return(0);
  }


static int nr_ua_ice_stream_failed(void *obj, nr_ice_media_stream *stream)
  {
    fprintf(stderr,"nr_ua_ice_stream_failed called.... stream=%s\n",stream->label);

    return(0);
  }


static int nr_ua_ice_completed(void *obj, nr_ice_peer_ctx *pctx)
  {
    fprintf(stderr,"*********** ICE IS DONE ******************\n");

    CallHandler *ch=(CallHandler *)obj;
    ch->send(0,1,(UCHAR *)"ABCDEF",6);

    return(0);
  }

  static int nr_ua_ice_msg_recvd(void *obj, nr_ice_peer_ctx *pctx,nr_ice_media_stream *stream, int component_id, UCHAR *msg, int len)
  {
    fprintf(stderr,"****** Test message received stream=%s:%d len=%d\n",stream->label,component_id,len);

    return(0);
  }


nr_ice_handler_vtbl test_handler_vtbl={
  nr_ua_ice_select_pair,
  nr_ua_ice_stream_ready,
  nr_ua_ice_stream_failed,
  nr_ua_ice_completed,
  nr_ua_ice_msg_recvd
};



CallHandler::CallHandler(Mode mode):
  mState(START),
  mIceCtx(0),
  mIcePeerCtx(0),
  mStreamCt(0)
  {
    int r;
    int flags;

    if(!initialized){
      nr_app_startup("nr_ice_test",NR_APP_STARTUP_REGISTRY_LOCAL,0,0,0);
      NR_reg_set_char("logging.stderr.enabled", 1);
      NR_reg_set_char("logging.syslog.enabled", 1);
      NR_reg_set_string("logging.syslog.facility.nr_ice_test.level", "debug");
      NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
      NR_reg_set_string("logging.stderr.facility.nr_ice_test.level", "debug");
      NR_reg_set_string("logging.stderr.facility.stun.level", "debug");
      NR_reg_set_string("ice.stun.server.0.addr","198.144.201.197");
//NR_reg_set_string("ice.stun.server.0.addr","192.168.1.105");
//NR_reg_set_string("ice.stun.server.0.addr","64.69.76.23");
      NR_reg_set_uint2("ice.stun.server.0.port",3478);
      NR_reg_set_uchar("ice.pref.type.srv_rflx",100);
      NR_reg_set_uchar("ice.pref.type.host",150);
      NR_reg_set_uchar("ice.pref.type.relayed",200);
      NR_reg_set_uchar("ice.pref.interface.rl0", 255);
      NR_reg_set_uchar("ice.pref.interface.wi0", 254);
      NR_reg_set_uchar("ice.pref.interface.lo0", 253);
      NR_reg_set_uchar("ice.pref.interface.en1", 252);
      NR_reg_set_uchar("ice.pref.interface.en0", 251);
      NR_reg_set_uchar("ice.pref.interface.ppp", 250);
      NR_reg_set_uchar("ice.pref.interface.ppp0", 249);
      NR_reg_set_uchar("ice.pref.interface.en2", 248);
      NR_reg_set_uchar("ice.pref.interface.en3", 247);
      NR_reg_set_uint4("stun.client.retransmission.timeout", 60000);

      initialized=1;
    }

    flags=mode==OFFERER?NR_ICE_CTX_FLAGS_OFFERER:NR_ICE_CTX_FLAGS_ANSWERER;
    
    if(r=nr_ice_ctx_create("label",flags,&mIceCtx))
      abort(); // throw something

    mHandler.vtbl=&test_handler_vtbl;
    mHandler.obj=(void *)this;
  }

void CallHandler::setupStreams(int stream_ct, int streams[])
  {
    int r;

    assert(stream_ct<10);

    for(int i=0;i<stream_ct;i++){
      if(r=nr_ice_add_media_stream(mIceCtx,"stream",streams[i],&mStreams[i]))
        abort(); // Throw something
    }
    
    mStreamCt=stream_ct;
  }

static void done_cb(int s, int how, void *cb_arg)
  {
    int *donep=(int *)cb_arg;

    cout << "Operation done..\n";
    *donep=1;
  }

void CallHandler::gatherCandidates()
  {
    int r;
    int done=0;

    if(r=nr_ice_initialize(mIceCtx,done_cb,&done)){
      if(r!=R_WOULDBLOCK)
        abort();
    }

    /* Blocking while we're gathering candidates is
       a bit of a bug, but this is a test program, not
       a PSTN gateway, and the interlocks with UserAgent are 
       too complicated.
       At least we're not recursively in NR_async_event_wait2()! */
    /* If we're done,... */
    if(r){
      while(!done){
        int events;
        struct timeval towait={0,10000};
        struct timeval tv;
        
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
    }

    cout << "Done gathering candidates\n";
  }

// TODO: !nn! probably need to do all these Datas as news
SdpContents *CallHandler::exportSdp()
  {
    int r;
    char **attrs;
    int attr_ct;
    int i;
    
    // Template for fixed stuff
    resip::Data *txt=new Data("v=0\r\n"  
    "o=- 333525334858460 333525334858460 IN IP4 192.168.0.156\r\n"  // fix
    "s=test123\r\n"
    "t=4058038202 0\r\n");

    HeaderFieldValue hfv(txt->data(), txt->size());
    Mime type("application", "sdp");
    SdpContents *sdp=new SdpContents(&hfv, type);


    // Now add the media streams
    for(i=0; i<mStreamCt; i++){
      nr_ice_candidate *cand;
      char addr[64];
      int port;

      if(r=nr_ice_media_stream_get_best_candidate(mStreams[i],1,&cand))
        abort(); // TODO: !nn! Error
      

      if(r=nr_transport_addr_get_addrstring(&cand->addr,addr,sizeof(addr)))
        abort();
      if(r=nr_transport_addr_get_port(&cand->addr,&port))
        abort();

      SdpContents::Session::Medium *m=new 
        SdpContents::Session::Medium(resip::Data("test"),port,0,
        resip::Data("RTP/AVP"));

      m->addConnection(SdpContents::Session::Connection(SdpContents::IP4,
                         resip::Data(addr)));
      m->addCodec(Codec(Data("gsm"),3));

      /* Now add all the candidates */
      if(r=nr_ice_media_stream_get_attributes(mStreams[i],&attrs,&attr_ct))
        abort();

      for(int j=0;j<attr_ct;j++){
        char *col=strchr(attrs[j],':');
        if(col){
          *col++=0;

          m->addAttribute(Data(attrs[j]),Data(col));
        }
        else
        {
          m->addAttribute(Data(attrs[j]));
        }
      }
      sdp->session().addMedium(*m);
    }

    // Finally add the global candidates 
    if(r=nr_ice_get_global_attributes(mIceCtx,&attrs,&attr_ct))
      abort();
    for(int j=0;j<attr_ct;j++){
      char *col=strchr(attrs[j],':');
      if(col){
        *col++=0;
        
        sdp->session().addAttribute(Data(attrs[j]),Data(col));
      }
      else
      {
        sdp->session().addAttribute(Data(attrs[j]));
      }
    }
    // Debug
    sdp->session().encode(cout);

    return sdp;
  }


void CallHandler::extractGlobalAttributes(SdpContents *sdp,char *attrs[],int maxattrs, int *attr_ctp)
  {
    char *names[]={"ice-pwd","ice-ufrag","ice-lite","ice-mismatch","ice-options",0};
    int j;
    int attr_ct=0;

    // Figure out how many we need
    for(j=0;names[j];j++){
      if(attr_ct==maxattrs)
        abort(); // TODO: !nn! Throw exception
      std::list<Data> values=
        sdp->session().getValues(names[j]);

      for(std::list<Data>::iterator i=
        values.begin(); i != values.end(); ++i){

        const char *val=i->c_str();
        int len=strlen(names[j]) + strlen(val)+2;
        char *attr=new char[len];
        
        if(strlen(val)){
          sprintf(attr,"%s:%s",names[j],val);
        }
        else{
          sprintf(attr,"%s",names[j]);
        }
        
        attrs[attr_ct++]=attr;

      }
    }
    
    *attr_ctp=attr_ct;
  }



void CallHandler::extractMediumAttributes(SdpContents::Session::Medium &medium,char *attrs[],int maxattrs, int *attr_ctp)
  {
    char *names[]={"ice-pwd","ice-ufrag","candidate",0};
    int j;
    int attr_ct=0;

    // Figure out how many we need
    for(j=0;names[j];j++){
      if(attr_ct==maxattrs)
        abort(); // TODO: !nn! Throw exception
      
      if(!medium.exists(names[j]))
        continue;

      std::list<Data> values=
        medium.getValues(names[j]);

      for(std::list<Data>::iterator i=
        values.begin(); i != values.end(); ++i){

        const char *val=i->c_str();
        int len=strlen(names[j]) + strlen(val)+2;
        char *attr=new char[len];
        
        if(strlen(val)){
          sprintf(attr,"%s:%s",names[j],val);
        }
        else{
          sprintf(attr,"%s",names[j]);
        }
        
        attrs[attr_ct++]=attr;

      }
    }
    
    *attr_ctp=attr_ct;
  }


void CallHandler::importSdp(SdpContents *otherSdp)
  {
    char *attrs[100];
    int attrct;
    int r;

    
    extractGlobalAttributes(otherSdp,attrs,100,&attrct);
    if(r=nr_ice_peer_ctx_parse_global_attributes(mIcePeerCtx,attrs,attrct))
      abort();

    std::list<SdpContents::Session::Medium> media=otherSdp->session().media();

    int j=0;

    // Basically just trust that the peer SDP more or less matches
    // ours...
    for (std::list<SdpContents::Session::Medium>::iterator i
      =media.begin(); i != media.end(); ++i){
      int r;
      
      extractMediumAttributes(*i,attrs,100,&attrct);

      if(r=nr_ice_peer_ctx_parse_stream_attributes(mIcePeerCtx,mStreams[j],attrs,attrct))
        abort();

      j++;
    }
  }


SdpContents *CallHandler::answerCall(SdpContents *otherSdp)
  {
    int r;
    std::list<SdpContents::Session::Medium> media=
      otherSdp->session().media();
    
    int j=0;

    // First figure out the stream situation
    for (std::list<SdpContents::Session::Medium>::iterator i
      =media.begin(); i != media.end(); ++i){
      int r;

      if(r=nr_ice_add_media_stream(mIceCtx,"label",1,&mStreams[j]))
        abort();

      j++;
    }

    mStreamCt=j;

    // OK, now we need to gather the candidates
    gatherCandidates();

    // Now we need to export the SDP
    SdpContents *sdp=exportSdp();

    // Create the peer ctx and import the other SDP 
    if(r=nr_ice_peer_ctx_create(mIceCtx,&mHandler,"peer",&mIcePeerCtx))
      abort();
    importSdp(otherSdp);
    
    // and return our sdp
    return sdp;
  }

void CallHandler::processAnswer(SdpContents *otherSdp)
  {
    int r;

    // Create the peer ctx and import the other SDP 
    if(r=nr_ice_peer_ctx_create(mIceCtx,&mHandler,"peer",&mIcePeerCtx))
      abort();
    importSdp(otherSdp);
  }

void CallHandler::doICE()
  {
    int r;

    if(r=nr_ice_peer_ctx_pair_candidates(mIcePeerCtx))
      abort();
    if(r=nr_ice_peer_ctx_start_checks(mIcePeerCtx))
      abort();
  }

CallHandler::~CallHandler()
  {
  }


void CallHandler::send(int stream, int component, unsigned char *data, 
  int len)
  {
    int r;

    assert(stream>=0 && stream<mStreamCt);
 
    if(r=nr_ice_media_stream_send(mIcePeerCtx,mStreams[stream],component,data,len))
      abort();
  }






