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




#if defined(HAVE_CONFIG_HXX)
#include "resip/stack/config.hxx"
#endif

#include <cstring>
#include <cassert>

#ifndef __APPLE__
bool TRUE=true;
bool FALSE=false;
#endif

#include "UserAgent.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Logger.hxx"

extern "C" {
#include <nr_api.h>
#include <sys/queue.h>
#include <async_wait.h>
#include <async_timer.h>
#include <registry.h>
#include <nr_startup.h>
}

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   struct timeval tv;

   try
   {
      UserAgent ua(argc, argv);
      ua.startup();

      InfoLog(<< argv[0] << " starting");

      int pp[2];
      pipe(pp);
      // Give async wait something to do. Fix this in
      // nrappkit so you can wait on nothing?
      NR_ASYNC_WAIT(pp[0],NR_ASYNC_WAIT_READ,0,0);
      
      NR_async_timer_init();
      gettimeofday(&tv,0);
      NR_async_timer_update_time(&tv);

      while(1){
        int r;
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
        
        // Poll the SIP UA
        ua.process(1);
        gettimeofday(&tv,0);

        NR_async_timer_update_time(&tv);
      }
   }
   catch (BaseSecurity::Exception& e)
   {
      WarningLog (<< "Couldn't set up security object");
      exit(-1);
   }
   catch (BaseException& e)
   {
      ErrLog (<< "Caught: " << e);
      exit(-1);
   }
   catch( ... )
   {
      ErrLog( << "Caught non-resip exception" );
      exit(-1);
   }

   return 0;
}


#if 0
static void sendInvite(UserAgent &ua)
  {
    Data *txt;
    SdpContents* sdp;     
    HeaderFieldValue* hfv;      

    txt = new Data("v=0\r\n"
      "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
      "s=X-Lite\r\n"
      "c=IN IP4 192.168.2.15\r\n"
      "t=0 0\r\n"
      "m=audio 8000 RTP/AVP 8 3 98 97 101\r\n"
      "a=rtpmap:8 pcma/8000\r\n"
      "a=rtpmap:3 gsm/8000\r\n"
      "a=rtpmap:98 iLBC\r\n"
      "a=rtpmap:97 speex/8000\r\n"
      "a=rtpmap:101 telephone-event/8000\r\n"
      "a=fmtp:101 0-15\r\n"
      "a=candidate:blahblah\r\n");
         
    hfv = new HeaderFieldValue(txt->data(), txt->size());
    Mime type("application", "sdp");
    sdp = new SdpContents(hfv, type);

    ua.getDum().send(ua.getDum().makeInviteSession(NameAddr(ua.mTarget), sdp));
  }
#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
