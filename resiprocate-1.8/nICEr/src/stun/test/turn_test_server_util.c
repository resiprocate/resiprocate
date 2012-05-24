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



#ifdef USE_TURN

static void s_cb(int s, int how, void *cb_arg)
  {
#if 0
    nr_socket *sock = (nr_socket*)cb_arg;
    UCHAR buf2[4096];
    size_t len;
    nr_transport_addr addr2;
    int r;
    StunMessage req;
    StunMessage res;
    //StunAddress4 from;

    fprintf(stderr,"TURN_SERVER_UTIL: SERVER CB\n");

    if(r=nr_socket_recvfrom(sock,buf2,sizeof(buf2),&len,0,&addr2)){
      fprintf(stderr,"Error in recvfrom\n");
      exit(1);
    }

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    //memset(&from, 0, sizeof(from));
    //from.addr = ntohl(addr2.u.addr4.sin_addr.s_addr);
    //from.port = ntohs(addr2.u.addr4.sin_port);

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
        fprintf(stderr,"TURN_SERVER_UTIL: No problem with parse failure ... process TURN in test server code\n");
    }

    if (!nr_is_stun_message((char*)buf2, len)) {
        fprintf(stderr,"TURN_SERVER_UTIL: not a STUN/TURN message\n");
        /* instead of sending to remoteAddress, just echo the content back
         * for the purposes of our tests */
        goto send;
    }

    res.hasFingerprint = 1;

    switch (req.msgHdr.msgType) {
    case AllocateRequestMsg:
       if (! req.hasNonce) {
           fprintf(stderr, "Received AllocateRequestMsg #1\n");
           assert(!req.hasMessageIntegrity);
/* TODO: what about username: does it go into the req or not? spec not clear */
           res.msgHdr.msgType = AllocateErrorResponseMsg;
           res.hasUsername = 0;
           res.hasNonce = 1;
           strcpy(res.nonce, nonce);
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
#if 0
//TODO: this is totally broken, but TURN is changing and so
//TODO: we'll need to take another pass at all this code
//TODO: anyhow, so leave it broken for now
           res.hasRelayAddress = 1;
           res.relayAddress.family = IPv4Family;
           res.relayAddress.ipv4.addr = from.addr;
           res.relayAddress.ipv4.port = from.port;
           res.hasXorMappedAddress = 1;
           res.xorMappedAddress.family = IPv4Family;
           res.xorMappedAddress.ipv4.addr = from.addr;
           res.xorMappedAddress.ipv4.port = from.port;
#endif
       }
       break;
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
       nr_transport_addr_copy(&res.remoteAddress, &req.remoteAddress);

       break;
    case SetActiveDestRequestMsg:
       fprintf(stderr, "Received SetActiveDestRequestMsg\n");
       assert(req.hasRemoteAddress);
 
       res.msgHdr.msgType = SetActiveDestResponseMsg;
       break;
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
      exit(1);
    }

    NR_ASYNC_WAIT(s, how, s_cb, cb_arg);
#else
UNIMPLEMENTED;
#endif
  }

#endif /* USE_TURN */
