#include "resiprocate/SipStack.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/PublicationHandler.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/Subsystem.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/AppDialog.hxx"
#include "resiprocate/dum/AppDialogSetFactory.hxx"
#include "resiprocate/Pidf.hxx"
//#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/GenericContents.hxx"


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
 
   Log::initialize(Log::Cout, Log::Info, argv[0]);

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

        char c;
        char str[256];
        if (!cin.eof())
        {
           c=cin.get();
           if ( c == 'n' || c == 'q') 
           {
              if (c == 'q')
              {
                 break;
              }
              else
              {
                 subServerHandler.sendNextNotify();
              }
           }
           else
           {
              cin >> str;
              cout << " You have entered [" << str << "] enter n to send the next notify or q to quit " << endl;
           }
        }
   }
   
   return 0;

}   
