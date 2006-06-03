#include "resip/stack/SipStack.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/Profile.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/stack/Pidf.hxx"
//#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/GenericContents.hxx"


#include <iostream>
#include <fstream>

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class RlsRegistrationHandler : public ClientRegistrationHandler
{
   public:
      virtual void onSuccess(ClientRegistrationHandle h, const SipMessage& response)
      {
          InfoLog( << "onSuccess: "  << response );
      }

      virtual void onRemoved(ClientRegistrationHandle)
      {
      }           

      virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
      {
          InfoLog ( << "Client::Failure: " << response );
          exit(-1);
      }
};

Contents* readFromFile(string filename)
{
   ifstream is;
   is.open (filename.c_str(), ios::binary );

   if (!is.good())
   {
      exit(-1);
   }

   // get length of file:
   is.seekg (0, ios::end);
   int length = is.tellg();
   is.seekg (0, ios::beg);

   // allocate memory:
   char* buffer = new char[length];

   // read data as a block:
   is.read (buffer,length);
   
   Mime mimeType;
   
   ParseBuffer pb(buffer, length);
   pb.skipChars("Content-Type:");
   pb.skipWhitespace();
   mimeType.parse(pb);   
   pb.skipChars(Symbols::CRLF);   
   
   HeaderFieldValue hfv(pb.position(), length - (pb.position() - pb.start()));
   GenericContents orig(&hfv, mimeType);
   
   GenericContents * copy = new GenericContents(orig);
   delete buffer;
   return copy;
}


class RlsServerSubscriptionHandler : public ServerSubscriptionHandler
{
  public:   
      class RlsSubscription
      {
         public:
            RlsSubscription() :
               currentContent(0)
            {
            }

            RlsSubscription(ServerSubscriptionHandle h, int pos) :
               handle(h),
               currentContent(pos)
            {
            }
               
            ServerSubscriptionHandle handle;
            int currentContent;            
      };      
         
      //first file is a full notification, no restriction on the rest
      RlsServerSubscriptionHandler(const vector<string> inputFiles)
      {
         for (vector<string>::const_iterator it = inputFiles.begin(); it != inputFiles.end(); it++)
         {
            mContentsList.push_back(readFromFile(*it));
         }
      }

      virtual void onNewSubscription(ServerSubscriptionHandle handle, const SipMessage& sub)
      {
         InfoLog ( << "onNewSubscription: " << sub.header(h_RequestLine) << "\t" << 
                   sub.header(h_From) << "\t" << handle->getAppDialog()->getDialogId());
         assert(mHandleMap.find(handle->getAppDialog()->getDialogId()) == mHandleMap.end());
         mHandleMap[handle->getAppDialog()->getDialogId()] = RlsSubscription(handle, 1);
         handle->send(handle->accept());
         SipMessage& notify = handle->update(mContentsList[0]);
         notify.header(h_Requires).push_back(Token("eventlist"));
         handle->send(notify);
      }
      
      virtual void onTerminated(ServerSubscriptionHandle sub)
      {
         InfoLog ( << "onTerminated: " << sub->getAppDialog()->getDialogId());
         mHandleMap.erase(sub->getAppDialog()->getDialogId());
      }

      //refresh resets to beginning
      virtual void onRefresh(ServerSubscriptionHandle handle, const SipMessage& sub)
      {
         DialogToHandle::iterator it = mHandleMap.find(handle->getAppDialog()->getDialogId());
         if (it == mHandleMap.end())
         {
            assert(0);
         }
         else
         {
            ServerSubscriptionHandle handle = it->second.handle;
            it->second.currentContent = 0;            
            SipMessage& notify = handle->update(mContentsList[it->second.currentContent++]);
            notify.header(h_Requires).push_back(Token("eventlist"));
            handle->send(notify);
         }
      }
      
      virtual bool hasDefaultExpires() const
      {
         return true;
      }
      
      virtual int getDefaultExpires() const
      {
         return 600;
      }

      virtual void sendNextNotify()
      {
         for (DialogToHandle::iterator it = mHandleMap.begin(); it != mHandleMap.end(); it++)
         {
            ServerSubscriptionHandle handle = it->second.handle;
            if (it->second.currentContent > mContentsList.size())
            {
               handle->end();
            }
            else
            {
               SipMessage& notify = handle->update(mContentsList[it->second.currentContent++]);
               notify.header(h_Requires).push_back(Token("eventlist"));
               handle->send(notify);
            }
         }
      }
      
      virtual ~RlsServerSubscriptionHandler()
      {
         //clean up map, contents
      }
   private:
      typedef map<DialogId, RlsSubscription> DialogToHandle;
      DialogToHandle mHandleMap;
      typedef vector<Contents*> ContentsList;
      vector<Contents*> mContentsList;
};
   

int 
main (int argc, char** argv)
{
   if (argc == 1)
   {
      cerr << "Usage: rlsServer contentsFile (SPACE contentsFile) << endl";
      return -1;
   }
 
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   vector<string> inputFiles;
   for (int i = 1; i < argc; i++)
   {
      inputFiles.push_back(string(argv[i]));
   }

   RlsServerSubscriptionHandler subServerHandler(inputFiles);
   
   NameAddr from("sip:testRlsServer@internal.xten.net");
   Profile profile;   
   profile.setDefaultFrom(from);
   profile.clearSupportedMethods();
   profile.addSupportedMethod(SUBSCRIBE);   

   profile.validateAcceptEnabled() = false;
   profile.validateContentLanguageEnabled() = false;
   profile.validateContentEnabled() = false;

   DialogUsageManager dum;
   dum.addTransport(UDP, 15060);
//   dum.addTransport(TCP, 15060);

   dum.setProfile(&profile);
   dum.addServerSubscriptionHandler("presence", &subServerHandler);

   RlsRegistrationHandler regHandler;
   dum.setClientRegistrationHandler(&regHandler);

   SipMessage & regMessage = dum.makeRegistration(from);
   InfoLog( << regMessage << "Generated register: " << endl << regMessage );
   dum.send( regMessage );

   while (true)
   {
      FdSet fdset;
      dum.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(100);
      assert ( err != -1 );
      dum.process(fdset);
      
//         char c;
//         char str[256];
//         if (!cin.eof())
//         {
//            c=cin.get();
//            if ( c == 'n' || c == 'q') 
//            {
//               if (c == 'q')
//               {
//                  break;
//               }
//               else
//               {
//                  subServerHandler.sendNextNotify();
//               }
//            }
//            else
//            {
//               cin >> str;
//               cout << " You have entered [" << str << "] enter n to send the next notify or q to quit " << endl;
//            }
//         }
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
