#include <cstring>
#include <cassert>
#include <iostream>

#include <ncurses.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/TuIM.hxx"
#include "sip2/sipstack/Security.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP


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
      virtual void presenseUpdate(const Uri& dest, bool open, const Data& status );
      virtual void receivedPage( const Data& msg, const Uri& from ,
                                 const Data& signedBy,  Security::SignatureStatus sigStatus,
                                 bool wasEncryped  );
      virtual void sendPageFailed( const Uri& dest,int respNumber );
      virtual void registrationFailed(const Vocal2::Uri&, int respNumber);
};
  

void 
TestCallback::presenseUpdate(const Uri& from, bool open, const Data& status )
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
                            const Data& signedBy,  Security::SignatureStatus sigStatus,
                            bool wasEncryped  )
{  
   //DebugLog(<< "In TestPageCallback");

   if ( dest != from )
   {
      dest = from;
      //cerr << "Set destination to <" << *mDest << ">" << endl;
      waddstr(textWin,"Set destination to ");
      waddstr(textWin, dest.value().c_str());
      waddstr(textWin,"\n");
   }
   
   //cout << from;  

   waddstr(textWin,"From: ");
   waddstr(textWin,from.getAor().c_str());

   if ( !wasEncryped )
   {
      //cout << " -NOT SECURE- ";
      waddstr(textWin," -NOT SECURE- ");
   }
   switch ( sigStatus )
   {
      case  Security::isBad:
         //cout << " -bad signature- ";
         waddstr(textWin,"bad signature");
      break;
      case  Security::none:
         //cout << " -no signature- ";
         waddstr(textWin,"no signature");
         break;
      case  Security::trusted:
         //cout << " <signed  " << signedBy << " > ";
         waddstr(textWin,"signed ");
         waddstr(textWin,signedBy.c_str());
         break;
      case  Security::caTrusted:
         //cout << " <ca signed  " << signedBy << " > ";
         waddstr(textWin,"ca signed " );
         waddstr(textWin,signedBy.c_str());
         break;
      case  Security::notTrusted:
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
TestCallback::sendPageFailed( const Uri& dest, int respNum )
{
   //InfoLog(<< "In TestErrCallback");  
   // cerr << "Message to " << dest << " failed" << endl;  
   Data num(respNum);
   
   waddstr(textWin,"Message to ");
   waddstr(textWin,dest.value().c_str());
   waddstr(textWin," failed (");
   waddstr(textWin,num.c_str());
   waddstr(textWin," response)\n");
   wrefresh(textWin);
}


void 
TestCallback::registrationFailed(const Vocal2::Uri&, int respNum )
{
   Data num(respNum);
   
   waddstr(textWin,"Registration to ");
   waddstr(textWin,dest.value().c_str());
   waddstr(textWin," failed (");
   waddstr(textWin,num.c_str());
   waddstr(textWin," response)\n");
   wrefresh(textWin);
}
  
                              
bool
processStdin( Uri* dest )
{
   static unsigned int num=0;
   static char buf[1024];

   char c = getch();	
      
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

#if 0
   char junk[6];
   junk[0]=' ';
   junk[1]='0'+(c/100);
   junk[2]='0'+((c/10)%10);
   junk[3]='0'+(c%10);
   junk[4]=' ';
   junk[5]=0;
   waddstr(commandWin,junk);
#endif

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
       
   if ( (c == '\r') || (num+2>=sizeof(buf)) )
   {
      buf[num] =0;

      if ( (num>3) && (!strncmp("to:",buf,3)) )
      {
         buf[num] = 0;
         *dest = Uri(Data(buf+3));
       
         //cerr << "Set destination to <" << *dest << ">";
         waddstr(textWin,"Set destination to ");
         waddstr(textWin,dest->value().c_str());
         waddstr(textWin,"\n");
         wrefresh(textWin);
      }
      else if ( (num>4) && (!strncmp("add:",buf,4)) )
      {
         buf[num] = 0;
         Uri uri(Data(buf+4));

         //cerr << "Subscribing to buddy <" << uri << ">";
         waddstr(textWin,"Subscribing to ");
         waddstr(textWin,uri.value().c_str());
         waddstr(textWin,"\n");
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

         tuIM->setMyPresense( !stat.empty(), stat );
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
         
            //DebugLog( << "Read <" << text << ">" );
            
            Data destValue  = dest->getAor();
            
            //cout << "Send to <" << *dest << ">";
            waddstr(textWin,"To: ");
            waddstr(textWin,destValue.c_str());
            waddstr(textWin," ");
            waddstr(textWin,text.c_str());
            waddstr(textWin,"\n");
            wrefresh(textWin);

            tuIM->sendPage( text , *dest, false /*sign*/, Data::Empty /*encryptFor*/ );
            //tuIM.sendPage( text , *dest, false /*sign*/, dest->getAorNoPort() /*encryptFor*/ );
            //tuIM.sendPage( text , *dest, true /*sign*/,  Data::Empty /*encryptFor*/ );
            //tuIM.sendPage( text , *dest, true /*sign*/, dest->getAorNoPort()
            ///*encryptFor*/ );
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
#ifdef ERR // ncurses defines a macro called ERR 
   Log::initialize(Log::COUT, Log::SIP2_ERR, argv[0]);
   Log::setLevel(Log::SIP2_ERR);
#else
   Log::initialize(Log::COUT, Log::ERR, argv[0]);
   Log::setLevel(Log::ERR);
#endif  

   InfoLog(<<"Test Driver for IM Starting");
    
   int port = 5060;
   int tlsPort = 0;
   Uri aor;
   bool haveAor=false;
   dest = Uri("sip:nobody@example.com");
   Data aorPassword;
   
   int numAdd=0;
   Data addList[100];
   
   for ( int i=1; i<argc; i++)
   {
      if (!strcmp(argv[i],"-vv"))
      {
         Log::setLevel(Log::DEBUG_STACK);
      }
      else if (!strcmp(argv[i],"-v"))
      {
         Log::setLevel(Log::INFO);
      }
      else if (!strcmp(argv[i],"-port"))
      {
         i++;
         assert( i<argc );
         port = atoi( argv[i] );
      } 
      else if (!strcmp(argv[i],"-tlsport"))
      {
         i++;
         assert( i<argc );
         tlsPort = atoi( argv[i] );
      } 
      else if (!strcmp(argv[i],"-aor"))
      {
         i++;
         assert( i<argc );
         aor = Uri(Data(argv[i]));
         haveAor=true;
      } 
      else if (!strcmp(argv[i],"-add"))
      {
         i++;
         assert( i<argc );
         addList[numAdd++] = Data(argv[i]);
         assert( numAdd < 100 );
      } 
      else if (!strcmp(argv[i],"-aorPassword"))
      {
         i++;
         assert( i<argc );
         aorPassword = Data(argv[i]);
      } 
      else if (!strcmp(argv[i],"-to"))
      {
         i++;
         assert( i<argc );
         dest = Uri(Data(argv[i]));
      } 
      else
      { 
         cerr <<"Bad command line opion: " << argv[i] << endl;
         cerr <<"options are: " << endl
              << "\t [-v] [-vv] [-port 5060] [-tlsport 5061]" << endl
              << "\t [-aor sip:alice@example.com] [-aorPassword password]" << endl
              << "\t [-to sip:friend@example.com] [-add sip:buddy@example.com]" << endl;
         cerr << endl
              << " -v is verbose" << endl
              << " -vv is very verbose" << endl
              << " -port sets the UDP and TCP port to listen on" << endl
              << " -tlsPort sets the port to listen for TLS on" << endl
              << " -aor sets the proxy and user name to register with" << endl
              << " -aorPasswrod sets the password to use for registration" << endl
              << " -to sets initial location to send messages to" << endl
              << " -add adds a budy who's presense will be monitored" << endl
              << "\t there can be many -add " << endl
              << endl;
         exit(1);
      }
   }
   
   //InfoLog( << "Using port " << port );
   
   SipStack sipStack;  

#ifdef USE_SSL
   assert( sipStack.security );
   bool ok = sipStack.security->loadAllCerts( Data("password") );
   if ( !ok )
   {
      //ErrLog( << "Could not load the certificates" );
      assert( ok );
   }
#endif
   
   if (port!=0)
   {
      sipStack.addTransport(Transport::UDP, port);
      sipStack.addTransport(Transport::TCP, port);
   }
   
#if USE_SSL
   if ( port == 5060 )
   {
      if ( tlsPort == 0 )
      {
         tlsPort = 5061;
      }
   }
   if ( tlsPort != 0 )
   {
      sipStack.addTransport(Transport::TLS, tlsPort);
   }
#endif

   TestCallback callback;

   Uri contact("sip:user@localhost");
   contact.port() = port;
   contact.host() = sipStack.getHostname();

   if ( haveAor )
   {
      contact.user() = aor.user();
#if USE_SSL
      if ( aor.scheme() == "sips" )
      {
         contact.scheme() = aor.scheme();
         contact.port() = tlsPort;
      }
#endif
   }
   else
   {
      aor = contact;
   }
   
   tuIM = new TuIM(&sipStack,aor,contact,&callback);

   if ( haveAor )
   {
      tuIM->registerAor( aor, aorPassword );
   }
   
   initscr(); 
   cbreak(); 
   noecho();
   nonl();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);

   int rows,cols;
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

   displayPres();
 
   waddstr(textWin,"Use -help on the command line to view options\n");
   waddstr(textWin,"To set where your messages will get sent type\n");
   waddstr(textWin,"    to: sip:alice@example.com \n");
   waddstr(textWin,"To monitores someeone presense type\n");
   waddstr(textWin,"    add: sip:buddy@example.com \n");
   waddstr(textWin,"To change you online status type\n");
   waddstr(textWin,"    status: in meetin\n");
   waddstr(textWin,"To set youselft to offline type\n");
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

      int  err = fdset.selectMiliSeconds( time );
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
         bool keepGoing = processStdin(&dest);
         if (!keepGoing) 
         {
            break;
         } 
      }
       
      // //DebugLog ( << "Try TO PROCESS " );
      sipStack.process(fdset);
       
      tuIM->process();       
   }
}

