#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/InviteSessionHandler.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Subsystem.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialog.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class RegisterAppDialogSet : public AppDialogSet
{
   public:
      RegisterAppDialogSet(DialogUsageManager& dum) : AppDialogSet(dum) 
      {}      
};

   

class Client : public ClientRegistrationHandler
{
   public:
      Client() : done(false), removing(true) {}

      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
         InfoLog( << "Client::Success: " << endl << response );
         if (removing)
         {
            removing = false;             
#ifdef WIN32
            Sleep(2000);
#else
            sleep(5);
#endif
            //ClientRegistration* foo = h.get();          
            h->removeAll();
         }
         else
         {             
            done = true;
         }          
      }

      virtual void onRemoved(ClientRegistrationHandle)
      {
         InfoLog ( << "Client::onRemoved ");
      }

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
      {
         InfoLog ( << "Client::onFailure: " << response );
         done = true;
      }

      virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
      {
         return 0;
      }
      

      bool done;
      bool removing;      
};

/*
  class RegistrationServer : public ServerRegistrationHandler
  {
  public:
  RegistrationServer() : done(false) {}
  virtual void onRefresh(ServerRegistrationHandle, const SipMessage& reg)=0;
      
  /// called when one of the contacts is removed
  virtual void onRemoveOne(ServerRegistrationHandle, const SipMessage& reg)=0;
      
  /// Called when all the contacts are removed 
  virtual void onRemoveAll(ServerRegistrationHandle, const SipMessage& reg)=0;
      
  /// Called when a new contact is added. This is after authentication has
  /// all sucseeded
  virtual void onAdd(ServerRegistrationHandle, const SipMessage& reg)=0;

  /// Called when an registration expires 
  virtual void onExpired(ServerRegistrationHandle, const NameAddr& contact)=0;
      
  private:
  bool done;
  };
*/

int 
main (int argc, char** argv)
{
   int level=(int)Log::Debug;
   if (argc >1 ) level = atoi(argv[1]);

   Log::initialize(Log::Cout, (resip::Log::Level)level, argv[0]);

   Client client;
   MasterProfile profile;   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);   

   SipStack stack;
   DialogUsageManager clientDum(stack);
   clientDum.addTransport(UDP, 15066);
   clientDum.setMasterProfile(&profile);

   clientDum.setClientRegistrationHandler(&client);
   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->setDefaultRegistrationTime(70);

   NameAddr from("sip:derek@foo.net");
   clientDum.getMasterProfile()->setDefaultFrom(from);


   profile.setDigestCredential( "foo.net", "derek", "derek");
   

   SipMessage & regMessage = clientDum.makeRegistration(from, new RegisterAppDialogSet(clientDum));
   NameAddr contact;
//   contact.uri().user() = "13015604286";   
//   regMessage.header(h_Contacts).push_back(contact);   

   InfoLog( << regMessage << "Generated register: " << endl << regMessage );
   clientDum.send( regMessage );

   int n = 0;
   while ( !client.done )

   {
      FdSet fdset;

      stack.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(100);
      assert ( err != -1 );

      stack.process(fdset);
      while(clientDum.process());

      if (!(n++ % 10)) cerr << "|/-\\"[(n/10)%4] << '\b';
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
