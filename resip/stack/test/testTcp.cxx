#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <signal.h>
#include <iostream>

#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/DataStream.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

void
process(TcpBaseTransport* sender,
        TcpBaseTransport* receiver)
{
   FdSet fdset; 
   if (receiver) receiver->buildFdSet(fdset);
   sender->buildFdSet(fdset);

   fdset.selectMilliSeconds(1); 

   try
   {
      if (receiver) receiver->process(fdset);
   }
   catch(std::exception& e)
   {
      // .bwc. Do nothing substantive, since the stack thread doesn't
   }
   sender->process(fdset);
}

int
main(int argc, char* argv[])
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

#ifdef WIN32
   initNetwork();
#endif

   char* logType = 0;
   const char* logLevel = "WARNING";
   int runs = 100;
   int window = 10;
   int seltime = 100;

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of registrations in test", 0},
      {"select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
   poptFreeContext(context);
#endif

#ifdef WIN32
   Log::initialize(Log::Cout, Log::Info, "testTcp");   
#else
   Log::initialize(logType, logLevel, argv[0]);
#endif
   
   cout << "Performing " << runs << " runs." << endl;
   
   Fifo<TransactionMessage> txFifo;
   TcpTransport* sender = new TcpTransport(txFifo, 5870, V4, Data::Empty);

   Fifo<TransactionMessage> rxFifo;
   TcpTransport* receiver = new TcpTransport(rxFifo, 5880, V4, Data::Empty);
   
   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = "localhost";
   target.uri().port() = 5880;
   target.uri().param(p_transport) = "tcp";
   
   NameAddr from = target;
   from.uri().port() = 5870;

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
         int contentLength=resip::Random::getRandom()%65535;
         std::string body(contentLength,'0');
         std::auto_ptr<Contents> contents(new PlainContents(Data(body.data(), body.size())));
         m->setContents(contents);
         int headerLength=resip::Random::getRandom()%1024;
         std::string bigHeader(headerLength,'h');
         m->header(h_Subject).value()=Data(bigHeader.data(), bigHeader.size());
         messages.push_back(m);
      }

      UInt64 elapsed = Timer::getTimeMs() - startTime;
      cout << runs << " calls performed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      
      InfoLog (<< "Messages created");
   }
   
   //delete receiver;
   //receiver=0;
   
   in_addr in;
   DnsUtil::inet_pton("127.0.0.1", in);
   Tuple dest(in, target.uri().port(), TCP);
   InfoLog (<< "Sending to " << dest);
   
   UInt64 startTime = Timer::getTimeMs();

   int tid=1;
   int outstanding=0;

   while (!messages.empty())
   {
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
            delete next;
         }
         std::auto_ptr<SendData> toSend(sender->makeSendData(dest, encoded, Data(tid++), Data::Empty));
         sender->send(toSend);
      }

      FdSet fdset; 
      if (receiver) receiver->buildFdSet(fdset);
      sender->buildFdSet(fdset);

      fdset.selectMilliSeconds(seltime); 
      
      if (receiver) receiver->process(fdset);
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
         }
         delete msg;
      }
   }

   while (outstanding>0)
   {
      FdSet fdset; 
      if (receiver) receiver->buildFdSet(fdset);
      sender->buildFdSet(fdset);

      fdset.selectMilliSeconds(seltime); 
      
      if (receiver) receiver->process(fdset);
      sender->process(fdset);

      while (rxFifo.messageAvailable())
      {
         Message* msg = rxFifo.getNext();
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
         }
         delete msg;
      }
   }

   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << runs << " calls peformed in " << elapsed << " ms, a rate of " 
        << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;

   SipMessage::checkContentLength=false;
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
      cout << runs << " calls performed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      
      InfoLog (<< "Messages created");
   }

   UInt32 type=0;
   Data badContentLength1("-1");
   Data badContentLength2("999999999999999999999999999999");
   std::string hugeString(ConnectionBase::ChunkSize*2,'h');
   ExtensionHeader h_huge(Data::from(hugeString));

   while (!messages.empty())
   {
      // We need a well formed message to test that any traffic has
      // gotten through at all.
      Data wellFormed;
      std::auto_ptr<SipMessage> next(messages.front());
      messages.pop_front();
            
      {
         DataStream strm(wellFormed);
         next->encode(strm);
         outstanding++;
      }

      Data garbage;
      const UInt32 NEGATIVE_CONTENT_LENGTH = 0;
      const UInt32 HUGE_CONTENT_LENGTH = 1;
      const UInt32 HUGE_HEADER_NAME = 2;
      const UInt32 HUGE_HEADER_VALUE = 3;

      switch(type%4)
      {
         case NEGATIVE_CONTENT_LENGTH:
         case HUGE_CONTENT_LENGTH:
         {
            garbage = wellFormed;
            // .bwc. Send one at a time for maximum potential damage.
            garbage.replace("Content-Length: 0","Content-Length: "+( type == NEGATIVE_CONTENT_LENGTH ? badContentLength1 : badContentLength2));
         }
         break;
         case HUGE_HEADER_NAME:
         {
            DataStream strm(garbage);
            next->header(h_Subject).value()=Data(hugeString.data(), hugeString.size());
            next->encode(strm);
         }
         break;
         case HUGE_HEADER_VALUE:
         {
            DataStream strm(garbage);
            next->header(h_huge).push_front(StringCategory("foo"));
            next->encode(strm);
         }
         break;
      }

      ++type;
      // Send a garbage request, followed by a good request
      std::auto_ptr<SendData> garbageSend(sender->makeSendData(dest, garbage, Data(tid++), Data::Empty));
      sender->send(garbageSend);

     
      for(int p=0; p < 10; ++p)
      {
         process(sender, receiver);
         std::auto_ptr<Message> msg(rxFifo.getNext(1));
         SipMessage* received = dynamic_cast<SipMessage*>(msg.get());
         // .bwc. These are all unrecoverable garbage, we should not get
         // any sip traffic on this fifo.
         if(received)
         {
            cout << "Unexpected message received: " << *received << endl;
         }
         assert(!received);
      }

      // Send a stun ping to make sure the sender has noticed the connection
      // is closed.
      std::auto_ptr<SendData> ping(sender->makeSendData(dest, "\r\n\r\n", Data(tid++), Data::Empty));
      sender->send(ping);

      // Throw in a process call to ensure that both sides have
      // torn down the connection.
      process(sender, receiver);

      // Verify that good traffic can come through
      std::auto_ptr<SendData> goodSend(sender->makeSendData(dest, wellFormed, Data(tid++), Data::Empty));
      sender->send(goodSend);
      bool failedToReceiveGoodMessage = true;
      for(int p=0; p < 10; ++p)
      {
         process(sender, receiver);
         std::auto_ptr<Message> msg(rxFifo.getNext(10));
         SipMessage* received = dynamic_cast<SipMessage*>(msg.get());
         if(received)
         {
            cout << *received << endl;
            failedToReceiveGoodMessage = false;
            break;
         }
      }      
      assert(!failedToReceiveGoodMessage);
   }

   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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

// vim: softtabstop=3:shiftwidth=3:expandtab

