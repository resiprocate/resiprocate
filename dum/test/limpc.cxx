#if defined(HAVE_CONFIG_HXX)
#include "resiprocate/config.hxx"
#endif

#include <cstring>
#include <cassert>

//#define USE_CURSES

#if defined (HAVE_POPT_H) 
#include <popt.h>
#endif

#ifdef USE_CURSES
#include <ncurses.h>
#else
#include <iostream>
#include <cstdio>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

typedef void WINDOW;

#ifndef __APPLE__
bool TRUE=true;
bool FALSE=false;
#endif

char ACS_HLINE=1;
char ACS_VLINE=2;
WINDOW* stdscr=0;
WINDOW* newwin(...) { return NULL; };
void waddstr(WINDOW*, const char* text) { std::clog << text; };
char getch()
{
   char buf[1];
   int r = read(fileno(stdin),&buf,1);
   if ( r ==1 )
   {
      return buf[0];
   }
   return 0;
};

void werase(WINDOW*) {};
void wrefresh(...) {};
void mvhline(...) {};
void refresh(...) {};
void getmaxyx(...) {};
void clearok(...) {};
void waddch(...) {};
void initscr(...) {};
void cbreak(...) {};
void noecho(...) {};
void nonl(...) {};
void intrflush(...) {};
void keypad(...) {};
void scrollok(...) {};
void wmove(...) {};
void mvvline(...) {};
#endif



#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"

static int myMain(int argc, char* argv[]);

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

static WINDOW* commandWin=0;
static WINDOW* textWin=0;
static WINDOW* statusWin=0;

static TuIM* tuIM;
static Uri   dest;

void 
displayPres()
{
   werase(statusWin);

   for( int i=0; i<tuIM->getNumBuddies();i++)
   {
      Uri uri = tuIM->getBuddyUri(i);
      Data status;
      bool online = tuIM->getBuddyStatus(i,&status);
      const char* stat = (online)?"online":"offline";
         
      waddstr(statusWin,uri.getAor().c_str());
      waddstr(statusWin," ");
      waddstr(statusWin,stat);
      waddstr(statusWin," ");
      waddstr(statusWin,status.c_str());
      waddstr(statusWin,"\n");
   }

   wrefresh(statusWin);  
}


class TestCallback: public TuIM::Callback
{
   public:
      virtual void presenceUpdate(const Uri& dest, bool open, const Data& status );
      virtual void receivedPage( const Data& msg, const Uri& from ,
                                 const Data& signedBy,  SignatureStatus sigStatus,
                                 bool wasEncryped  );
      virtual void sendPageFailed( const Uri& dest,int respNumber );
      virtual void registrationFailed(const resip::Uri&, int respNumber); 
      virtual void registrationWorked(const Uri& dest );
      virtual void receivePageFailed(const Uri& sender);
};
  

void 
TestCallback::presenceUpdate(const Uri& from, bool open, const Data& status )
{
   const char* stat = (open)?"online":"offline";
   //cout << from << " set presence to " << stat << " " << status.c_str() << endl;

   waddstr(textWin,"Status: ");
   waddstr(textWin, from.getAor().c_str());
   waddstr(textWin," is ");
   waddstr(textWin,stat);
   waddstr(textWin," ");
   waddstr(textWin,status.c_str());
   waddstr(textWin,"\n");

   wrefresh(textWin);

   displayPres();
}

void 
TestCallback::receivedPage( const Data& msg, const Uri& from,
                            const Data& signedBy,  SignatureStatus sigStatus,
                            bool wasEncryped  )
{  
   //DebugLog(<< "In TestPageCallback");

   if ( dest != from )
   {
      dest = from;
      //cerr << "Set destination to <" << *mDest << ">" << endl;
      waddstr(textWin,"Set destination to ");
      waddstr(textWin, Data::from(dest).c_str());
      waddstr(textWin,"\n");
   }
   
   //cout << from;  

   waddstr(textWin,"From: ");
   waddstr(textWin,from.getAor().c_str());

   if ( !wasEncryped )
   {
      //cout << " -NOT SECURE- ";
      waddstr(textWin," -NOT SECURE-");
   }
   else
   {
      waddstr(textWin," -secure-");
   }
   switch ( sigStatus )
   {
      case  SignatureIsBad:
         //cout << " -bad signature- ";
         waddstr(textWin,"bad signature");
	 break;
      case  SignatureNone:
         //cout << " -no signature- ";
         waddstr(textWin,"no signature");
         break;
      case  SignatureTrusted:
         //cout << " <signed  " << signedBy << " > ";
         waddstr(textWin,"signed ");
         waddstr(textWin,signedBy.c_str());
         break;
      case  SignatureCATrusted:
         //cout << " <ca signed  " << signedBy << " > ";
         waddstr(textWin,"ca signed " );
         waddstr(textWin,signedBy.c_str());
         break;
      case  SignatureNotTrusted:
         //cout << " <signed  " << signedBy << " NOT TRUSTED > ";
         waddstr(textWin,"untrusted signature ");
         waddstr(textWin,signedBy.c_str());
         break;
   }
   
   //cout << " says:" << endl;
   //cout << msg.escaped() << endl;  
   waddstr(textWin, " says: ");
   waddstr(textWin, msg.escaped().c_str() );
   waddstr(textWin, "\n");
   
   wrefresh(textWin);
}


void 
TestCallback::sendPageFailed( const Uri& target, int respNum )
{
   //InfoLog(<< "In TestErrCallback");  
   // cerr << "Message to " << dest << " failed" << endl;  
   Data num(respNum);
   
   waddstr(textWin,"Message to ");
   waddstr(textWin, Data::from(target).c_str());
   waddstr(textWin," failed (");
   waddstr(textWin,num.c_str());
   waddstr(textWin," response)\n");
   wrefresh(textWin);
}


void 
TestCallback::receivePageFailed( const Uri& target )
{
   //InfoLog(<< "In TestErrCallback");  
   // cerr << "Message to " << dest << " failed" << endl;  

   waddstr(textWin,"Can not understand messager from ");
   waddstr(textWin, Data::from(target).c_str());
   waddstr(textWin,"\n");
   wrefresh(textWin);
}


void 
TestCallback::registrationFailed(const resip::Uri& target, int respNum )
{
   Data num(respNum);
   
   waddstr(textWin,"Registration to ");
   waddstr(textWin, Data::from(target).c_str());
   waddstr(textWin," failed (");
   waddstr(textWin,num.c_str());
   waddstr(textWin," response)\n");
   wrefresh(textWin);
}
  
                              
void 
TestCallback::registrationWorked(const resip::Uri& target)
{
   waddstr(textWin,"Registration to ");
   waddstr(textWin, Data::from(target).c_str());
   waddstr(textWin," worked");
   wrefresh(textWin);
}
  
                              
bool
processStdin( Uri* dest, bool sign, bool encryp )
{
   static unsigned int num=0;
   static char buf[1024];

   char c = getch();	
      
   if ( c == 0 )
   {
      return true;
   }
   
   if ( c == '\f' )
   {
      clearok(textWin,TRUE);
      clearok(statusWin,TRUE);
      clearok(commandWin,TRUE);

      assert( num < sizeof(buf) );
      buf[num] = 0;
      werase(commandWin);
      waddstr(commandWin,buf);

      wrefresh(textWin);
      wrefresh(statusWin);
      wrefresh(commandWin);

      return true;
   }

   if (  (c == '\a') || (c == '\b') || (c == 4 ) || (c == 0x7F) )
   {
      if ( num > 0 )
      {
         num--;
      }
      buf[num]=0;

      werase(commandWin);
      waddstr(commandWin,buf);
      wrefresh(commandWin);
      
      return true;
   }
       
   if ( (c == '\r') || (c == '\n') || (num+2>=sizeof(buf)) )
   {
      buf[num] =0;

      if ( (num>3) && (!strncmp("to:",buf,3)) )
      {
         buf[num] = 0;
         *dest = Uri(Data(buf+3));
       
         //cerr << "Set destination to <" << *dest << ">";
         waddstr(textWin,"Set destination to ");
         waddstr(textWin, Data::from(*dest).c_str());
         waddstr(textWin,"\n");
         wrefresh(textWin);
      }
      else if ( (num>4) && (!strncmp("add:",buf,4)) )
      {
         buf[num] = 0;
         Uri uri(Data(buf+4));

         //cerr << "Subscribing to buddy <" << uri << ">";
         waddstr(textWin, "Subscribing to ");
         waddstr(textWin, Data::from(uri).c_str());
         waddstr(textWin, "\n");
         wrefresh(textWin);
         
         tuIM->addBuddy( uri, Data::Empty );
         displayPres();
      }
      else if ( (num>=7) && (!strncmp("status:",buf,7)) )
      {
         buf[num] = 0;
         Data stat(buf+7);

         //cerr << "setting presence status to  <" << stat << ">";
         waddstr(textWin,"Set presece status to <");
         waddstr(textWin,stat.c_str());
         waddstr(textWin,">\n");
         wrefresh(textWin);

         tuIM->setMyPresence( !stat.empty(), stat );
      }
      else if ( (num==1) && (!strncmp(".",buf,1)) )
      {
         //DebugLog( << "Got a period - end program" );
         return false;
      }
      else
      { 
         if ( num >= 1 )
         {
            assert( num < sizeof(buf) );
            buf[num] = 0;
            Data text(buf);
            
            Data destValue  = dest->getAor();
            
            DebugLog( << "Destination is " << destValue );

            Data encFor = Data::Empty;
            if (encryp)
            { 
               encFor = dest->getAorNoPort();
            }

            DebugLog( << "Destination encrypt for is " << encFor );

            if ( tuIM->haveCerts(sign,encFor) )
            {
               waddstr(textWin,"To: ");
               waddstr(textWin,destValue.c_str());
               waddstr(textWin," ");
               waddstr(textWin,text.c_str());
               waddstr(textWin,"\n");
               wrefresh(textWin);
               
               tuIM->sendPage( text , *dest, sign , encFor );
            }
            else
            {
               waddstr(textWin,"Don't have aproperate certificates to sign and encrypt a message to ");
               waddstr(textWin,destValue.c_str());
               waddstr(textWin,"\n");
               wrefresh(textWin);
            }
         }
      }

      num = 0;  

      werase(commandWin);
      wrefresh(commandWin);
   }
   else
   {
      buf[num++] = c;
      assert( num < sizeof(buf) );
      
      waddch(commandWin,c);
      wrefresh(commandWin);
   }

   return true;
}


int
main(int argc, char* argv[])
{
   try
   {
      UserAgent ua(argc, argv);

      InfoLog(<< argv[0] << " starting");
      while(1)
      {
         ua.process();
      }
   }
   catch (BaseSecurity::Exception& e)
   {
      WarningLog (<< "Couldn't set up security object");
      exit(-1);
   }
   catch (BaseException& e)
   {
      ErrLog (<< "Caught: " << e);
      exit(-1);
   }
   catch( ... )
   {
      ErrLog( << "Caught non-resip exception" );
      exit(-1);
   }

   return 0;
}

static int
myMain(int argc, char* argv[])
{  
   DebugLog( << "Done adding the transports " );   


   InfoLog( << "aor is " << aor );
   InfoLog( << "contact is " << contact );
   TestCallback callback;
   tuIM = new TuIM(&sipStack,aor,contact,&callback);

   Data name("SIPimp.org/0.2.5 (curses)");
   tuIM->setUAName( name );
      
   if ( !outbound.host().empty() )
   {
      tuIM->setOutboundProxy( outbound );
   }

   // setup prefered outbound transport 
   if ( prefUdp )
   {
      tuIM->setDefaultProtocol( UDP );
   }
   if ( prefTcp )
   {
      tuIM->setDefaultProtocol( TCP );
   }
   if ( prefTls )
   {
      tuIM->setDefaultProtocol( TLS );
   }
   if ( prefDtls )
   {
      tuIM->setDefaultProtocol( DTLS );
   }

   if ( haveAor )
   {
      if ( !noRegister )
      {
         tuIM->registerAor( aor, aorPassword );
      }
   }
   
   initscr(); 
   cbreak(); 
   noecho();
   nonl();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);

   int rows=0;
   int cols=0;
   getmaxyx(stdscr,rows,cols);		/* get the number of rows and columns */
        
   commandWin = newwin(2,cols,rows-2,0);
   scrollok(commandWin, TRUE);
   wmove(commandWin,0,0);
     
   textWin = newwin(rows-3,cols*3/4,0,0);
   scrollok(textWin, TRUE);
   wmove(textWin,0,0);
     
   statusWin = newwin(rows-3,cols-(cols*3/4)-1,0,1+cols*3/4);
   scrollok(statusWin, FALSE);
   wmove(statusWin,0,0);

   mvhline(rows-3,0,ACS_HLINE,cols);
   mvvline(0,(cols*3/4),ACS_VLINE,rows-3);
   refresh();

   for ( int i=0; i<numAdd; i++ )
   { 
      Uri uri(addList[i]);
      tuIM->addBuddy( uri, Data::Empty );
   }

   for ( int i=0; i<numPub; i++ )
   { 
      Uri uri(pubList[i]);
      tuIM->addStateAgent( uri );
   }

   displayPres();
 
   waddstr(textWin,"Use -help on the command line to view options\n");
   waddstr(textWin,"To set where your messages will get sent type\n");
   waddstr(textWin,"    to: sip:alice@example.com \n");
   waddstr(textWin,"To monitores someeone presence type\n");
   waddstr(textWin,"    add: sip:buddy@example.com \n");
   waddstr(textWin,"To change you online status type\n");
   waddstr(textWin,"    status: in meeting\n");
   waddstr(textWin,"To set yourself to offline type\n");
   waddstr(textWin,"   status:\n");
   waddstr(textWin,"To exit type a single period\n");
   waddstr(textWin,"\n");
   wrefresh(textWin);     

   while (1)
   {
      FdSet fdset; 
      sipStack.buildFdSet(fdset);
      int time = sipStack.getTimeTillNextProcessMS();

      fdset.setRead( fileno(stdin) );
      
      //cerr << time << endl;

      int  err = fdset.selectMilliSeconds( time );
      if ( err == -1 )
      {
         int e = errno;
         switch (e)
         {
            case 0:
               break;
            default:
               //InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
               break;
         }
      }
      if ( err == 0 )
      {
         //cerr << "select timed out" << endl;
      }
      if ( err > 0 )
      {
         //cerr << "select has " << err << " fd ready" << endl;
      }
      
      ////InfoLog(<< "Select returned");
       
      if ( fdset.readyToRead( fileno(stdin) ) )
      {
         bool keepGoing = processStdin(&dest,sign,encryp);
         if (!keepGoing) 
         {
            break;
         } 
      }
       
      // //DebugLog ( << "Try TO PROCESS " );
      try
      {
         sipStack.process(fdset);
      }
      catch (...)
      {
         ErrLog( << "Got a exception from sipStack::process" );
      }
      
      try
      {
         tuIM->process();       
      }
      catch (...)
      {
         ErrLog( << "Got a exception passed from TuIM::process" );
      }
   }

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
