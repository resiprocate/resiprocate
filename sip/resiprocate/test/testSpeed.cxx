
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

#include "resiprocate/sipstack/Helper.hxx"
#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/sipstack/SipStack.hxx"
#include "resiprocate/sipstack/Dialog.hxx"
#include "resiprocate/util/Logger.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::ERR, argv[0]);
   
   Log::toLevel( Data("DEBUG") );

   //const Data& ret = ParameterTypes::ParameterNames[ParameterTypes::transport];

   SipStack stack1;
   SipStack stack2;
   stack1.addTransport(Transport::UDP, 5070);
   stack2.addTransport(Transport::UDP, 5080);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "localhost";
   dest.uri().port() = 5080;
   dest.uri().param(p_transport) = "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5070;
   
#ifdef WIN32
   int totalCalls = 5;
#else
   int totalCalls = 500;
#endif

   bool done[10000];
   assert( sizeof(done)/sizeof(bool) > (unsigned int)(totalCalls+1) );
   for ( int ii=0; ii<=totalCalls; ii++ )
   {
      done[ii]=false;
   }
   
   int lastSent=0;
   int lastRecv=0;
   int i=0;
   
   UInt64 startTime = Timer::getTimeMs();

   while ( lastRecv < totalCalls )
   {
      InfoLog( << "loop " << i++  );
      
      if ( (lastRecv >= lastSent) && (lastSent <= totalCalls) )
      {
         Data count( ++lastSent );
         from.uri().user() = count;
         auto_ptr<SipMessage> message = auto_ptr<SipMessage>(Helper::makeInvite( dest, from, from));
         stack1.send(*message);
         InfoLog( << "Stack1 sent msg from user: " << count );
      }
         
      FdSet fdset;
      stack1.buildFdSet(fdset);
      stack2.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(0);
      assert (err != -1);

      stack1.process(fdset);
      stack2.process(fdset);

      SipMessage* received1 = (stack1.receive());
      if (received1)
      {
         InfoLog (<< "stack1 got: " << received1->brief() 
                  << " from user: " << received1->header(h_From).uri().user() );

         int user = atoi( received1->header(h_From).uri().user().c_str() );
      
         if ( (received1->isResponse()) )
         {
            if ( received1->header(h_StatusLine).responseCode() == 200 )
            {
               if ( !done[user] )
               {
                  DebugLog(<< "Creating dialog.");
                  done[user]=true;
               
                  Dialog dlog(from);
               
                  DebugLog(<< "Creating dialog as UAC.");
                  dlog.createDialogAsUAC(*received1);
               
                  DebugLog(<< "making ack.");
                  auto_ptr<SipMessage> ack(dlog.makeAck(*received1) );
               
                  DebugLog(<< "making bye.");
                  auto_ptr<SipMessage> bye(dlog.makeBye());
               
                  DebugLog(<< "Sending ack: << *ack");
                  stack1.send(*ack);
               
#if 0
                    FdSet fdset;
      stack1.buildFdSet(fdset);
      stack2.buildFdSet(fdset);
      int err = fdset.select(0);
      assert (err != -1);
	  
	  stack1.process(fdset);
#endif

                  DebugLog(<< "Sending bye: << *bye");
                  stack1.send(*bye);  
               }
               else
               { 
                  if ( user > lastRecv )
                  {
                     lastRecv = user;
                  }
               }
            }
         }     
         
         delete received1;
      }
               
      SipMessage* received2 = (stack2.receive());
      if (received2)
      {
         InfoLog (<< "stack2 got: " << received2->brief() 
                  << " from user: " << received2->header(h_From).uri().user() );   
 
         MethodTypes meth = received2->header(h_RequestLine).getMethod();

         if ( meth == INVITE )
         {
            //Data localTag = Helper::computeTag(4);

            auto_ptr<SipMessage> msg180(Helper::makeResponse(*received2, 180, dest, "Ringing"));
            //msg180->header(h_To).uri().param(p_tag) = localTag;
            stack2.send( *msg180);
            
#if 0
           FdSet fdset;
      stack1.buildFdSet(fdset);
      stack2.buildFdSet(fdset);
      int err = fdset.select(0);
      assert (err != -1);
	  stack2.process(fdset); // !cj! seems to be a bug that cuases this to be
            // needed 
#endif

            auto_ptr<SipMessage> msg200(Helper::makeResponse(*received2, 200, dest, "OK"));
            //msg200->header(h_To).uri().param(p_tag) = localTag;
            stack2.send(*msg200);
         }

         if ( meth == BYE )
         {
            auto_ptr<SipMessage> msg200(Helper::makeResponse(*received2, 200, dest, "OK"));
            InfoLog (<< "stack2 got bye - send 200 : " << *msg200 );   
            
            stack2.send(*msg200);
         }

         delete received2;
      }
   }
  
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << 2.0 * totalCalls * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
        << " half calls/s/GHz  ["
        << totalCalls << " calls peformed in " << elapsed << " ms, a rate of " 
        << totalCalls / ((float) elapsed / 1000.0) << " calls per second.]" << endl;

   return 0;
}
