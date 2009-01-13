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



#ifndef _callhandler_h
#define _callhandler_h

#include "resip/stack/StackThread.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"


// ICE
extern "C" {
#include <nr_api.h>
#include <sys/queue.h>
}
#include "ice_ctx.h"

namespace resip
{

class CallHandler {
public:
  typedef enum { OFFERER, ANSWERER} Mode;
  Mode mMode;
  typedef enum { START, GATHER, SDP_WAIT, ICE_RUNNING, ACTIVE } State;
  State mState;
  
  CallHandler(Mode mode);
  ~CallHandler();
  void setupStreams(int stream_ct, int streams[]);
  void gatherCandidates();
  SdpContents *exportSdp();
  void importSdp(SdpContents *sdp);
  SdpContents *answerCall(SdpContents *otherSdp);
  void processAnswer(SdpContents *otherSdp);
  void doICE();
  void send(int stream, int component, unsigned char *data, 
    int len);

private:
  nr_ice_ctx *mIceCtx;
  nr_ice_peer_ctx *mIcePeerCtx;
  nr_ice_media_stream *mStreams[10];
  int mStreamCt;
  nr_ice_handler mHandler;

  void extractGlobalAttributes(SdpContents *sdp,char *attrs[],int maxattrs, int *attr_ctp);
  void extractMediumAttributes(SdpContents::Session::Medium &medium,char *attrs[],int maxattrs, int *attr_ctp);

};

}


#endif

