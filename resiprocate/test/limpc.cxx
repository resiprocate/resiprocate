
#include <cstring>
#include <cassert>

//#define USE_CURSES

#ifdef USE_CURSES
#include <ncurses.h>
#else
#include <iostream>
#include <cstdio>
#include <unistd.h>
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
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"


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
      virtual void presenseUpdate(const Uri& dest, bool open, const Data& status );
      virtual void receivedPage( const Data& msg, const Uri& from ,
                                 const Data& signedBy,  Security::SignatureStatus sigStatus,
                                 bool wasEncryped  );
      virtual void sendPageFailed( const Uri& dest,int respNumber );
      virtual void registrationFailed(const resip::Uri&, int respNumber); 
      virtual void registrationWorked(const Uri& dest );
      virtual void receivePageFailed(const Uri& sender);
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
            
            Data destValue  = dest->getAor();
            Data encFor = Data::Empty;
            if (encryp)
            { 
               encFor = dest->getAorNoPort();
            }

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
   Uri contact("sip:user@localhost");
   bool haveContact=false;
   Uri outbound;
   bool noRegister = false;
   bool tlsServer=false;
   
   int numAdd=0;
   Data addList[100];
   bool encryp=false;
   bool sign=false;
   Data key("password");
   bool useTls = true;
   
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
      else if (!strcmp(argv[i],"-encrypt"))
      {
         encryp = true;
      }
      else if (!strcmp(argv[i],"-noRegister"))
      {
         noRegister = true;
      }
      else if (!strcmp(argv[i],"-sign"))
      {
         sign = true;
      }
      else if (!strcmp(argv[i],"-tlsServer"))
      {
         tlsServer = true;
      }
      else if (!strcmp(argv[i],"-ssl"))
      {
         useTls = false;
      }
      else if (!strcmp(argv[i],"-port"))
      {
         i++;
         assert( i<argc );
         port = atoi( argv[i] );
      } 
      else if (!strcmp(argv[i],"-tlsPort"))
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
      else if (!strcmp(argv[i],"-outbound"))
      {
         i++;
         assert( i<argc );
         outbound = Uri(Data(argv[i]));
         haveAor=true;
      } 
      else if (!strcmp(argv[i],"-contact"))
      {
         i++;
         assert( i<argc );
         contact = Uri(Data(argv[i]));
         haveContact=true;
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
     else if (!strcmp(argv[i],"-key"))
      {
         i++;
         assert( i<argc );
         key = Data(argv[i]);
      } 
      else
      { 
         clog <<"Bad command line opion: " << argv[i] << endl;
         clog <<"options are: " << endl
              << "\t [-v] [-vv] [-port 5060] [-tlsport 5061]" << endl
              << "\t [-aor sip:alice@example.com] [-aorPassword password]" << endl
              << "\t [-to sip:friend@example.com] [-add sip:buddy@example.com]" << endl
              << "\t [-sign] [-encrypt] [-key secret]" << endl
              << "\t [-contact sip:me@example.com] " << endl
              << "\t [-outbound \"sip:example.com;lr\"] " << endl
              << "\t [-noRegister] " << endl;
         clog << endl
              << " -v is verbose" << endl
              << " -vv is very verbose" << endl
              << " -port sets the UDP and TCP port to listen on" << endl
              << " -tlsPort sets the port to listen for TLS on" << endl
              << " -tlsServer - sets to act as tls server instead of  client" << endl
              << " -ssl - use ssl instead of tls" << endl
              << " -aor sets the proxy and user name to register with" << endl
              << " -aorPassword sets the password to use for registration" << endl
              << " -noRegister causes it not to register - by default the AOR is registered" << endl
              << " -to sets initial location to send messages to" << endl
              << " -outbound sets the outbound proxy" << endl
              << " -add adds a budy who's presense will be monitored" << endl
              << " -sign signs message you send and -encryp encrypt them " << endl
              << "\t(You need PKI certs for this to work)" << endl
              << " -key allows you to enter a secret used to load your private key."<< endl
              << "  If you set the secret to - the system will querry you for it."<< endl
              << " -contact overrides your SIP contact - can be used for NAT games" << endl
              << "\t there can be many -add " << endl
              << " " << endl
              << "Examples" << endl
              << "An example command line for a user with account name alice at example.com is:" << endl
              << "\t" << argv[0] << " -aor \"alice@example.com\" -aorPassword \"secret\"" << endl
              << "to watch the presence of bob and charlie add" << endl
              << "\t-add \"sip:bob@bilboxi.com\" -add \"charlie@example.com\" " << endl
              << "If Alice was behind a NAT that had a public address of 1.2.3.4 and had forwarded" << endl
              << "port 5070 on this NAT to the machine Alice was using, then the following " << endl
              << "options would be added" << endl
              << "\t-contact \"sip:alice@1.2.3.4:5070\" -port 5070" << endl
              << "" << endl
              << endl;
         exit(1);
      }
   }
   
   //InfoLog( << "Using port " << port );
   
   Security security( tlsServer, useTls );
   SipStack sipStack( false /*multihtread*/, &security );  

   if ( key == Data("-") )
   {
      clog << "Please enter password to use to load your private key: ";
      char buf[1024];
      cin.get(buf,1024);
      key = Data(buf);
      InfoLog( << "Certificate key set to <" << key << ">" );
   }
   
#ifdef USE_SSL
   assert( sipStack.security );
   bool ok = sipStack.security->loadAllCerts( key , Data::Empty );
   if ( !ok )
   {
      InfoLog( << "Could not load the certificates" );
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

   if (!haveContact)
   {
      contact.port() = port;
      contact.host() = sipStack.getHostname();
   }
   
   if ( haveAor )
   {
      if (!haveContact)
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
   }
   else
   {
      aor = contact;
   }

   InfoLog( << "aor is " << aor );
   InfoLog( << "contact is " << contact );
   TestCallback callback;
   tuIM = new TuIM(&sipStack,aor,contact,&callback);

   Data name("SIPimp.org/0.2.1 (curses)");
   tuIM->setUAName( name );
      
   if ( !outbound.host().empty() )
   {
      tuIM->setOutboundProxy( outbound );
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
      sipStack.process(fdset);
       
      tuIM->process();       
   }
}

