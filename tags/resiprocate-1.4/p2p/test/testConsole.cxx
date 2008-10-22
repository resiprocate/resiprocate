#include <signal.h>
#ifdef WIN32
#include <conio.h>
#else
/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
//#include <stropts.h>
#include <sys/ioctl.h>

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Random.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/BaseException.hxx>

#include "p2p/P2PSubsystem.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/Profile.hxx"
#include "p2p/SelectTransporter.hxx"
#include "p2p/P2PStack.hxx"

using namespace p2p;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;

   if ( finished )
   {
      exit(1);
   }
   
   finished = true;
}

void processCommandLine(Data& commandline)
{
   Data command;
#define MAX_ARGS 5
   Data arg[MAX_ARGS];
   ParseBuffer pb(commandline);
   pb.skipWhitespace();
   if(pb.eof()) return;
   const char *start = pb.position();
   pb.skipToOneOf(ParseBuffer::Whitespace);
   pb.data(command, start);

   // Get arguments (up to MAX_ARGS)
   int currentArg = 0;
   while(!pb.eof() && currentArg < MAX_ARGS)
   {
      pb.skipWhitespace();
      if(!pb.eof())
      {
         const char *start = pb.position();
         pb.skipToOneOf(ParseBuffer::Whitespace);
         pb.data(arg[currentArg++], start);
      }
   }

   // Process commands
   if(isEqualNoCase(command, "quit") || isEqualNoCase(command, "q") || isEqualNoCase(command, "exit"))
   {
      finished=true;
      return;
   }   
   if(isEqualNoCase(command, "store") || isEqualNoCase(command, "st"))
   {
      // TODO - resource name and value
      return;
   }
   if(isEqualNoCase(command, "fetch") || isEqualNoCase(command, "ft"))
   {
      // TODO - resource name
      return;
   }

   InfoLog( << "Possible commands are: " << endl
         << "  <store|st> <...>" << endl
         << "  <fetch|ft> <...>" << endl
         << "  <'exit'|'quit'|'q'>");
}

#define KBD_BUFFER_SIZE 256
void processKeyboard(char input)
{
   static char buffer[KBD_BUFFER_SIZE];
   static int bufferpos = 0;

   if(input == 13 || input == 10)  // enter
   {
      Data db(buffer,bufferpos);
#ifdef WIN32
      cout << endl;
#endif
      processCommandLine(db);
      bufferpos = 0;
   }
   else if(input == 8 || input == 127) // backspace
   {
      if(bufferpos > 0)
      {
#ifdef WIN32
         cout << input << ' ' << input;
#else
         // note:  This is bit of a hack and may not be portable to all linux terminal types
         cout << "\b\b\b   \b\b\b";
         fflush(stdout);
#endif
         bufferpos--;
      }
   }
   else
   {
      if(bufferpos == KBD_BUFFER_SIZE) 
      {
         cout << endl;
         bufferpos = 0;
      }
      else
      {
#ifdef WIN32
         cout << input;
#endif
         buffer[bufferpos++] = (char)input;
      }
   }
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

   // Defaults
   Data address = DnsUtil::getLocalIpAddress();
   Data bootstrapAddress;
   unsigned short bootstrapPort=0;
   unsigned short listenPort=9000;

   //Data logLevel("INFO");
   Data logLevel("DEBUG");

   //////////////////////////////////////////////////////////////////////////////
   // Setup Config Object
   //////////////////////////////////////////////////////////////////////////////
   Profile profile;
   profile.overlayName() = "p2poverlay.com"; 
   profile.isBootstrap() = false;

   // Loop through command line arguments and process them
   for(int i = 1; i < argc; i++)
   {
      Data commandName(argv[i]);

      // Process all commandNames that don't take values
      if(isEqualNoCase(commandName, "-?") || 
         isEqualNoCase(commandName, "--?") ||
         isEqualNoCase(commandName, "--help") ||
         isEqualNoCase(commandName, "/?"))
      {
         cout << "Command line options are:" << endl;
         cout << " -p <listenPort>" << endl;
         cout << " -bs <booststrap node hostname/address>:<port>" << endl;
         cout << " -l <NONE|CRIT|ERR|WARNING|INFO|DEBUG|STACK> - logging level" << endl;
         cout << endl;
         cout << "Sample Command line:" << endl;
         cout << "testConsole -bs 192.168.1.100:9000" << endl;
         return 0;
      }
      else if (isEqualNoCase(commandName,"--bootstrap") || isEqualNoCase(commandName,"-B"))
      {
         profile.isBootstrap() = true;
      }
      else
      {
         // Process commands that have values
         Data commandValue(i+1 < argc ? argv[i+1] : Data::Empty);
         if(commandValue.empty() || commandValue.at(0) == '-')
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
         i++;  // increment argument

         if(isEqualNoCase(commandName, "-p"))
         {
            listenPort = commandValue.convertInt();
         }
         else if(isEqualNoCase(commandName, "-bs"))
         {
            // Read server and port
            Data serverAndPort = commandValue;
            ParseBuffer pb(serverAndPort);
            pb.skipWhitespace();
            const char *start = pb.position();
            pb.skipToOneOf(ParseBuffer::Whitespace, ":");  // white space or ":" 
            Data hostname;
            pb.data(hostname, start);
            bootstrapAddress = hostname;
            if(!pb.eof())
            {
               pb.skipChar(':');
               start = pb.position();
               pb.skipToOneOf(ParseBuffer::Whitespace);  // white space 
               Data port;
               pb.data(port, start);
               bootstrapPort = (unsigned short)port.convertUnsignedLong();
            }
         }
         else if(isEqualNoCase(commandName, "-l"))
         {
            logLevel = commandValue;
         }
         else
         {
            cerr << "Invalid command line parameters!" << endl;
            exit(-1);
         }
      }
   }

   Log::initialize("Cout", logLevel, "testConsole");

   initNetwork();

   InfoLog( << "testConsole settings:");
   InfoLog( << "  Listen Port = " << listenPort);
   InfoLog( << "  Bootstrap server = " << bootstrapAddress << ":" << bootstrapPort);
   InfoLog( << "  Log Level = " << logLevel);
   
   InfoLog( << "type help or '?' for list of accepted commands." << endl);


   ResourceId rid(Random::getRandom(16));
   profile.nodeId() = NodeId(rid);
   cerr << "Using NodeId: " << profile.nodeId() << endl;
   
   profile.userName().value() = "test";
   
   if(bootstrapPort != 0)
   {
      struct in_addr addr;
      if(resip::DnsUtil::inet_pton(bootstrapAddress, addr)==0)
      {
         cerr << "Invalid bootstrap address:" << bootstrapAddress << endl;
         exit(-1);
      }
      sockaddr_in addr_in;
      addr_in.sin_family = AF_INET;
      addr_in.sin_addr = addr;
      addr_in.sin_port = htons(bootstrapPort);      
      profile.bootstrapNodes().push_back(resip::GenericIPAddress(addr_in));    
   }
   profile.numInitialFingers() = 0;   // FIXME - debugging only

   //////////////////////////////////////////////////////////////////////////////
   // Setup P2PStack
   //////////////////////////////////////////////////////////////////////////////
   P2PStack p2pStack(profile);

   p2pStack.listenOn(listenPort);
   if(bootstrapPort != 0)
   {
      p2pStack.join();
   }
   
   int input;
   while(true)
   {
      p2pStack.process(10);
      
      while(_kbhit() != 0)
      {
#ifdef WIN32
         input = _getch();
         processKeyboard(input);
#else
         input = fgetc(stdin);
         fflush(stdin);
         //cout << "input: " << input << endl;
         processKeyboard(input);
#endif
      }
      if(finished) break;
   }

   InfoLog(<< "testConsole is shutdown.");
   sleepSeconds(2);
}


/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
