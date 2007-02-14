#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"

#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/Mime.hxx"

#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Helper.hxx"

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#ifdef WIN32
#include "resip/stack/WinSecurity.hxx"
#endif

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class TestSMIMEMessageHandler : public ClientPagerMessageHandler,
                            public ServerPagerMessageHandler,
                            public ClientRegistrationHandler
{
   public:
      TestSMIMEMessageHandler(Security *security) : security(security), _registered(false), _ended(false), _rcvd(false)
      {}

      virtual ~TestSMIMEMessageHandler() 
      {
      }

      void reset()
      {
         _ended = false;
         _rcvd = false;
      }

      bool isRegistered()
      {
         return _registered;
      }

      bool isEnded()
      {
         return _ended;
      }

      bool isRcvd()
      {
         return _rcvd;
      }

      virtual void onMessageArrived(ServerPagerMessageHandle handle,
                                    const SipMessage& message)
      {
         
         InfoLog( << "ServerPagerMessageHandler::onMessageArrived: " );

         SharedPtr<SipMessage> ok = handle->accept();
         handle->send(ok);

         _rcvd = true;

         InfoLog( << "received type " << message.header(h_ContentType) );

         Helper::ContentsSecAttrs csa(Helper::extractFromPkcs7(message, *security));
         message.setSecurityAttributes(csa.mAttributes);

         InfoLog( << "Body: " << *csa.mContents << "\n" );

         const SecurityAttributes *attr = message.getSecurityAttributes();
         InfoLog( << *attr );
         
      }

      virtual void onSuccess(ClientPagerMessageHandle,
                             const SipMessage& status)
      {
         InfoLog( << "ClientMessageHandler::onSuccess\n" );
         _ended = true;
      }

      virtual void onFailure(ClientPagerMessageHandle,
                             const SipMessage& status,
                             std::auto_ptr<Contents> contents)
      {
         InfoLog( << "ClientMessageHandler::onFailure\n" );
         _ended = true;
      }

      virtual void onSuccess(ClientRegistrationHandle,
                             const SipMessage& response)
      {
         InfoLog( << "ClientRegistrationHandler::onSuccess\n" );
         _registered = true;
      }

      virtual void onRemoved(ClientRegistrationHandle, const SipMessage&)
      {
         InfoLog( << "ClientRegistrationHander::onRemoved\n" );
         exit(-1);
      }
      
      virtual void onFailure(ClientRegistrationHandle,
                             const SipMessage& response)
      {
         InfoLog( << "ClientRegistrationHandler::onFailure\n" );
         exit(-1);
      }

      virtual int onRequestRetry(ClientRegistrationHandle,
                                 int retrySeconds, const SipMessage& response)
      {
         InfoLog( << "ClientRegistrationHandler::onRequestRetry\n" );
         exit(-1);
      }

   protected:
      Security *security;
      bool _registered;
      bool _ended;
      bool _rcvd;
      
};


/*****************************************************************************/

int main(int argc, char *argv[])
{

   if ( argc < 3 ) {
      cout << "usage: " << argv[0] << " sip:user passwd\n";
      return 0;
   }

   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   NameAddr userAor(argv[1]);
   Data passwd(argv[2]);

   InfoLog(<< "user: " << userAor << ", passwd: " << passwd << "\n");

#ifdef WIN32
   Security* security = new WinSecurity;
#else
   Security* security = new Security;
#endif

   assert(security);

   SipStack clientStack(security);
   DialogUsageManager clientDum(clientStack);
   clientDum.addTransport(UDP, 10000 + rand()&0x7fff, V4);
   //clientDum.addTransport(TCP, 10000 + rand()&0x7fff, V4);
   //clientDum.addTransport(TLS, 10000 + rand()&0x7fff, V4);
   // clientDum.addTransport(UDP, 10000 + rand()&0x7fff, V6);
   // clientDum.addTransport(TCP, 10000 + rand()&0x7fff, V6);
   // clientDum.addTransport(TLS, 10000 + rand()&0x7fff, V6);

   SharedPtr<MasterProfile> clientProfile(new MasterProfile);   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());   
   TestSMIMEMessageHandler clientHandler(security);

   clientDum.setClientAuthManager(clientAuth);
   clientDum.setClientRegistrationHandler(&clientHandler);
   clientDum.setClientPagerMessageHandler(&clientHandler);
   clientDum.setServerPagerMessageHandler(&clientHandler);

   clientProfile->setDefaultFrom(userAor);
   clientProfile->setDigestCredential(userAor.uri().host(), userAor.uri().user(), passwd);
   clientProfile->setDefaultRegistrationTime(70);		
   clientProfile->addSupportedMethod(MESSAGE);
   clientProfile->addSupportedMimeType(MESSAGE, Mime("text", "plain"));
   clientProfile->addSupportedMimeType(MESSAGE, Mime("application", "pkcs7-mime"));
   clientProfile->addSupportedMimeType(MESSAGE, Mime("multipart", "signed"));
   clientDum.setMasterProfile(clientProfile);

   SharedPtr<SipMessage> regMessage = clientDum.makeRegistration(userAor);
	
   InfoLog( << *regMessage << "Generated register: " << endl << *regMessage );
   clientDum.send(regMessage);

   int iteration = 0;
   bool finished = false;
   while (!finished) 
   {

      clientHandler.reset();
      
      bool sent = false;
      while (!clientHandler.isEnded() || !clientHandler.isRcvd() )
      {
         FdSet fdset;

         clientStack.buildFdSet(fdset);

         int err = fdset.selectMilliSeconds(100);
         assert ( err != -1 );
      
         clientStack.process(fdset);
         while (clientDum.process());

         if (!sent && clientHandler.isRegistered())
         {
            try 
            {
               ClientPagerMessageHandle cpmh = clientDum.makePagerMessage(userAor);			
               Contents* mcontent = new PlainContents(Data("message"));
               auto_ptr<Contents> content;

               switch (iteration) 
               {
                  case 0:
                  {
                     InfoLog( << "Sending PKCS7 encrypted data" );
                     Pkcs7Contents* pkcs7 = security->encrypt(mcontent, userAor.uri().getAor());
                     content = auto_ptr<Contents>(static_cast<Contents *>(pkcs7));
                     break;
                  }
                  case 1:
                  {
                     InfoLog( << "Sending PKCS7 signed data" );
                     MultipartSignedContents* mpc = security->sign(userAor.uri().getAor(), mcontent);
                     content = auto_ptr<Contents>(static_cast<Contents *>(mpc));
                     break;
                  }
                  default:
                  {
                     InfoLog( << "Finished!" );
                     finished = true;
                     break;
                  }
               }
               
               if (finished)
                  break;
               
               cpmh.get()->page(content);
               sent = true;
               
            }
               
            catch (...)
            {
               InfoLog( << "failure to send message at iteration " << iteration );
               break;
            }

         }

      }
      ++iteration;
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
