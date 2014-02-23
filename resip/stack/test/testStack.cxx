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

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/GeneralCongestionManager.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "rutil/SelectInterruptor.hxx"
#include "resip/stack/TransportThread.hxx"
#include "resip/stack/InterruptableStackThread.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/Uri.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

/************************************************************************

  This application tests the core resip stack as a whole. It does
  this by creating two instances of SipStack, and sending messages
  (transactions) between them.

  Resiprocate is designed to support a variety of threading modeling
  and event handling models (e.g., select, poll, epoll system calls),
  and this test application can excercise various combinations.

  =============================================
  Options: --thread-type, --seltime, --intepoll
  The various permutations and are controlled via the --thread-type
  option.  Note that because this test application runs two stacks, its
  threading models are unusual, and not a good template for a "normal"
  application.

  The default thread-type is now "event" to better model "typical"
  applications.  The old behavior (pre-2011) can be obtained with
  "--thread-type=common"; this is useful for debugging stack internal
  issues since fewer threads running around.

  Values are:

    none        Everything (both stacks and test application) run in same
                thread. App block for time specified by --seltime each
                message cycle.  This defaults to zero, meaning app will
                loop without every waiting, looking for any work to do.
                When non-zero, app will wait this before checking queues.
                Because of the endless spinning checking for work to do,
                this mode is not useful for profiling.

    common      Like "none", everything (both stacks and test application)
                run in same thread. Thread blocks until something to do,
                and stacks will be configured to break the wait loop
                when there is something for app to do. This avoids the
                endless spinning of "none", and is useful for profiling.

    std         Each stack runs in its own thread, and that thread is
                a "standard" select-based thread. This mode is now obsolete.

    intr        Each stack runs in its own thread, and that thread
                is an interruptable select-based thread. Here, interruptable
                means that when the app gives the thread work to do, it will
                immediate wake up and do it, instead of waiting for
                the select interval to expire. This mode is now obsolete.

    event       Each stack runs in its own thread, and that thread
                is an interruptable event-loop based thread. The specific
                type of event-loop depends upon the underlying platform
                and how you compiled the stack.

    epoll       Like "event", but specifically uses the epoll implmentation.

    fdset       Like "event", but specifically uses the FdSet/select
                implmentation.

  ===============
  Option: --bind
  The test application creates two stack and sends messages (requests
  and responses) between them using UDP or TCP transports. The --bind
  option controls where the UAS side is listening. This defaults to
  "127.0.0.1"; we use this as default because it avoids any dependency on DNS.

  If DNS is working on your system, then you can set this to the DNS name
  of one of your interfaces. Or you can use --bind=='' and we'll try
  to figure out your local DNS name.


************************************************************************/

class SharedAsyncNotify : public AsyncProcessHandler
{
   public:
      SharedAsyncNotify() { };
      virtual ~SharedAsyncNotify() { };

      virtual void handleProcessNotification();

      bool waitNotify(int ms);

   protected:
      Mutex mMutex;
      Condition mCondition;
};

void
SharedAsyncNotify::handleProcessNotification()
{
   Lock lock(mMutex); (void)lock;
   mCondition.signal();
}

/**
   {ms}<0 -> wait forever
   {ms}=0 -> don't wait
   {ms}>0 -> Wait this many ms
   Returns true if condition was signalled (false if timer
   or other interrupt)
**/
bool
SharedAsyncNotify::waitNotify(int ms)
{
   Lock lock(mMutex); (void)lock;

   if (ms<0)
   {
      mCondition.wait(mMutex);
      return true;
   }
   else
   {
      return mCondition.wait(mMutex, ms);
   }
}

class SipStackAndThread
{
   public:
      SipStackAndThread(const char *tType,
        AsyncProcessHandler *notifyDn=0,
        AsyncProcessHandler *notifyUp=0);
         ~SipStackAndThread() {
         destroy();
      }

      SipStack&         getStack() const { assert(mStack); return *mStack; }

      // I don't know if these are such a good idea
      SipStack&         operator*() const { assert(mStack); return *mStack; }
      SipStack*         operator->() const { return mStack; }

      void setCongestionManager(CongestionManager* cm)
      {
         mStack->setCongestionManager(cm);
      }

      void              run() 
      {
         if ( mThread ) mThread->run();
         if ( mMultiThreadedStack ) mStack->run();
      }
      void              shutdown()
      {
         if ( mThread ) mThread->shutdown();
      }
      void              join()
      {
         if ( mThread ) mThread->join();
         if ( mMultiThreadedStack ) mStack->shutdownAndJoinThreads();
      }

      void              destroy();

      // dis-allowed by not-implemented
      SipStackAndThread& operator=(SipStackAndThread&);

   protected:

      SipStack          *mStack;
      ThreadIf          *mThread;
      SelectInterruptor *mSelIntr;
      FdPollGrp         *mPollGrp;
      EventThreadInterruptor    *mEventIntr;
      bool mMultiThreadedStack;
};


SipStackAndThread::SipStackAndThread(const char *tType,
 AsyncProcessHandler *notifyDn, AsyncProcessHandler *notifyUp)
  : mStack(0), 
      mThread(0), 
      mSelIntr(0), 
      mPollGrp(0), 
      mEventIntr(0), 
      mMultiThreadedStack(false)
{
   bool doStd = false;

   assert( tType );

   if (strcmp(tType,"intr")==0) 
   {
      mSelIntr = new SelectInterruptor();
   }
   else if ( strcmp(tType,"event")==0
          || strcmp(tType,"epoll")==0
          || strcmp(tType,"fdset")==0
          || strcmp(tType,"poll")==0 )
   {
      mPollGrp = FdPollGrp::create(tType);
      mEventIntr = new EventThreadInterruptor(*mPollGrp);
   }
   else if ( strcmp(tType,"std")==0 ) 
   {
      doStd = true;
   } 
   else if ( strcmp(tType,"none")==0 ) 
   {
   }
   else if ( strcmp(tType,"multithreadedstack")==0 )
   {
      mMultiThreadedStack=true;
      mPollGrp = FdPollGrp::create("event");
      mEventIntr = new EventThreadInterruptor(*mPollGrp);
   }
   else 
   {
      CritLog(<<"Bad thread-type: "<<tType);
      exit(1);
   }
   SipStackOptions options;
   options.mAsyncProcessHandler = mEventIntr?mEventIntr
      :(mSelIntr?mSelIntr:notifyDn);
   options.mPollGrp = mPollGrp;
   mStack = new SipStack(options);
   
   mStack->setFallbackPostNotify(notifyUp);
   if (mEventIntr) 
   {
      mThread = new EventStackThread(*mStack, *mEventIntr, *mPollGrp);
   } 
   else 
   if (mSelIntr) 
   {
      mThread = new InterruptableStackThread(*mStack, *mSelIntr);
   } 
   else if (doStd) 
   {
      mThread = new StackThread(*mStack);
   }
}

void
SipStackAndThread::destroy()
{
   if ( mThread )
   {
      delete mThread;
      mThread = 0;
   }
   if ( mStack )
   {
      delete mStack;
      mStack = 0;
   }
   if ( mSelIntr )
   {
      delete mSelIntr;
      mSelIntr = 0;
   }
   if ( mEventIntr )
   {
      delete mEventIntr;
      mEventIntr = 0;
   }
   if ( mPollGrp )
   {
      delete mPollGrp;
      mPollGrp = 0;
   }
}

static void
waitForTwoStacks(SipStackAndThread& receiver, SipStackAndThread& sender,
                 SelectInterruptor *commonIntr, int& thisseltime, bool& isStrange)
{
   FdSet fdset;
   receiver->buildFdSet(fdset);
   sender->buildFdSet(fdset);
   if ( commonIntr )
   {
      commonIntr->buildFdSet(fdset);
   }

   if ( thisseltime > 0 )
   {
      unsigned int stackMs = resipMin(
              receiver->getTimeTillNextProcessMS(),
              sender->getTimeTillNextProcessMS());
      thisseltime = resipMin((unsigned)thisseltime, stackMs);
   }
   int numReady = fdset.selectMilliSeconds(thisseltime);

   isStrange = (thisseltime > 4000 && numReady==0);
   if ( commonIntr )
   {
      commonIntr->process(fdset);
   }
   receiver->process(fdset);
   sender->process(fdset);
}

struct StackThreadPair
{
   StackThreadPair(SipStackAndThread& receiver,
     SipStackAndThread& sender, SharedAsyncNotify& sharedUp)
      : mReceiver(receiver), mSender(sender), mSharedUp(sharedUp),
       mSeltime(0), mNoStackThread(false), mCommonIntr(0)
   {
   }

   bool wait(int& thisseltime);

   SipStackAndThread& mReceiver;
   SipStackAndThread& mSender;
   SharedAsyncNotify& mSharedUp;
   int mSeltime;
   bool mNoStackThread;
   SelectInterruptor *mCommonIntr;
};

bool
StackThreadPair::wait(int& thisseltime) 
{
   if(mReceiver.getStack().hasMessage() || mSender.getStack().hasMessage())
   {
      return false;
   }

   thisseltime = mSeltime;
   bool isStrange = false;
   if ( mNoStackThread )
   {
      // handles 'none' and 'common' thread-type
      // if none, then commonIntr will be NULL
      waitForTwoStacks( mReceiver, mSender, mCommonIntr, thisseltime, isStrange);
   }
   else
   {
      thisseltime = 4000;
      bool gotPost = mSharedUp.waitNotify(thisseltime);
      isStrange = !gotPost;
   }
   return isStrange;
}


static void
performTest(int verbose, int runs, int window, int invite,
      Data& bindIfAddr,
      int numPorts, int senderPort, int registrarPort, const char *proto,
      int sendSleepMs,
      StackThreadPair& pair)
{
   NameAddr target;
   target.uri().scheme() = "sip";
   target.uri().user() = "fluffy";
   target.uri().host() = bindIfAddr;
   target.uri().port() = registrarPort;
   target.uri().param(p_transport) = proto;

   NameAddr contact;
   contact.uri().scheme() = "sip";
   contact.uri().user() = "fluffy";

   NameAddr from = target;
   from.uri().port() = senderPort;

   UInt64 startTime = Timer::getTimeMs();
   int outstanding=0;
   int count = 0;
   int sent = 0;

   int rxReqTryCnt = 0, rxReqHitCnt = 0;
   int rxRspTryCnt = 0, rxRspHitCnt = 0;

   while (count < runs)
   {
      //InfoLog (<< "count=" << count << " messages=" << messages.size());

      // load up the send window
      for (int i=0; i<64 && sent < runs && outstanding < window; ++i)
      {
         DebugLog (<< "Sending " << count << " / " << runs << " (" << outstanding << ")");
         target.uri().port() = registrarPort + (sent%numPorts);

         SipMessage* next=0;
         if (invite)
         {
            next = Helper::makeInvite( target, from, contact);
         }
         else
         {
            next = Helper::makeRegister( target, from, contact);
         }

         // The Via header serves two purposes:
         // (1) tells the recipient where to send the response,
         // (2) selects which Transport we send from
         if (!bindIfAddr.empty() && numPorts > 1)
         {
             // currently TCP only honors Via if host is populated
             // the "numPorts>1" test is for backwards compat
             next->header(h_Vias).front().sentHost() = bindIfAddr;
         }
         next->header(h_Vias).front().sentPort() = senderPort + (sent%numPorts);
         pair.mSender->send(std::auto_ptr<SipMessage>(next));
         next = 0; // DON'T delete next; consumed by send above
         outstanding++;
         sent++;
         if (sendSleepMs>0)
         {
            sleepMs(sendSleepMs);
         }
      }

      int thisseltime = 0;
      bool isStrange = pair.wait(thisseltime);
      if (isStrange)
      {
         cout << "STRANGE: Stuck for long time: "
             <<" sent="<<sent
             <<" done="<<count
             <<" thisseltime="<<thisseltime
             <<endl;
      }

      for (int i=0;i<64;++i)
      {
         static NameAddr contact;

         ++rxReqTryCnt;
         SipMessage* request = pair.mReceiver->receive();
          if (request==NULL)
             break;
         ++rxReqHitCnt;
         assert(request->isRequest());
         SipMessage response;
         switch (request->header(h_RequestLine).getMethod())
         {
            case INVITE:
            {
               DeprecatedDialog dlg(contact);
               dlg.makeResponse(*request, response, 180);
               pair.mReceiver->send(response);
               dlg.makeResponse(*request, response, 200);
               pair.mReceiver->send(response);
               break;
            }

            case ACK:
               break;

            case BYE:
               Helper::makeResponse(response, *request, 200);
               pair.mReceiver->send(response);
               break;

            case REGISTER:
               Helper::makeResponse(response, *request, 200);
               pair.mReceiver->send(response);
               break;
            default:
               assert(0);
               break;
         }
         delete request;
      }

      for (int i=0;i<64;++i)
      {
         ++rxRspTryCnt;
         SipMessage* response = pair.mSender->receive();
         if (response==NULL)
            break;
         ++rxRspHitCnt;
         assert(response->isResponse());
         switch(response->header(h_CSeq).method())
         {
            case REGISTER:
               outstanding--;
               if (response->header(h_StatusLine).statusCode() == 200)
               {
                  count++;
               }
               else
               {
                  --sent;
               }
               break;

            case INVITE:
               if (response->header(h_StatusLine).statusCode() == 200)
               {
                  outstanding--;
                  count++;

                  DeprecatedDialog dlg(contact);
                  dlg.createDialogAsUAC(*response);
                  SipMessage* ack = dlg.makeAck();
                  pair.mSender->send(*ack);
                  delete ack;

                  SipMessage* bye = dlg.makeBye();
                  pair.mSender->send(*bye);
                  delete bye;
               }
               break;

            case BYE:
               break;

            default:
               assert(0);
               break;
         }

         delete response;
      }
   }
   InfoLog (<< "Finished " << count << " runs");

   UInt64 elapsed = Timer::getTimeMs() - startTime;
   if (!invite)
   {
      cout << runs << " registrations performed in " << elapsed << " ms, a rate of "
           << runs / ((float) elapsed / 1000.0) << " transactions per second." << endl;
   }
   else
   {
      cout << runs << " calls performed in " << elapsed << " ms, a rate of "
           << runs / ((float) elapsed / 1000.0) << " calls per second." << endl;
   }
   if ( verbose )
   {
      cout << "Note: this test runs both sides (client and server)" << endl;

      cout << "RxCnts: "
        <<" Req="<<rxReqHitCnt<<"/"<<rxReqTryCnt
        <<" ("<<(rxReqHitCnt*100/rxReqTryCnt)<<"%)"
        <<" Rsp="<<rxRspHitCnt<<"/"<<rxRspTryCnt
        <<" ("<<(rxRspHitCnt*100/rxRspTryCnt)<<"%)"
        << endl;
   }
}

int
main(int argc, char* argv[])
{

   const char* logType = "cout";
   const char* logLevel = "WARNING";
   const char* proto = "tcp";
   const char* bindAddr = "127.0.0.1";
   int doListen = 1;

   int verbose = 0;
   int runs = 10000;
   int window = 100;
   int seltime = 0;
   int v6 = 0;
   int invite=0;
   int numPorts = 1;
   int portBase = 0;
   const char* threadType = "event";
   int tpFlags = 0;
   int sendSleepMs = 0;
   int cManager=0;
   int statisticsInterval=60;

#if defined(HAVE_POPT_H)

   char threadTypeDesc[200];
   strcpy(threadTypeDesc, "none|common|std|intr|multithreadedstack|");
   strcat(threadTypeDesc, FdPollGrp::getImplList());

   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of runs (SIP requests) in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of concurrent transactions", 0},
      {"select-time", 's', POPT_ARG_INT,    &seltime,   0, "polling interval (ms) for stack thread", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,     0, "protocol to use (tcp | udp)", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      {"listen",      0,   POPT_ARG_INT,    &doListen,  0, "do not bind/listen sender ports", 0},
      {"verbose",     0,   POPT_ARG_INT,    &verbose,   0, "verbose", 0},
      {"v6",          '6', POPT_ARG_NONE,   &v6     ,   0, "ipv6", 0},
      {"invite",      'i', POPT_ARG_NONE,   &invite ,   0, "send INVITE/BYE instead of REGISTER", 0},
      {"port",        0,   POPT_ARG_INT,    &portBase,  0, "first port to use", 0},
      {"numports",    'n', POPT_ARG_INT,    &numPorts,  0, "number of parallel sessions(ports)", 0},
      {"thread-type", 't', POPT_ARG_STRING, &threadType,0, "stack thread type", threadTypeDesc},
      {"tf",          0,   POPT_ARG_INT,    &tpFlags,   0, "bit encoding of transportFlags", 0},
      {"sleep",       0,   POPT_ARG_INT,    &sendSleepMs,0, "time (ms) to sleep after each sent request", 0},
      {"use-congestion-manager",0, POPT_ARG_NONE, &cManager ,   0, "use a CongestionManager", 0},
      {"statistics-interval",       0,   POPT_ARG_INT,    &statisticsInterval,0, "time in seconds between statistics logging", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };

   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   int pret=poptGetNextOpt(context);
   assert(pret==-1);
   assert( poptGetArg(context)==NULL);
#endif  // popt
   Log::initialize(logType, logLevel, argv[0]);

   Data bindIfAddr(bindAddr);
   if ( bindIfAddr.size()==0 )
   {
      bindIfAddr = DnsUtil::getLocalHostName();
   }

   cout << "Performing " << runs << " runs with"
     <<" win="<<window
     <<" ip"<<(v6?"v4":"v4")
     <<" proto="<<proto
     <<" numports="<<numPorts
     <<" thread="<<threadType
     <<" bindIf="<<bindIfAddr
     <<" listen="<<doListen
     <<" tf="<<tpFlags
     <<"." << endl;

   const char *eachThreadType = threadType;
   SelectInterruptor *commonIntr = NULL;
   AsyncProcessHandler *notifyUp = NULL;
   bool noStackThread = false;
   SharedAsyncNotify sharedUp;
   if ( strcmp(eachThreadType,"none")==0 )
   {
      // Everything runs in single thread,
      // and we spin, just keep cycling thru looking for stuff to do.
      // Default seltime is zero, so no delay at all. This isn't
      // very useful for profiling because we spend all our time checking
      // stuff to do.
      noStackThread = true;
   }
   else if ( strcmp(eachThreadType,"common")==0 )
   {
      // Everything runs in single thread, but thread blocks
      // until there is something to do. When there is something
      // to do within the stack (notifyDn) or app (notifyUp) the
      // common interruptor is invoked and it breaks the select loop.
      seltime = 10*1000;
      commonIntr = new SelectInterruptor();
      notifyUp = commonIntr;
      noStackThread = true;
      eachThreadType = "none";
   }
   else
   {
      notifyUp = &sharedUp;
   }
   SipStackAndThread receiver(eachThreadType, commonIntr, notifyUp);
   SipStackAndThread sender(eachThreadType, commonIntr, notifyUp);
   receiver.getStack().setStatisticsInterval(statisticsInterval);
   sender.getStack().setStatisticsInterval(statisticsInterval);

   IpVersion version = (v6 ? V6 : V4);

   // estimate number of sockets we need:
   // 2x for sender and receiver
   // 3 for UDP (listen + select interruptor) 
   // 4 for TCP (listen + connection + select interruptor)
   // ~30 for misc (DNS, SelectInterruptors)
   int needFds = numPorts * 14 + 30;
   increaseLimitFds(needFds);

   /* On linux, the client TCP connection port range is controll by
    * /proc/sys/net/ipv4/ip_local_port_range, and defaults to [32768,61000].
    * To avoid conflicts when binding, the bound ports below should
    * stay out of the range (e.g., below 32768)
    */


   int senderPort = portBase;
   if ( senderPort==0 )
   {
      senderPort = numPorts==1 ? 25060+(rand()&0x0fff) : 11000;
   }
   int registrarPort = senderPort + numPorts;

   int idx;
   std::vector<Transport*> transports;
   for (idx=0; idx < numPorts; idx++)
   {
      transports.push_back(sender->addTransport(UDP, 
                           senderPort+idx, 
                           version, 
                           StunDisabled, 
                           bindIfAddr,
                           /*sipDomain*/Data::Empty, 
                           /*keypass*/Data::Empty, 
                           SecurityTypes::TLSv1,
                           tpFlags));

      // NOBIND doesn't make sense for UDP
      transports.push_back(sender->addTransport(TCP, 
                           senderPort+idx, 
                           version, 
                           StunDisabled, 
                           bindIfAddr,
                           /*sipDomain*/Data::Empty, 
                           /*keypass*/Data::Empty, 
                           SecurityTypes::TLSv1,
                           tpFlags|(doListen?0:RESIP_TRANSPORT_FLAG_NOBIND)));

      // NOTE: we could also bind receive to bindIfAddr, but existing code
      // doesn't do this. Responses are sent from here, so why don't we?
      transports.push_back(receiver->addTransport(UDP, 
                             registrarPort+idx, 
                             version, 
                             StunDisabled,
                             /*ipInterface*/Data::Empty,
                             /*sipDomain*/Data::Empty, 
                             /*keypass*/Data::Empty, 
                             SecurityTypes::TLSv1,
                             tpFlags));

      transports.push_back(receiver->addTransport(TCP, 
                             registrarPort+idx, 
                             version, 
                             StunDisabled,
                             /*ipInterface*/Data::Empty,
                             /*sipDomain*/Data::Empty, 
                             /*keypass*/Data::Empty, 
                             SecurityTypes::TLSv1,
                             tpFlags));
   }

   std::auto_ptr<CongestionManager> senderCongestionManager;
   std::auto_ptr<CongestionManager> receiverCongestionManager;
   if(cManager)
   {
      senderCongestionManager.reset(new GeneralCongestionManager(
                                    GeneralCongestionManager::WAIT_TIME,
                                    200));
      receiverCongestionManager.reset(new GeneralCongestionManager(
                                    GeneralCongestionManager::WAIT_TIME,
                                    200));
      sender.setCongestionManager(senderCongestionManager.get());
      receiver.setCongestionManager(receiverCongestionManager.get());
   }

   std::vector<TransportThread*> transportThreads;
   if(tpFlags & RESIP_TRANSPORT_FLAG_OWNTHREAD)
   {
      while(!transports.empty())
      {
         transportThreads.push_back(new TransportThread(*transports.back()));
         transportThreads.back()->run();
         transports.pop_back();
      }
   }

   sender.run();
   receiver.run();

   StackThreadPair pair(receiver, sender, sharedUp);
   pair.mSeltime = seltime;
   pair.mCommonIntr = commonIntr;
   pair.mNoStackThread = noStackThread;

   performTest(verbose, runs, window, invite,
      bindIfAddr, numPorts, senderPort, registrarPort, proto,
      sendSleepMs, pair);

   sender.shutdown();
   receiver.shutdown();

   sender.join();
   receiver.join();

   sender.setCongestionManager(0);
   receiver.setCongestionManager(0);

   if(tpFlags&RESIP_TRANSPORT_FLAG_OWNTHREAD)
   {
      while(!transportThreads.empty())
      {
         transportThreads.back()->shutdown();
         transportThreads.back()->join();
         delete transportThreads.back();
         transportThreads.pop_back();
      }
   }

   sender.destroy();
   receiver.destroy();

   if ( commonIntr )
   {
       delete commonIntr;
       commonIntr = NULL;
   }

#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
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
 * vi: set shiftwidth=3 expandtab:
 */
