#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/DtmfPayloadContents.hxx"
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
      Data txt("Signal=5\r\n"  
               "Duration=100\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "dtmf-relay");
      DtmfPayloadContents payload(hfv, type);
      
      assert(payload.dtmfPayload().getButton() == '5');
      assert(payload.dtmfPayload().getEventCode() == 5);
      assert(payload.dtmfPayload().getDuration() == 100);
   }

   {
      Data txt("Signal=A\r\n"
               "Duration=150\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "dtmf-relay");
      DtmfPayloadContents payload(hfv, type);

      assert(payload.dtmfPayload().getButton() == 'A');
      assert(payload.dtmfPayload().getEventCode() == 12);
      assert(payload.dtmfPayload().getDuration() == 150);
   }

   {
      Data txt("Signal=a\r\n"
               "Duration=100\r\n");
      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "dtmf-relay");
      try
      {
         DtmfPayloadContents payload(hfv, type);
         char button = payload.dtmfPayload().getButton();
         ErrLog(<<"Failed to detect a bad DTMF signal");
         assert(0);
      }
      catch (ParseException& ex)
      {
         // expected exception because of lowercase 'a'
         InfoLog(<<"detected the bad DTMF signal correctly");
      }
   }

   return 0;   
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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

