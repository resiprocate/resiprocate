
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/os/Timer.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


class LogThread : public ThreadIf
{
    public:
      LogThread(const Data& description, const Log::ThreadSetting& s)
         : mDescription(description),
           mSetting(s)
      {}

      void thread()
      {
         Log::setThreadSetting(mSetting);
         while(!waitForShutdown(100))
         {
            DebugLog(<< mDescription << "  DEBUG");
            InfoLog(<< mDescription << "  INFO");
         }
      }
   private:
      Data mDescription;
      Log::ThreadSetting mSetting;
};

int logsInCall()
{
   InfoLog(<< "Got here?");
   return 17;
}


int debugLogsInCall()
{
   DebugLog(<< "Got here?");
   return 17;
}

int
main(int argc, char* argv[])
{
   Log::initialize(Log::SYSLOG, Log::INFO, argv[0]);
   //Log::initialize(Log::COUT, Log::INFO, argv[0]);

   DebugLog(<<"This should not appear.");
   InfoLog(<<"This should appear.");

   LogThread service1a("service1----A", Log::ThreadSetting(1, Log::DEBUG));
   LogThread service1b("service1-------B", Log::ThreadSetting(1, Log::DEBUG));
   LogThread service1c("service1---------C", Log::ThreadSetting(1, Log::DEBUG));

   LogThread service2a("service2-----------A", Log::ThreadSetting(2, Log::DEBUG));
   LogThread service2b("service2------------------B", Log::ThreadSetting(2, Log::ERR));

   service1a.run();
   service1b.run();
   service1c.run();
   service2a.run();
   service2b.run();

   sleep(2);

   InfoLog(<<"Setting service 1 to INFO\n");
   Log::setServiceLevel(1, Log::INFO);
   sleep(2);

   InfoLog(<<"Setting service 1 to CRIT\n");
   Log::setServiceLevel(1, Log::CRIT);
   sleep(2);

   InfoLog(<<"Setting service 2 to DEBUG_STACK\n");
   Log::setServiceLevel(2, Log::DEBUG_STACK);
   sleep(2);

   InfoLog(<<"Setting service 1 to DEBUG\n");
   Log::setServiceLevel(1, Log::DEBUG);
   sleep(2);

   DebugLog(<<"This should still not appear.");
   InfoLog(<<"This should still appear.");

   service1a.shutdown();
   service1b.shutdown();
   service1c.shutdown();
   service2a.shutdown();
   service2b.shutdown();

   service1a.join();
   service1b.join();
   service1c.join();
   service2a.join();
   service2b.join();

   Log::setLevel(Log::DEBUG);

   if (false)
   {
      UInt64 start = Timer::getTimeMs();
      for (int i = 0; i < 10000; i++)
      {
         InfoLog(<< "string");
      }
      cerr << "Info Took: " << Timer::getTimeMs() - start << endl;
   }

   if (false)
   {
      UInt64 start = Timer::getTimeMs();
      for (int i = 0; i < 10000; i++)
      {
         DebugLog(<< "string");
      }
      cerr << "Debug Took: " << Timer::getTimeMs() - start << endl;
   }

   InfoLog(<< "Recursive debug: " << debugLogsInCall());
   DebugLog(<< "Recursive non-debug OK: " << logsInCall());

#ifdef NO_DEBUG
   cerr << "All OK -- will deadlock next" << endl;
#else
   cerr << "All OK -- will assert next (linux)" << endl;
#endif

   // this will assert except on WIN32 where it will deadlock
   InfoLog(<< "Recursive non-debug NOT OK: " << logsInCall());
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
 */
