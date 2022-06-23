

#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/Time.hxx"

#include "rutil/WinLeakCheck.hxx"

#include "media/kurento/KurentoManager.hxx"
#include "media/kurento/KurentoSubsystem.hxx"
#include "media/kurento/Object.hxx"

using namespace kurento;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

#define KURENTO_TIMEOUT 1000
#define KURENTO_URL "ws://127.0.0.1:8888/kurento"

/* Runs the event loop
 * Application callbacks will be invoked from the event loop
 */
void doProcess(KurentoManager& km, unsigned int duration, std::shared_ptr<Object> obj = nullptr)
{
   unsigned int interval = 2;
   unsigned int elapsed = 0;

   while(elapsed < duration && !(obj.get() && obj->valid()))
   {
      km.process();
      sleepMs(interval);
      elapsed += interval;
   }

   if(obj.get() && !obj->valid())
   {
      ErrLog(<<"object " << obj->getName() << " still not valid after " << elapsed << " ms");
      assert(obj->valid());
   }
}

int
main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK)
   FindMemoryLeaks fml;
#endif

   Log::initialize(Log::Cout, Log::Stack, argv[0]);

   KurentoManager mKurentoManager(KURENTO_TIMEOUT);
   std::string kUrl = KURENTO_URL;
   KurentoConnection::ptr kurentoConnection_ptr = mKurentoManager.getKurentoConnection(kUrl);

   std::shared_ptr<kurento::MediaPipeline> pipeline = std::make_shared<kurento::MediaPipeline>(kurentoConnection_ptr);
   pipeline->create([]{
      DebugLog(<<"pipeline created");
   });
   doProcess(mKurentoManager, 100, pipeline);

   std::shared_ptr<kurento::WebRtcEndpoint> ep = std::make_shared<kurento::WebRtcEndpoint>(pipeline);
   ep->create([]{
      DebugLog(<<"WebRtcEndpoint created");
   });
   doProcess(mKurentoManager, 100, ep);

   // Need to manually insert the IP address in the code below before
   // uncommenting it
   /*ep->setExternalIPv4([&ep](){
      DebugLog(<<"external IPv4 address set");
   }, "192.168.1.100"); */

   ep->gatherCandidates([&ep](){
      DebugLog(<<"got callback");
      ep->generateOffer([&ep](const std::string& s){
         DebugLog(<<"got callback: "<<s);
         ep->getLocalSessionDescriptor([](const std::string& s){
            DebugLog(<<"got callback again: "<<s);
         });
      });
   });

   doProcess(mKurentoManager, 5000);

   return 0;
}

/* ====================================================================

 Copyright (c) 2021, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
