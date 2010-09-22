// made by Aron Rosenberg
// Will do an initial PUBLICATION of pidf presence
// then do 2 in-dialog updates, then quit
// If you define PUB_REALLY_FAST then overlapping
// or queued PUBLISH'es will be created.

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/Profile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/ClientPublication.hxx"

#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Pidf.hxx"
#include "rutil/Random.hxx"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

//#define PUB_REALLY_FAST 1// Will create overlapping PUBLISH'es

int transCount = 0;

class ClientPubHandler : public ClientPublicationHandler {
public:
   ClientPubHandler() {}
   virtual void onSuccess(ClientPublicationHandle cph, const SipMessage& status)
   {
      handle = cph;
      InfoLog(<<"ClientPubHandler::onSuccess\n");
      transCount--;
   }
   virtual void onRemove(ClientPublicationHandle cph, const SipMessage& status)
   {
	  InfoLog(<<"ClientPubHandler::onRemove\n");
      handle = ClientPublicationHandle();
      transCount--;
   }
   virtual int onRequestRetry(ClientPublicationHandle cph, int retrySeconds, const SipMessage& status)
   {
      handle = cph;
      InfoLog(<<"ClientPubHandler::onRequestRetry\n");
      return 30;
   }
   virtual void onFailure(ClientPublicationHandle cph, const SipMessage& status)
   {
      InfoLog(<<"ClientPubHandler::onFailure\n");
      handle = ClientPublicationHandle();
      transCount--;
   }
   ClientPublicationHandle handle;
};


/*****************************************************************************/

int main(int argc, char *argv[])
{
   if( (argc < 5) || (argc > 6) ) {
      cout << "usage: " << argv[0] << " sip:aor user passwd realm [port]\n";
      return 0;
   }

   Log::initialize(Log::VSDebugWindow, Log::Debug, argv[0]);

   bool first = true;
   string aor(argv[1]);
   string user(argv[2]);
   string passwd(argv[3]);
   string realm(argv[4]);
   int port = 5060;
   if(argc == 6) {
      string temp(argv[5]);
      istringstream src(temp);
      src >> port;
   }
   Data eventName("presence");

   InfoLog(<< "log: aor: " << aor << ", port: " << port << "\n");
   InfoLog(<< "user: " << user << ", passwd: " << passwd << ", realm: " << realm << "\n");
   
   // sip logic
   SharedPtr<MasterProfile> profile(new MasterProfile);   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());   

   SipStack clientStack;
   DialogUsageManager clientDum(clientStack);
   clientDum.addTransport(UDP, port);
   clientDum.setMasterProfile(profile);

   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->addSupportedMethod(PUBLISH);
   clientDum.getMasterProfile()->addSupportedMimeType(PUBLISH,Pidf::getStaticType());

   ClientPubHandler cph;
   clientDum.addClientPublicationHandler(eventName,&cph);

   /////
   NameAddr naAor(aor.c_str());
   profile->setDefaultFrom(naAor);
   profile->setDigestCredential(realm.c_str(), user.c_str(), passwd.c_str());
   
   Pidf pidf;
   pidf.setSimpleStatus(true);
   pidf.setEntity(naAor.uri());
   pidf.setSimpleId(Random::getRandomHex(3));

   {
      SharedPtr<SipMessage> pubMessage = clientDum.makePublication(naAor, profile,pidf,eventName,120);
      InfoLog( << "Generated publish: " << endl << *pubMessage );
      transCount++;
      clientDum.send( pubMessage );
   }

   int nAttempts = 0;
   bool bKeepGoing = true;
   while(bKeepGoing)
   {
      FdSet fdset;

      // Should these be buildFdSet on the DUM?
      clientStack.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(100);
      assert ( err != -1 );
      
      clientStack.process(fdset);
        while(clientDum.process());
      //if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';

      if(transCount == 0) // No pending transactions.
      {
         nAttempts++;
         if(nAttempts > 2)
            bKeepGoing = false;
         else
         {
            int nLoops = 1;
#ifdef PUB_REALLY_FAST
            nLoops = 2;
#endif
            for(int i = 0; i < nLoops; i++)
            {
               pidf.setSimpleStatus(!pidf.getSimpleStatus()); //toggle status
               transCount++;
               cph.handle->update(&pidf);
            }
         }
      }
   }   

   return 0;
}

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
