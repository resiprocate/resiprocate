
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


using namespace resip;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


class Client : public ThreadIf
{
    public:
      Client(SipStack& stack) : mStack(stack)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Client");

         NameAddr dest;
         dest.uri().scheme() = "sip";
         dest.uri().user() = "fluffy";
         dest.uri().host() = "localhost";
         dest.uri().port() = 5080;
         dest.uri().param(p_transport) = "tcp";
         
         NameAddr from = dest;
         from.uri().port() = 5070;
         
         Data count(1);
   
         from.uri().user() = count;
         {
            auto_ptr<SipMessage> message(Helper::makeInvite( dest, from, from));
            mStack.send(*message);
         }
         
         bool done = false;
         bool inviteState = true;
         
         
         while(true)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(5);
            assert (err != -1);
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
                     if (done)
                     {
                        break;
                     }
                     done = true;
                     
                     DebugLog(<< "Creating dialog.");
                     Dialog dlog(from);
                        
                     DebugLog(<< "Creating dialog as UAC.");
                     dlog.createDialogAsUAC(*received);
                        
                     DebugLog(<< "making ack.");
                     auto_ptr<SipMessage> ack(dlog.makeAck(*received) );
                        
                     DebugLog(<< "making bye.");
                     auto_ptr<SipMessage> bye(dlog.makeBye());
                        
                     DebugLog(<< "Sending ack: << " << endl << *ack);
                     mStack.send(*ack);
                        
                     DebugLog(<< "Sending bye: << " << endl << *bye);
                     mStack.send(*bye);
                  }
               }
            }
            usleep(1000);
         }
      }
   private:
      SipStack& mStack;
};

class Server : public ThreadIf
{
    public:

      Server(SipStack& stack) : mStack(stack)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Server");

         NameAddr dest;
         dest.uri().scheme() = "sip";
         dest.uri().user() = "fluffy";
         dest.uri().host() = "localhost";
         dest.uri().port() = 5080;
         dest.uri().param(p_transport) = "tcp";

         while(true)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(5);
            assert (err != -1);
            mStack.process(fdset);
            
            SipMessage* received = mStack.receive();
            if (received)
            {
               auto_ptr<SipMessage> forDel(received);
               InfoLog ( << "Server recieved: " << received->brief());
               MethodTypes meth = received->header(h_RequestLine).getMethod();
               if ( meth == INVITE )
               {
                  Data localTag = Helper::computeTag(4);
                  auto_ptr<SipMessage> msg180(Helper::makeResponse(*received, 180, dest));
                  msg180->header(h_To).param(p_tag) = localTag;
                  mStack.send( *msg180);
                  
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, dest));
                  msg200->header(h_To).param(p_tag) = localTag;
                  mStack.send(*msg200);
               }
               if ( meth == BYE)
               {
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, dest));
                  InfoLog (<< "stack2 got bye - send 200 : " << *msg200 );   
               
                  mStack.send(*msg200);
               }
            }
         }
      }
   private:
      SipStack& mStack;
};


int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::ERR, argv[0]);
   Log::toLevel( Data("DEBUG") );

   SipStack stack1;
   stack1.addTransport(Transport::TCP, 5070);

   SipStack stack2;
   stack2.addTransport(Transport::TCP, 5080);

   Client client(stack1);
   Server server(stack2);
   
   client.run();
   server.run();

   client.join();
   server.join();

   InfoLog(<< "Test failed.");

   return 0;
}
