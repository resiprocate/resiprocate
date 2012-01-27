/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include "resip/stack/ApiCheckList.hxx"
#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/EsLogger.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/ThreadIf.hxx"
#include "resip/stack/TransactionUser.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
class NITServer : public TransactionUser, public ThreadIf
{
   public:
      NITServer(SipStack& stack,size_t selectTime) 
      : mStack(stack),
         mSelectTime(selectTime),
         mName("NITServer")
      {}
      virtual ~NITServer(){}
      virtual void thread()
      {
         while(!isShutdown())
         {
            FdSet fdset; 
            mStack.buildFdSet(fdset);
            fdset.selectMilliSeconds(mSelectTime); 
            mStack.process(fdset);
            while(mFifo.messageAvailable())
            {
               Message* msg = mFifo.getNext();
               InfoLog(<< "Handling msg in server.");
               SipMessage* request=0;
               if ( (request=dynamic_cast<SipMessage*>(msg)) )
               {
                  assert(request->isRequest() && request->method()==REGISTER);
                  SipMessage response;
                  Helper::makeResponse(response, *request, 200);
                  mStack.send(response,this);
               }
               delete msg;
            }

         }
      }
      
      virtual const resip::Data& name() const {return mName;};
   private:
      SipStack& mStack;
      size_t mSelectTime;
      Data mName;

};

class NITClient : public TransactionUser, public ThreadIf
{
   public:
      NITClient(SipStack& stack, 
                  size_t runs, 
                  size_t concurrentTransactions, 
                  NameAddr& target, 
                  NameAddr& contact, 
                  NameAddr& from, 
                  int localPort, 
                  int remotePort, 
                  size_t selectTime) 
      : mStack(stack),
         mRuns(runs),
         mConcurrentTransactions(concurrentTransactions),
         mTarget(target),
         mContact(contact),
         mFrom(from),
         mLocalPort(localPort),
         mRemotePort(remotePort),
         mSelectTime(selectTime),
         mName("NITClient")
      {}
      virtual ~NITClient(){}

      virtual void thread()
      {
         size_t count=0;
         UInt64 startTime = Timer::getTimeMs();
         UInt64 lastTime = startTime;
         size_t sent=0;
         size_t outstanding=0;
         WarningLog(<< "Total runs/total time:last x runs/time");
         while(count < mRuns)
         {
            InfoLog (<< "count=" << count << " messages=" <<mRuns);
            
            // load up the send window
            while (sent < mRuns && outstanding < mConcurrentTransactions)
            {
               InfoLog (<< "Sending " << count << " / " << mRuns << " (" 
                           << outstanding << ")");
               mTarget.uri().port() = mRemotePort; // +(sent%window);
               SipMessage* next=Helper::makeRegister( mTarget, mFrom, mContact);
               next->header(h_Vias).front().sentPort() = mLocalPort;
               mStack.send(*next,this);
               outstanding++;
               sent++;
               delete next;
            }

            FdSet fdset; 
            mStack.buildFdSet(fdset);
            fdset.selectMilliSeconds(mSelectTime); 
            mStack.process(fdset);
            while(mFifo.messageAvailable())
            {
               Message* msg = mFifo.getNext();
               InfoLog(<< "Handling msg in client.");
               SipMessage* response=0;
               if ( (response=dynamic_cast<SipMessage*>(msg)) )
               {
                  assert(response->isResponse() 
                           && response->method()==REGISTER);
                  outstanding--;
                  count++;
      
                  if(count%1000==0)
                  {
                     UInt64 time=Timer::getTimeMs();
                     WarningLog(<< count << "/" << time - startTime<< ":" 
                                 << 1000 << "/" << time-lastTime);
                     lastTime=time;
                  }
               }
               delete msg;
            }
         }
      }
      
      virtual const resip::Data& name() const {return mName;};
      
   private:
      SipStack& mStack;
      size_t mRuns;
      size_t mConcurrentTransactions;
      NameAddr& mTarget;
      NameAddr& mContact;
      NameAddr& mFrom;
      int mLocalPort;
      int mRemotePort;
      size_t mSelectTime;
      Data mName;

};

class InviteServer : public TransactionUser, public ThreadIf
{
   public:
      InviteServer(SipStack& stack, size_t selectTime) 
      : mStack(stack),
         mSelectTime(selectTime),
         mName("InviteServer")
      {}
      virtual ~InviteServer(){}
      virtual void thread()
      {
         while(!isShutdown())
         {
            FdSet fdset; 
            mStack.buildFdSet(fdset);
            fdset.selectMilliSeconds(mSelectTime); 
            mStack.process(fdset);

            while(mFifo.messageAvailable())
            {
               Message* msg = mFifo.getNext();
               InfoLog(<< "Handling msg in server.");
               SipMessage* request=0;
               static NameAddr contact;
               if ( (request=dynamic_cast<SipMessage*>(msg)) )
               {
                  assert(request->isRequest());
                  SipMessage response;
                  switch(request->method())
                  {
                     case INVITE:
                     {
                        DeprecatedDialog dlg(contact);
                        dlg.makeResponse(*request, response, 180);
                        mStack.send(response,this);               
                        dlg.makeResponse(*request, response, 200);
                        mStack.send(response,this);               
                        break;
                     }
         
                     case ACK:
                        break;
         
                     default:
                        assert(0);
                        break;
                  }
               }
               delete msg;
            }
         }
      }
   
      virtual const resip::Data& name() const {return mName;};
   private:
      SipStack& mStack;
      size_t mSelectTime;
      Data mName;

};

class InviteClient : public TransactionUser, public ThreadIf
{
   public:
      InviteClient(SipStack& stack, 
                  size_t runs, 
                  size_t concurrentTransactions, 
                  NameAddr& target, 
                  NameAddr& contact, 
                  NameAddr& from, 
                  int localPort, 
                  int remotePort, 
                  size_t selectTime) 
      : mStack(stack),
         mRuns(runs),
         mConcurrentTransactions(concurrentTransactions),
         mTarget(target),
         mContact(contact),
         mFrom(from),
         mLocalPort(localPort),
         mRemotePort(remotePort),
         mSelectTime(selectTime),
         mName("InviteClient")
      {}
      virtual ~InviteClient(){}

      virtual void thread()
      {
         size_t count=0;
         UInt64 startTime = Timer::getTimeMs();
         UInt64 lastTime = startTime;
         size_t sent=0;
         WarningLog(<< "Total runs/total time:last x runs/time");
         while(count < mRuns)
         {
            InfoLog (<< "count=" << count << " messages=" <<mRuns);
            
            size_t outstanding=0;
            // load up the send window
            while (sent < mRuns && outstanding < mConcurrentTransactions)
            {
               InfoLog (<< "Sending " << count << " / " << mRuns << " (" 
                           << outstanding << ")");
               mTarget.uri().port() = mRemotePort; // +(sent%window);
               SipMessage* next = Helper::makeInvite( mTarget, mFrom, mContact);
               next->header(h_Vias).front().sentPort() = mLocalPort;
               mStack.send(*next,this);
               outstanding++;
               sent++;
               delete next;
            }

            FdSet fdset; 
            mStack.buildFdSet(fdset);
            fdset.selectMilliSeconds(mSelectTime); 
            mStack.process(fdset);
         
            while(mFifo.messageAvailable())
            {
               Message* msg = mFifo.getNext();
               InfoLog(<< "Handling msg in client.");
               SipMessage* response=0;
               if ( (response=dynamic_cast<SipMessage*>(msg)) )
               {
                  assert(response->isResponse() && response->method()==INVITE);
                  if (response->header(h_StatusLine).statusCode() == 200)
                  {
                     outstanding--;
                     count++;
   
                     DeprecatedDialog dlg(mContact);
                     dlg.createDialogAsUAC(*response);
                     SipMessage* ack = dlg.makeAck();
                     mStack.send(*ack,this);
                     delete ack;
                  }
      
                  if(count%1000==0)
                  {
                     UInt64 time=Timer::getTimeMs();
                     WarningLog(<< count << "/" << time - startTime<< ":" 
                                 << 1000 << "/" << time-lastTime);
                     lastTime=time;
                  }
               }
               delete msg;
            }
         }
      }
      
      virtual const resip::Data& name() const {return mName;};
   private:
      SipStack& mStack;
      size_t mRuns;
      size_t mConcurrentTransactions;
      NameAddr& mTarget;
      NameAddr& mContact;
      NameAddr& mFrom;
      int mLocalPort;
      int mRemotePort;
      size_t mSelectTime;
      Data mName;

};
}

int
main(int argc, char* argv[])
{

   const char* logType = "cout";
   const char* logLevel = "ALERT";
   const char* proto = "tcp";
   const char* bindAddr = 0;

   int runs = 50000;
   int window = 100;
   int seltime = 25;
   int v6 = 0;
   int invite=0;
   int quiet=0;
   
#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of concurrent transactions", 0},
      { "select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,     0, "protocol to use (tcp | udp)", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      {"v6",          '6', POPT_ARG_NONE,   &v6     ,   0, "ipv6", 0},
      {"invite",      'i', POPT_ARG_NONE,   &invite     ,   0, "send INVITE/BYE instead of REGISTER", 0},
      {"quiet",      'q', POPT_ARG_NONE,   &quiet     ,   0, "minimal output (only write transactions per second)", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   estacado::EsLogger::init(logType, argc, argv, log4cplus::WARN_LOG_LEVEL);
   if(!quiet)
   {
      cout << "Performing " << runs << " runs." << endl;
   }

   IpVersion version = (v6 ? V6 : V4);
   SipStack receiver;
   SipStack sender;
   
//   sender.addTransport(UDP, 25060, version); // !ah! just for debugging TransportSelector
//   sender.addTransport(TCP, 25060, version);

   int senderPort = 25070 + (rand()& 0x7fff);   
   if (bindAddr)
   {
      InfoLog(<<"Binding to address: " << bindAddr);
      sender.addTransport(UDP, senderPort, version, StunDisabled, bindAddr);
      sender.addTransport(TCP, senderPort, version, StunDisabled, bindAddr);
   }
   else
   {
      sender.addTransport(UDP, senderPort, version);
      sender.addTransport(TCP, senderPort, version);
   }

   int registrarPort = 25080 + (rand()& 0x7fff);   
   receiver.addTransport(UDP, registrarPort, version);
   receiver.addTransport(TCP, registrarPort, version);


   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = bindAddr ? bindAddr :DnsUtil::getLocalHostName();
   target.uri().port() = registrarPort;
   target.uri().param(p_transport) = proto;
  
   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";

#ifdef WIN32
     target.uri().host() = Data("127.0.0.1");
#endif

   NameAddr from = target;
   from.uri().port() = senderPort;
   
   ThreadIf* server=0;
   ThreadIf* client=0;
   sender.run();
   receiver.run();
   if(invite)
   {
      server=new InviteServer(receiver,seltime);
      client=new InviteClient(sender, runs, window, target, contact, 
                              from, senderPort, registrarPort, seltime);
      
   }
   else
   {
      server=new NITServer(receiver,seltime);
      client=new NITClient(sender, runs, window, target, contact, 
                              from, senderPort, registrarPort, seltime);
   }

   receiver.registerTransactionUser(*dynamic_cast<TransactionUser*>(server));
   sender.registerTransactionUser(*dynamic_cast<TransactionUser*>(client));
   
   UInt64 startTime = Timer::getTimeMs();

   server->run();
   client->run();
   client->join();

   InfoLog (<< "Finished " << runs << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   if (!invite)
   {
      if(quiet)
      {
         cout << runs / ((float) elapsed / 1000.0) << endl;
      }
      else
      {
         cout << runs << " registrations peformed in " << elapsed << " ms, a rate of " 
              << runs / ((float) elapsed / 1000.0) << " transactions per second.]" << endl;
      }
   }
   else
   {
      if(quiet)
      {
         cout << runs / ((float) elapsed / 1000.0) << endl;
      }
      else
      {
         cout << runs << " calls peformed in " << elapsed << " ms, a rate of " 
              << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
      }
   }
   
   if(!quiet)
   {
      cout << "Note: this test runs both sides (client and server)" << endl;
   }
   delete server;
   delete client;
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
   return 0;
}
/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
