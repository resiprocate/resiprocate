#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Dialog.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::TEST

class Client
{
    public:
      Client(Transport::Type transport, const NameAddr& contact, const NameAddr& target) 
         : mStack(),
           mContact(contact),
           mTarget(target),
           mWaitingForBye200(false)
      {
         mStack.addTransport(transport, contact.uri().port());
         auto_ptr<SipMessage> message(Helper::makeInvite( target, mContact, mContact));
         mStack.send(*message);
      }

      void buildFdSet(FdSet& fdset)
      {
         mStack.buildFdSet(fdset);
      }
      
      void process(FdSet& fdset)
      {
         mStack.process(fdset);
         
         SipMessage* received = mStack.receive();
         if (received)
         {
            InfoLog (<< "Client received: " << received->brief());
            
            auto_ptr<SipMessage> forDel(received);
            if ( (received->isResponse()) )
            {
               if ( received->header(h_StatusLine).responseCode() == 200 )
               {
                  if (!mWaitingForBye200)
                  {
                     ErrLog(<< "Creating dialog.");
                     Dialog dlog(mContact);
                     
                     DebugLog(<< "Creating dialog as UAC.");
                     dlog.createDialogAsUAC(*received);
                     
                     DebugLog(<< "making ack.");
                     auto_ptr<SipMessage> ack(dlog.makeAck(*received) );
                     DebugLog(<< *ack);

                     DebugLog(<< "making bye.");
                     auto_ptr<SipMessage> bye(dlog.makeBye());
                     
                     DebugLog(<< "Sending ack: << " << endl << *ack);
                     mStack.send(*ack);
                     
                     DebugLog(<< "Sending bye: << " << endl << *bye);
                     mStack.send(*bye);
                     mWaitingForBye200 = true;
                  }
                  else
                  {
                     auto_ptr<SipMessage> message(Helper::makeInvite( mTarget, mContact, mContact));
                     mStack.send(*message);
                     mWaitingForBye200 = false;
                  }
               }
            }
         }
      }
   private:
      SipStack mStack;
      NameAddr mContact;
      NameAddr mTarget;
      bool mWaitingForBye200;
};

int
main(int argc, char* argv[])
{
   if (argc != 3)
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI" << endl;
      exit(-1);
   } 
   Log::initialize(Log::COUT, Log::toLevel(argv[1]), argv[0]);

   NameAddr target(argv[2]);

   NameAddr contact;
   contact.uri().host() = SipStack::getHostname();
   contact.uri().port() = 5080;
   contact.uri().user() = "yffulf";

   Transport::Type protocol;
   if (isEqualNoCase(target.uri().param(p_transport), "UDP"))
   {
      protocol = Transport::UDP;
   }
   else if (isEqualNoCase(target.uri().param(p_transport), "TCP"))
   {
      protocol = Transport::TCP;
   }
   else
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI(must include transport parameter)" << endl;
      exit(-1);
   }
   
   Client c(protocol, contact, target);

   while (true)
   {
      FdSet fdset;
      c.buildFdSet(fdset);
      fdset.selectMilliSeconds(0);
      c.process(fdset);
   }
}
