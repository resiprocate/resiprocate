#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/MediaControlContents.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "rutil/ParseBuffer.hxx"

#include <iostream>
#include "TestSupport.hxx"
#include "tassert.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Stack;
    
   if (argc > 1)
   {
      switch(*argv[1])
      {
         case 'd': l = Log::Debug;
            break;
            case 'i': l = Log::Info;
               break;
            case 's': l = Log::Stack;
               break;
            case 'c': l = Log::Crit;
               break;
      }
        
   }
    
   Log::initialize(Log::Cout, l, argv[0]);
   CritLog(<<"Test Driver Starting");

   {
      Data txt("<?xml version=\"1.0\" encoding=\"utf-8\"?><media_control><vc_primitive><to_encoder><picture_fast_update></picture_fast_update></to_encoder><stream_id>11</stream_id></vc_primitive></media_control>");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "media_control+xml");
      MediaControlContents payload(hfv, type);
      
      InfoLog(<< payload);

      assert(!payload.mediaControl().isError());
      const MediaControlContents::MediaControl::VCPrimitive& p = *payload.mediaControl().vCPrimitives().cbegin();
      assert(p.pictureFastUpdate());
      assert(p.streamIDs().size() == 1);
      assert(*(p.streamIDs().cbegin()) == Data("11"));
   }

   {
      Data txt("<?xml version=\"1.0\" encoding=\"utf-8\"?><media_control><vc_primitive><to_encoder><picture_fast_update></picture_fast_update></to_encoder><stream_id>11</stream_id></vc_primitive>"
            "<general_error>XML parsing complaints go here</general_error>"
            "</media_control>");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "media_control+xml");
      MediaControlContents payload(hfv, type);

      InfoLog(<< payload);

      assert(payload.mediaControl().isError());
      assert(*payload.mediaControl().generalErrors().cbegin() == Data("XML parsing complaints go here"));
   }

   {
      MediaControlContents payload;
      MediaControlContents::MediaControl& m = payload.mediaControl();

      MediaControlContents::MediaControl::VCPrimitive::StreamIDList streams;
      streams.insert("123");
      m = MediaControlContents::MediaControl(streams, true);

      InfoLog(<< payload);
   }

   return 0;   
}

/* ====================================================================
 *
 * Copyright (c) 2021, Daniel Pocock, https://danielpocock.com
 * Copyright (c) 2021, Software Freedom Institute SA, https://softwarefreedom.institute
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

