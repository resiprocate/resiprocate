#if !defined(UserAgent_hxx)
#define UserAgent_hxx

#include "RegistrationManager.hxx"

#include <resip/dum/DialogUsageManager.hxx>
#include <rutil/Log.hxx>
#include <rutil/SharedPtr.hxx>
#include <rutil/Mutex.hxx>

namespace recon
{
/**
  Author: Jeremy Geras (jgeras AT counterpath DOT com)
*/

class RTPPortAllocator;
class MediaStack;
class CodecFactory;

class UserAgent
{
public:

   /**
     Constructor

     @param conversationManager Application subclassed Conversation 
                                Manager
     @param masterProfile       Object containing useragent settings
   */
   UserAgent() {}
   virtual ~UserAgent() {}

   virtual resip::DialogUsageManager* getDialogUsageManager() = 0;

   /**
    * The port range should return 0 .. 0 if there is no specified port
    * range for min and max.
    */
   virtual void getPortRange( unsigned int& minPort, unsigned int& maxPort ) = 0;

   virtual recon::MediaStack* getMediaStack() = 0;
   virtual recon::CodecFactory* getCodecFactory() = 0;
   virtual recon::RegistrationManager* getRegistrationManager() = 0;

};
 
}

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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

 ==================================================================== */
