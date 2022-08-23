#ifndef SIPXHELPER_HXX
#define SIPXHELPER_HXX

#include "rutil/Data.hxx"

namespace recon
{

class SipXHelper
{
public:
   // Sets up a link from the sipX logging class to
   // the reSIProcate logging stack.  This version grabs logs after they 
   // have been queued to the OsSysLog thread.
   static void setupLoggingBridge(const resip::Data& appName);

   // Sets up a link from the sipX logging class to
   // the reSIProcate logging stack.  This version grabs logs before they 
   // have been queued to the OsSysLog thread. This is more appropriate
   // for use if a non-blocking external resip logger has been configured, 
   // as it runs inline with sipX media processing.
   // This version also avoids the overhead of sipX log message encoding, and
   // then decoding in the logging handler.
   static void setupPreQueueLoggingBridge(const resip::Data& appName);
};


}

#endif


/* ====================================================================
 *
 * Copyright 2022 SIP Spectrum, Inc. http://sipspectrum.com
 * Copyright 2014 Daniel Pocock http://danielpocock.com  
 * All rights reserved.
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

