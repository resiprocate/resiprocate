#include <iostream>

#include "resiprocate/TcpTransport.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DataStream.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
   int runs = 50000;
   int window = 50;
   int seltime = 100;
   
   if (argc == 2)
   {
      runs = atoi(argv[1]);
   }

   cout << "Performing " << runs << " runs." << endl;
   
   Fifo<Message> txFifo;
   TcpTransport* sender = new TcpTransport(txFifo, 5070, Data::Empty);

   Fifo<Message> rxFifo;
   TcpTransport* receiver = new TcpTransport(rxFifo, 5080, Data::Empty);

   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = "localhost";
   target.uri().port() = 5080;
   target.uri().param(p_transport) = "tcp";
   
   NameAddr from = target;
   from.uri().port() = 5070;

   InfoLog (<< "Creating messages");

   list<SipMessage*> messages;
   {
      UInt64 startTime = Timer::getTimeMs();
      for (int i=0; i<runs; i++)
      {
         SipMessage* m = Helper::makeInvite( target, from, from);      
         m->header(h_Vias).front().transport() = Tuple::toData(sender->transport());
         m->header(h_Vias).front().sentHost() = "localhost";
         m->header(h_Vias).front().sentPort() = sender->port();
      
         messages.push_back(m);
      }

      UInt64 elapsed = Timer::getTimeMs() - startTime;
      cout <<  runs * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
           << " half calls/s/GHz  ["
           << runs << " calls peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      
      InfoLog (<< "Messages created");
   }
   
   
   in_addr in;
   DnsUtil::inet_pton("127.0.0.1", in);
   Tuple dest(in, target.uri().port(), TCP);
   InfoLog (<< "Sending to " << dest);
   
   UInt64 startTime = Timer::getTimeMs();

   int tid=1;
   int outstanding=0;
   int count=0;
   while (!messages.empty())
   {
      if (count > 500) exit(0);
      // load up the send window
      while (outstanding < window)
      {
         Data encoded;
         {
            DataStream strm(encoded);
            SipMessage* next = messages.front();
            messages.pop_front();
            next->encode(strm);
            outstanding++;
         }
         sender->send(dest, encoded, Data(tid++));
      }

      FdSet fdset; 
      receiver->buildFdSet(fdset);
      sender->buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(seltime);
      
      receiver->process(fdset);
      sender->process(fdset);
      
      Message* msg;
      if (rxFifo.messageAvailable())
      {
         msg = rxFifo.getNext();
         SipMessage* received = dynamic_cast<SipMessage*>(msg);
         if (received)
         {
            //DebugLog (<< "got: " << received->brief());
            outstanding--;
         
            assert (received->header(h_RequestLine).uri().host() == "localhost");
            assert (received->header(h_To).uri().host() == "localhost");
            assert (received->header(h_From).uri().host() == "localhost");
            assert (!received->header(h_Vias).begin()->sentHost().empty());
            assert (received->header(h_Contacts).begin()->uri().host() == "localhost");
            assert (!received->header(h_CallId).value().empty());
            delete received;
         }
      }
   }

   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout <<  runs * ( 1000.0 / (float) elapsed) * ( 1000.0 / (float)Timer::getCpuSpeedMhz() ) 
        << " half calls/s/GHz  ["
        << runs << " calls peformed in " << elapsed << " ms, a rate of " 
        << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;

   return 0;
}
