#include <signal.h>
#include <iostream>
#include "rutil/ResipAssert.h"

#include "JabberComponent.hxx"

using namespace gateway;
using namespace std;

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

JabberComponent* g_component;

static void
signalHandler(int signo)
{
   //std::cerr << "Shutting down" << endl;
   g_component->stop();
}

int 
main (int argc, char** argv)
{
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

   if(argc != 11 || std::string(argv[0]) != std::string("ichat-gw"))
   {
      cerr << "argc=" << argc << ", argv[0]=" << argv[0] << endl;
      cerr << "Jabber connector process must not be launched manually, it is launched automatically from the main ichat-gw program." << endl;
      exit(-1);
   }

#if defined(WIN32)
   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 ) 
   {
      // could not find a usable WinSock DLL
      cerr << "Could not load winsock" << endl;
      resip_assert(0); 
      exit(1);
   }    
#endif

   unsigned int jabberConnectorIPCPort = atoi(argv[1]);
   unsigned int gatewayIPCPort = atoi(argv[2]);

   g_component = new JabberComponent(jabberConnectorIPCPort,
                                     gatewayIPCPort,
                                     argv[3],   // Jabber server
                                     argv[4],   // Jabber component name
                                     argv[5],   // Jabber component password
                                     atoi(argv[6]),   // Jabber component port 
                                     atoi(argv[7]),   // Jabber server ping duration 
                                     argv[8],   // Jabber control username 
                                     argv[9]);  // IPPort Data blob


   g_component->run();

   g_component->join();
   delete g_component;

   cout << "ichat-gw-jc is shutdown." << endl;
   sleepSeconds(2);
}


/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

