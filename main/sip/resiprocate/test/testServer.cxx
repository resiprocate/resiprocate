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

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class Server : public ThreadIf
{
    public:

      Server(SipStack& stack, int numCalls, TransportType transport) 
         : mStack(stack), 
           mNumCalls(numCalls),
           mTransport(transport)
      {}
      
      void thread()
      {
         InfoLog(<<"This is the Server");

         UInt64 startTime = Timer::getTimeMs();

         NameAddr contact;
         contact.uri().scheme() = "sip";
         contact.uri().user() = "fluffy";
         
         contact.uri().host() = SipStack::getHostname();
         contact.uri().port() = 5070;
         contact.uri().param(p_transport) = Tuple::toData(mTransport);
         
         int calls = mNumCalls;
         while(calls > 0)
         {
            FdSet fdset;
            mStack.buildFdSet(fdset);
            int err = fdset.selectMilliSeconds(0);
            assert (err != -1);
            mStack.process(fdset);
            
            SipMessage* received = mStack.receive();
            if (received)
            {
               auto_ptr<SipMessage> forDel(received);
               MethodTypes meth = received->header(h_RequestLine).getMethod();
               ErrLog ( << "Server received: " << MethodNames[meth]);
               if ( meth == INVITE )
               {
                  Data localTag = Helper::computeTag(4);
                  auto_ptr<SipMessage> msg180(Helper::makeResponse(*received, 180, contact));
                  msg180->header(h_To).param(p_tag) = localTag;
                  ErrLog( << "Sent 180");
                  mStack.send( *msg180);

                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, contact));
                  msg200->header(h_To).param(p_tag) = localTag;
                  ErrLog( << "Sent 200");
                  mStack.send(*msg200);
               }
               if ( meth == BYE)
               {
                  auto_ptr<SipMessage> msg200(Helper::makeResponse(*received, 200, contact));
                  calls--;
                  ErrLog( << "Sent 200 to BYE");
                  mStack.send(*msg200);
               }
            }
         }
         UInt64 endTime = Timer::getTimeMs();

         CritLog(<< "Completed: " << mNumCalls << " calls in " << endTime - startTime << "ms, " 
                 << mNumCalls*1000 / (float)(endTime - startTime) << " CPS");
      }
   private:
      SipStack& mStack;
      int mNumCalls;
      TransportType mTransport;
};


int
main(int argc, char* argv[])
{
   if (argc != 4)
   {
      cerr << argv[0] << " LOG_LEVEL NUM_CALLS PROTOCOL" << endl;
      exit(-1);
   } 
   Log::initialize(Log::COUT, Log::toLevel(argv[1]), argv[0]);
   SipStack stack;


   TransportType protocol = UDP;
   if (strcasecmp(argv[3], "UDP") == 0)
   {
      protocol = UDP;
   }
   else if (strcasecmp(argv[3], "TCP") == 0)
   {
      protocol = TCP;
   }
   else
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI PROTOCOL" << endl;
   }

   stack.addTransport(protocol, 5070);


   int numCalls = atoi(argv[2]);

   if (numCalls == 0)
   {
      cerr << argv[0] << " LOG_LEVEL NUM_CALLS" << endl;
      exit(-1);
   } 

   Server server(stack, numCalls, protocol);
   
   server.run();
   server.join();
   return 0;
}
