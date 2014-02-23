#include <signal.h>

// Need to include this early to avoid problems with __STDC_FORMAT_MACROS
#include "rutil/compat.hxx"

#include "repro/PersistentMessageQueue.hxx"
#include <rutil/Time.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace resip;
using namespace std;
using namespace repro;

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

int 
main (int argc, char** argv)
{
   // Install signal handlers
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

   // Log any resip logs to cerr, since session events are logged to cout
   Log::initialize(Log::Cerr, Log::Info, "");

   Data msgQueueName("sessioneventqueue");
   if(argc >= 2)
   {
      msgQueueName = argv[1];
   }
   PersistentMessageDequeue* queue = new PersistentMessageDequeue("");
   if(queue->init(true, msgQueueName))
   {
      vector<resip::Data> recs;
      while(!finished)
      {
         if(queue->pop(5, recs, true))
         {
            if(recs.size() > 0)
            {
               for(size_t i = 0; i < recs.size(); i++)
               {
                  cout << recs[i] << endl;
               }
            }
            else
            {
               resip::sleepSeconds(1);
            }
         }
         else 
         {
            if(queue->isRecoveryNeeded())
            {
               delete queue;
               queue = new PersistentMessageDequeue("");
               if(!queue->init(true, msgQueueName))
               {
                  cerr << "Error initializing message queue after error!" << endl;
                  break;
               }
            }
            else
            {
               cerr << "Error dequeuing!" << endl;
               break;
            }
         }
      }
   }
   else
   {
      cerr << "Error initializing message queue!" << endl;
   }
   delete queue;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2012
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
/*
 * vi: set shiftwidth=3 expandtab:
 */
