#include <signal.h>
#ifdef WIN32
#include <conio.h>
#else
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#define LIBEDIT
#if defined(LIBEDIT)

extern "C" {
#include <histedit.h>
#include <readline/readline.h>
}

#endif
#include <sys/ioctl.h>

// I'm thinking this can all just be readline/libedit compliant.

extern "C" char **completion_matches(const char*, char* func(const char*, int));
extern "C" void rl_set_help(int func(int, int));
extern "C" void rl_insertstr(char*);
extern "C" int add_history(const char*);

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


int
bind_help_key()
{
    // rl_bind_key('?', helpme);
    return 0;
}

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

class TtyInterface
{
public:
      TtyInterface(char * name);
    virtual ~TtyInterface();
    void go();
      static int helpme(int,int){/*unimplemented*/return 0;};
      static void sigCatch(int) {  cerr << "Exiting..."; if (mFinished) exit(-1); mFinished=true;}
//      static void sigCatchAlrm(int) { }
      
private:
      static bool mFinished;
};

bool TtyInterface::mFinished = false;

TtyInterface::TtyInterface(char * name)
{
    rl_readline_name = name;
    rl_set_help(TtyInterface::helpme);
}

void
TtyInterface::go()
{
    // setup signals
    //
#define TS(sig,disposition) { #sig, sig, disposition}
   struct
   {
         char *name;
         int signum;
         sig_t f;
   } sigs[] = {
      TS(SIGTSTP,sigCatch),
      TS(SIGINT,sigCatch),
#if !defined(WIN32)
      TS(SIGPIPE,SIG_IGN),
#endif
      TS(SIGQUIT,sigCatch),
      TS(SIGTERM,sigCatch)
};
#undef TS        
                                               
    for (unsigned int i = 0 ; i < sizeof(sigs)/sizeof(*sigs); i++)
    {
       if( signal( sigs[i].signum, sigs[i].f ) == SIG_ERR )
       {
          cerr << sigs[i].name << ": error setting singal disposition, errno=" << errno << endl;
          // This isn't fatal .. let it ride.
       }
    }



}
int 
main (int argc, char** argv)
{
   TtyInterface tty(*argv);

   // Defaults
   Data address = DnsUtil::getLocalIpAddress();
   Data bootstrapAddress;
   unsigned short bootstrapPort=0;
   unsigned short listenPort=9000;

   Data logLevel("DEBUG");

   // Command line args
   // --help
   // -p listenPort
   // -bs 
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

   //////////////////////////////////////////////////////////////////////////////
   // Setup Config Object
   //////////////////////////////////////////////////////////////////////////////
   Profile profile;
   profile.overlayName() = "p2poverlay.com"; 
   
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
      
      while(3 /* doing things in readline() */)
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
