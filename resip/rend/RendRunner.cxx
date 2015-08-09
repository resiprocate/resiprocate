/**
  * REND Runner
  *
  * Object which owns the stack, Tu, engine, waves, etc. and
  * joins them all together.
  *
  * Written by Kennard White (Logitech, Inc.) over 2010..2011
**/

#include "popt.h"

#include "rutil/Logger.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/ssl/Security.hxx"
// #include "rutil/dns/DnsStub.hxx"

#include "RendAcct.hxx"
#include "RendWavePres.hxx"
#include "RendSketch.hxx"
#include "RendRunner.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

class RendSketchThread : public resip::ThreadIf 
{
public:
   RendSketchThread(RendSketchBase& sketch) : mSketch(sketch) { };
   virtual void shutdown() { mSketch.signalStop(200); };

protected:
   virtual void thread();

   // we ref it, but don't own it
   RendSketchBase& mSketch;
};

void
RendSketchThread::thread() 
{
   InfoLog(<<"Starting sketch worker thread.");
   RendTimeUs rptPeriod = 0;
   mSketch.doWorkLoop(rptPeriod);
   InfoLog(<<"Finishing sketch worker thread.");
}

extern struct poptOption TheRunnerOptTbl[];

struct RendRunnerOpts : public RendOptsBase 
{
   RendRunnerOpts() 
   {
      mLogTypeStr = "file";
      mLogLevelStr = "WARNING";
      mStackLogLevelStr = "WARNING";
      mLogFileStr = "rend.log";
      mNumFds = 0;
      mAnyAddrPort = 0;
#ifdef WIN32
      mStackEventImpl = "event";
#else
      mStackEventImpl = "epoll";
#endif
      mRootCertFn = 0;
      mUseSketchThread = 0;
      mSketchTickPeriodUs = 100*1000;
   };
   const char* mLogTypeStr;
   const char* mLogLevelStr;
   const char* mStackLogLevelStr;
   const char* mLogFileStr;
   int mNumFds;
   int mAnyAddrPort;
   const char* mStackEventImpl;
   const char* mRootCertFn;
   int mUseSketchThread;
   int mSketchTickPeriodUs;

   virtual poptOption* getPoptTbl() { return TheRunnerOptTbl; }
   virtual const char* getPoptDesc() { return "Runner options"; }
};

static RendRunnerOpts TheRendRunnerOpts;
#define OPTOBJ TheRendRunnerOpts

struct poptOption TheRunnerOptTbl[] = 
{
   { "log-type", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mLogTypeStr, 0, "Where to send log message", "cout|cerr|file|syslog"},
   { "log-level", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mLogLevelStr, 0, "Log level", "DEBUG|INFO|NOTICE|WARNING|ERR|ALERT"},
   { "stacklog-level", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mStackLogLevelStr, 0, "SIP Stack Log level", "DEBUG|INFO|NOTICE|WARNING|ERR|ALERT"},
   { "log-file", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mLogFileStr, 0, "Name of file for log-type=file", 0},
   { "numfds", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mNumFds, 0, "Number of fds to ask for (setrlimit)" },
   { "anyaddrport", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mAnyAddrPort, 0, "Create transports on this port (0.0.0.0)" },
   { "sketchthread", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mUseSketchThread, 0, "Run sketch thread in own process" },
   { "sketchtick", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mSketchTickPeriodUs, 0, "Polling interval (us) for sketch" },
   { "tlsrootcert", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mRootCertFn, 0, "Filename of root certificate for TLS", 0},
   { "stackevent", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mStackEventImpl, 0, "Type of event loop for SipStack thread" },
   { NULL, 0, 0, NULL, 0 }
};

RendRunnerIf::~RendRunnerIf() 
{
}

class RendRunner : public RendRunnerIf 
{
public:
   RendRunner();
   ~RendRunner();

   virtual void setSketch(const char *sketchName);
   virtual void setArgs(const char *appName, int argc, char **argv);
   virtual void start();
   virtual void signalStop(int urgency);
   virtual void stop();
   virtual bool waitForFinish(int ms);
   virtual int getTroopReport(RendTroopReport& rpt);
   virtual int setWorkVolume(RendWorkVolume& vol);
   virtual int doSimpleAction(RendSimpleAction act);

   //void makeSketch(const char *sketchName);
   RendSketchFactory* findFactory(const char *sketchName, const char *dieMsg);

protected:
   //poptContext mPoptCxt;
   resip::EventStackSimpleMgr* mSimpleMgr;
   RendCntMgr mCntMgr;
   RendAcctMgrSimple *mAcctMgr;
   RendTu *mRendTu;

   RendSketchBase* mSketch;
   RendSketchFactory* mSketchFac;
   RendSketchThread* mSketchThread;

   bool mLogStarted;
};

// static int RendFinished = 0;

RendRunner::RendRunner() 
{
   mSimpleMgr = NULL;
   mAcctMgr = NULL;
   mRendTu = NULL;
   mSketch = NULL;
   mSketchFac = NULL;
   mSketchThread = NULL;
   mLogStarted = false;
}

RendRunner::~RendRunner() 
{
   InfoLog(<< "Runner destructing");
   stop();
}

void
RendRunner::setSketch(const char *sketchName) 
{
   resip_assert( mSketch==NULL );
   if ( sketchName==NULL )
      return;
   mSketchFac = findFactory(sketchName, "cannot set sketch");
}

void
RendRunner::setArgs(const char *appName, int argc, char **argv) 
{
   RendRunnerOpts *	runnerOpts = &TheRendRunnerOpts;
   RendAcctOptsSimple*	acctOpts = &TheRendAcctOptsSimple;
   RendOptsBase*	tuOpts = TheRendTuOptsPtr;
   resip_assert(mSketchFac);
   RendOptsBase*	sketchOpts = mSketchFac->getOpts();

   struct poptOption appOptTbl[] = 
   {
      { 0, 0, POPT_ARG_INCLUDE_TABLE, runnerOpts->getPoptTbl(), 0, runnerOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, acctOpts->getPoptTbl(), 0, acctOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, tuOpts->getPoptTbl(), 0, tuOpts->getPoptDesc() },
#if 0
      { 0, 0, POPT_ARG_INCLUDE_TABLE, waveOpts->getPoptTbl(), 0, waveOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, testOpts->getPoptTbl(), 0, testOpts->getPoptDesc() },
#endif
      { 0, 0, POPT_ARG_INCLUDE_TABLE, sketchOpts->getPoptTbl(), 0, sketchOpts->getPoptDesc() },
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };

   int ret;
   const char **cargv = (const char**)argv;
   poptContext pcon = poptGetContext(appName, argc, cargv, appOptTbl, 
      POPT_CONTEXT_KEEP_FIRST);
   while ( (ret = poptGetNextOpt(pcon)) != -1 ) 
   {
      if ( ret < 0 ) 
      {
         std::cerr << "rend option error: "<<poptBadOption(pcon, 0) 
            <<": "<<poptStrerror(ret) << std::endl;
         exit(1);
      }
      std::cerr << "rend got unexpected option code="<<ret<<std::endl;
      exit(1);
   }
   // mPoptCxt = pcon;
   poptFreeContext(pcon);	// is it safe to do this here?
}

int
RendRunner::doSimpleAction(RendSimpleAction act) 
{
   switch ( act ) 
   {
   default:
      if ( mSketch ) 
      {
         return mSketch->doSimpleAction(act);
      }
      return -1;
   }
   return -1;
}

void
RendRunner::start()
{
   resip_assert( mSketchFac );
   resip_assert( mSketch == NULL );
   if ( ! mLogStarted )
   {
      // only start log the first time!
      resip::Log::initialize(TheRendRunnerOpts.mLogTypeStr, TheRendRunnerOpts.mLogLevelStr, "rend", TheRendRunnerOpts.mLogFileStr);

      // Set the log level of the stack subsystems
      resip::Log::setLevel(resip::Log::toLevel(TheRendRunnerOpts.mStackLogLevelStr), resip::Subsystem::TRANSPORT);
      resip::Log::setLevel(resip::Log::toLevel(TheRendRunnerOpts.mStackLogLevelStr), resip::Subsystem::TRANSACTION);
      resip::Log::setLevel(resip::Log::toLevel(TheRendRunnerOpts.mStackLogLevelStr), resip::Subsystem::SIP);
      resip::Log::setLevel(resip::Log::toLevel(TheRendRunnerOpts.mStackLogLevelStr), resip::Subsystem::DNS);

      mLogStarted = true;
   }
   InfoLog(<< "Runner starting...");

   if ( TheRendRunnerOpts.mNumFds > 0 ) 
   {
      if ( resip::increaseLimitFds(TheRendRunnerOpts.mNumFds) <= 0 ) 
      {
         fprintf(stderr, "\nFailed to increase number of fds. Exiting.\n");
         exit(1);
      }
   }
   mCntMgr.reset();
   mAcctMgr = TheRendAcctOptsSimple.createAcctMgr();

   mSimpleMgr = new resip::EventStackSimpleMgr(TheRendRunnerOpts.mStackEventImpl);
   resip::Security *secObj = new resip::Security;
   secObj->setAllowWildcardCertificates(true);
   const char* cfn = TheRendRunnerOpts.mRootCertFn;
   if ( cfn && cfn[0] ) 
   {
      resip::Data certval;
      if ( RendReadFileIntoData(cfn, certval) < 0 ) 
      {
         fprintf(stderr, "\nFailed to read cert file. Exiting.\n");
         exit(1);
      }
      secObj->addRootCertPEM(certval);
   }
   resip::SipStackOptions stackOptions;
   stackOptions.mSecurity = secObj;
   resip::SipStack& myStack = mSimpleMgr->createStack(stackOptions);

   mRendTu = RendTuCreate(myStack, *mAcctMgr);

   if ( TheRendRunnerOpts.mAnyAddrPort > 0 ) 
   {
      // create default transports after
      myStack.addTransport(resip::UDP, TheRendRunnerOpts.mAnyAddrPort);
      myStack.addTransport(resip::TCP, TheRendRunnerOpts.mAnyAddrPort);
      myStack.addTransport(resip::TLS, TheRendRunnerOpts.mAnyAddrPort+1);
   }

   mSketch = mSketchFac->makeSketch(*mRendTu, mCntMgr);
   if ( TheRendRunnerOpts.mSketchTickPeriodUs > 0 ) 
   {
      mSketch->setTickPeriod(TheRendRunnerOpts.mSketchTickPeriodUs);
   }

   mSimpleMgr->getThread().run();

   if ( TheRendRunnerOpts.mUseSketchThread  ) 
   {
      mSketchThread = new RendSketchThread(*mSketch);
      InfoLog(<<"Running sketch worker thread");
      mSketchThread->run();
   } 
   else 
   {
      InfoLog(<<"No sketch worker thread (using main process)");
      RendTimeUs rptPeriod = REND_S2US(1);
      mSketch->doWorkLoop(rptPeriod);
   }
}

void
RendRunner::signalStop(int urgency) 
{
   if ( mSketch==NULL ) 
   {
      return;
   }
   mSketch->signalStop(urgency);
}

bool
RendRunner::waitForFinish(int ms) 
{
   if ( mSketch==NULL )
      return true;
   while ( ! mSketch->isFinished() ) 
   {
      if ( ms <= 0 )
         return false;
      int sleepms = rendMIN(ms,500);
      sleepMs(sleepms);
      ms -= sleepms;
   }
   return true;
}

void
RendRunner::stop()
{
   InfoLog(<< "Runner stopping...");

   signalStop(200);
   // usleep(500*1000);	// let any last responses go out

   if ( mSketchThread ) 
   {
      mSketchThread->shutdown();
   }
#if 0
   if ( mStackThread ) 
   {
      mStackThread->shutdown();
   }
#else
   if ( mSimpleMgr ) 
   {
      mSimpleMgr->getThread().shutdown();
   }
#endif

   if ( mSketchThread ) 
   {
      mSketchThread->join();
      InfoLog(<< "sketch shutdown and joined");
      delete mSketchThread; mSketchThread = NULL;
   }
#if 0
   if ( mStackThread ) 
   {
      mStackThread->join();
      InfoLog(<< "stack shutdown and joined");
      delete mStackThread; mStackThread = NULL;
   }
#else
   if ( mSimpleMgr ) 
   {
      mSimpleMgr->getThread().join();
      InfoLog(<< "stack shutdown and joined");
      delete mSimpleMgr; mSimpleMgr = NULL;
   }
#endif
   if ( mSketch ) 
   {
      delete mSketch; mSketch = NULL;
   }
   if ( mRendTu ) 
   {
      RendTuDestroy(mRendTu); mRendTu = NULL;
   }
   if ( mAcctMgr ) 
   {
      delete mAcctMgr; mAcctMgr = NULL;
   }
#if 0
   if ( mStack ) 
   {
      delete mStack; mStack = NULL;
   }
   if ( mIntr ) 
   {
      delete mIntr; mIntr = NULL;
   }
   if ( mPollGrp ) 
   {
      delete mPollGrp; mPollGrp = NULL;
   }
#endif
}

int
RendRunner::getTroopReport(RendTroopReport& rpt) 
{
   if ( mSketch==NULL ) 
   {
      return -1;
   }
   int sts = mSketch->getTroopReport(rpt);
   resip_assert( rpt.mWavePubAliveCnt < 10000 );
   return sts;
}

int
RendRunner::setWorkVolume(RendWorkVolume& vol) 
{
   if ( mSketch==NULL ) 
   {
      return -1;
   }
   InfoLog(<<"setwork max="<<vol.mWorkRateMax);
   mSketch->setWorkVolume(vol);
   return 1;
}

RendSketchFactory**
TheRendFactoryList[] = 
{
   &TheRendReg1SketchFacPtr,
   &TheRendPres1SketchFacPtr,
   NULL
};

RendSketchFactory*
RendRunner::findFactory(const char *sketchName, const char *dieMsg) 
{
   int idx;
   for (idx=0; ; idx++) 
   {
      RendSketchFactory** facPtr = TheRendFactoryList[idx];
      if ( facPtr==NULL )
         break;
      RendSketchFactory* fac = *facPtr;
      if ( fac==NULL )
         continue;
      if ( strcasecmp(fac->getName(),sketchName)==0 )
         return fac;
   }
   if ( dieMsg ) 
   {
      fprintf(stderr, "%s: no known sketch \"%s\".\n", dieMsg, sketchName);
      ::exit(1);
   }
   return NULL;
}



#if 0
poptContext
RendParseOpts(const char *appName, int argc, char *argv[],
              RendOptsBase *testOpts) 
{
   RendRunnerOpts  *runnerOpts = &TheRendRunnerOpts;
   RendAcctOptsSimple *acctOpts = &TheRendAcctOptsSimple;
   // RendWavePresOpts *waveOpts = &TheRendWavePresOpts;

   struct poptOption appOptTbl[] = {
      { 0, 0, POPT_ARG_INCLUDE_TABLE, runnerOpts->getPoptTbl(), 0, runnerOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, acctOpts->getPoptTbl(), 0, acctOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, waveOpts->getPoptTbl(), 0, waveOpts->getPoptDesc() },
      { 0, 0, POPT_ARG_INCLUDE_TABLE, testOpts->getPoptTbl(), 0, testOpts->getPoptDesc() },
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };

   int ret;
   const char **cargv = (const char**)argv;
   poptContext pcon = poptGetContext(appName, argc, cargv, appOptTbl, 0);
   while ( (ret = poptGetNextOpt(pcon)) != -1 ) 
   {
      if ( ret < 0 ) 
      {
         std::cerr << "rend option error: "<<poptBadOption(pcon, 0) 
            <<": "<<poptStrerror(ret) << std::endl;
         exit(1);
      }
      resip_assert( false );
   }
   return pcon;
}
#endif

RendRunnerIf*
RendRunnerCreate() 
{
   return new RendRunner();
}

/* ====================================================================

 Copyright (c) 2011, Logitech, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Logitech nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
