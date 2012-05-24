#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <cstring>
#include <cassert>
#include <stdio.h>
#include <signal.h>
//#define USE_CURSES

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
// !ah! Really ought to check for ncurses and be a bit better behaved than this.
#if !defined(TRUE)
#define TRUE true
#endif
#if !defined(FALSE)
#define FALSE false
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

#include "rutil/FdPoll.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/TuIM.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

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

class StdInWatcher : public resip::FdPollItemIf
{
   public:
      StdInWatcher(Uri* dest, bool sign, bool encrypt) :
         mDest(dest),
         mSign(sign),
         mEncrypt(encrypt),
         mKeepGoing(true)
      {}

      virtual ~StdInWatcher(){}

      virtual void processPollEvent(FdPollEventMask mask)
      {
         mKeepGoing=processStdin(mDest, mSign, mEncrypt);
      }

      inline bool keepGoing() const {return mKeepGoing;} 

   private:
      Uri* mDest;
      bool mSign;
      bool mEncrypt;
      bool mKeepGoing;

}; // class StdInWatcher


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
      case  SignatureSelfSigned:
         //cout << " -self signed signature (bad)- ";
         waddstr(textWin,"bad signature");
	 break;
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
  
                              
int
main(int argc, char* argv[])
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

   int r;
   
   try
   {
      r = myMain( argc, argv );
   }
   catch( ... )
   {
      ErrLog( << "Got a exception passed all the way to the top of limp" );
      exit(-1);
   }
   
   return r;
}


static int
myMain(int argc, char* argv[])
{  
   Log::initialize(Log::Cerr, Log::Err, argv[0]);
   Log::setLevel(Log::Warning);

   InfoLog(<<"Test Driver for IM Starting");
    
   int port = 5060;
   int tlsPort = 0;
   int dtlsPort = 0;
   Uri aor;
   bool haveAor=false;
   dest = Uri("sip:nobody@example.com");
   Data aorPassword;
   Uri contact("sip:user@");
   bool haveContact=false;
   Uri outbound;
   bool noRegister = false;
   Data tlsDomain = Data::Empty;

   Data sendMsg = Data::Empty;
   
   int numAdd=0;
   Data addList[100];
   int numPub=0;
   Data pubList[100];
   bool encryp=false;
   bool sign=false;
   Data key("password");
   bool useTls = true;
   bool noTls = false;
   bool noTcp = false;
   bool prefUdp = false;
   bool prefTls = false;
   bool prefDtls = false;
   bool prefTcp = false;
   bool noUdp = false;
   bool noV6 = false;
   bool noV4 = false;
   bool genUserCert = false;
   
   for ( int i=1; i<argc; i++)
   {
      if (!strcmp(argv[i],"-vv"))
      {
         Log::setLevel(Log::Stack);
      }
      else if (!strcmp(argv[i],"-v"))
      {
         Log::setLevel(Log::Info);
      }
      else if (!strcmp(argv[i],"-encrypt"))
      {
         encryp = true;
      }
      else if (!strcmp(argv[i],"-genUserCert"))
      {
         genUserCert = true;
      }
      else if (!strcmp(argv[i],"-noRegister"))
      {
         noRegister = true;
      }
      else if (!strcmp(argv[i],"-sign"))
      {
         sign = true;
      }
      else if (!strcmp(argv[i],"-tlsDomain"))
      {
         i++;
         assert( i<argc );
         tlsDomain = Data(argv[i]);
      }
      else if (!strcmp(argv[i],"-ssl"))
      {
         useTls = false;
      }
      else if (!strcmp(argv[i],"-noTcp"))
      {
         noTcp = true;
      }
      else if (!strcmp(argv[i],"-noTls"))
      {
         noTls = true;
      }
      else if (!strcmp(argv[i],"-noUdp"))
      {
         noUdp = true;
      }
      else if (!strcmp(argv[i],"-prefTcp"))
      {
         prefTcp = true;
      }
      else if (!strcmp(argv[i],"-prefTls"))
      {
         prefTls = true;
      }
      else if (!strcmp(argv[i],"-prefUdp"))
      {
         prefUdp = true;
      }
      else if (!strcmp(argv[i],"-noV6"))
      {
         noV6 = true;
      }
      else if (!strcmp(argv[i],"-noV4"))
      {
         noV4 = true;
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
      else if (!strcmp(argv[i],"-dtlsPort"))
      {
	 i++;
	 assert( i<argc );
	 dtlsPort = atoi( argv[i] );
      }
      else if (!strcmp(argv[i],"-aor"))
      {
         i++;
         assert( i<argc );
         try
         {
            aor = Uri(Data(argv[i]));
         }
         catch (...)
         {
            ErrLog( <<"AOR URI is not valid - must start with sip: ");
            exit(-1);
         }
         haveAor=true;
      } 
      else if (!strcmp(argv[i],"-outbound"))
      {
         i++;
         assert( i<argc );
	 try
         {
            outbound = Uri(Data(argv[i]));
         }
	 catch (...)
	 {
	    ErrLog( <<"Outbound URI is not valid - must start with sip: ");
	    exit(-1);
         }
      } 
      else if (!strcmp(argv[i],"-send"))
      {
         i++;
         assert( i<argc );
         sendMsg = Data(argv[i]);
      } 
      else if (!strcmp(argv[i],"-contact"))
      {
         i++;
         assert( i<argc );
         try
         {
            contact = Uri(Data(argv[i]));
         }
         catch (...)
         {
            ErrLog( <<"Contact URI is not valid - must start with sip: ");
            exit(-1);
         }
         haveContact=true;
      } 
      else if (!strcmp(argv[i],"-add"))
      {
         i++;
         assert( i<argc );
         addList[numAdd++] = Data(argv[i]);
         assert( numAdd < 100 ); 
         try
         {
            // CJ TODO FIX 
            //Uri uri( Data(argv[i]) );
         }
         catch (...)
         {
            ErrLog( <<"URI in -add is not valid - must start with sip: ");
            exit(-1);
         }
      } 
      else if (!strcmp(argv[i],"-pub"))
      {
         i++;
         assert( i<argc );
         pubList[numPub++] = Data(argv[i]);
         assert( numPub < 100 ); 
         try
         {
            // CJ TODO FIX 
            //Uri uri(Data(argv[i]));
         }
         catch (...)
         {
            ErrLog( <<"Pub URI is not valid - must start with sip: ");
            exit(-1);
         }
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
         try
         {
            dest = Uri(Data(argv[i])); 
         }
         catch (...)
         {
            ErrLog( <<"To URI is not valid - must start with sip: ");
            exit(-1);
         }
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
              << "\t [-v] [-vv] [-tls] [-port 5060] [-tlsport 5061]" << endl
              << "\t [-aor sip:alice@example.com] [-aorPassword password]" << endl
              << "\t [-to sip:friend@example.com] [-add sip:buddy@example.com]" << endl
              << "\t [-sign] [-encrypt] [-key secret]" << endl
              << "\t [-contact sip:me@example.com] " << endl
              << "\t [-outbound \"sip:example.com;lr\"] " << endl
              << "\t [-noRegister] " << endl
              << "\t [-pub sip:foo.com] " << endl
              << "\t [-tlsDomain foo.com] " << endl
              << "\t [-send myMessage] " << endl; 
         clog << endl
              << " -v is verbose" << endl
              << " -vv is very verbose" << endl
              << " -noV6 don't use IPv6" << endl
              << " -noV4 don't use IPv4" << endl
              << " -noUdp don't use UDP" << endl
              << " -noTcp don't use TCP" << endl
              << " -noTls don't use TLS" << endl
              << " -prefUdp prefer UDP" << endl
              << " -prefTcp prefer TCP" << endl
              << " -prefTls prefer TLS" << endl
              << " -port sets the UDP and TCP port to listen on" << endl
              << " -tlsPort sets the port to listen for TLS on" << endl
              << " -tlsDomain domainName - sets tls and dtls to act as tls server instead of client" << endl
              << " -ssl - use ssl instead of tls" << endl
              << " -aor sets the proxy and user name to register with" << endl
              << " -aorPassword sets the password to use for registration" << endl
              << " -noRegister causes it not to register - by default the AOR is registered" << endl
              << " -to sets initial location to send messages to" << endl
              << " -outbound sets the outbound proxy" << endl
              << " -add adds a budy who's presence will be monitored" << endl
              << " -pub adds a State Agent to send publishes too" << endl
              << " -sign signs message you send and -encryp encrypt them " << endl
              << " -send takes a string (needs to be quoted if it has spaces) " 
              <<                                      "and sends it as an IM " << endl
              << "\t(You need PKI certs for this to work)" << endl
              << " -key allows you to enter a secret used to load your private key."<< endl
              << "  If you set the secret to - the system will querry you for it."<< endl
              << " -contact overrides your SIP contact - can be used for NAT games" << endl
              << "\t there can be many -add " << endl
              << " -genUserCert - generate a new user cert" << endl
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
  
#ifdef USE_SSL
   InfoLog( << "Setting up Security" );
   Security* security=NULL;
   try
   {
      char cert_dir[ 1024 ] ;
      char *home_dir = getenv( "HOME" ) ;

      cert_dir[ 0 ] = '\0' ;
      ::strcat( cert_dir, home_dir ) ;
      ::strcat( cert_dir, "/.sipCerts/" ) ;

      security = new Security( cert_dir ) ;

      //  ::free( home_dir ) ; // CJ TODO mem leak 
   }
   catch( ... )
   {
      security = NULL;
      ErrLog( << "Got a exception setting up Security" );
   }

   SipStack sipStack( security );  
#else
   SipStack sipStack( false /*multihtread*/ );  
#endif

   if ( key == Data("-") )
   {
      clog << "Please enter password to use to load your private key: ";
      char buf[1024];
      cin.get(buf,1024);
      key = Data(buf);
      InfoLog( << "Certificate key set to <" << key << ">" );
   }
   
#ifdef USE_SSL
   try
   {
      Security* security = sipStack.getSecurity();
      assert(security != 0);
   }
   catch( ... )
   {
      ErrLog( << "Got an exception creating security object " );
   }

   try
   {
      assert(security != 0);
      security->preload();
   }
   catch( ... )
   {
      ErrLog( << "Got a exception pre loading certificates" );
   }


   if (genUserCert)
   {
      assert( security );
      security->generateUserCert(aor.getAor());
   }
#endif

   DebugLog( << "About to add the transports " );   
   if (port!=0)
   {
      if ( noUdp != true )
      {
         if (!noV4) sipStack.addTransport(UDP, port, V4);
#ifdef USE_IPV6
         if (!noV6) sipStack.addTransport(UDP, port, V6);
#endif
      }
      if ( noTcp != true )
      {
         if (!noV4) sipStack.addTransport(TCP, port, V4);
#ifdef USE_IPV6
         if (!noV6) sipStack.addTransport(TCP, port, V6);
#endif
      }
   }
#ifdef USE_SSL
   if ( tlsPort != 0 )
   {
      if ( noTls != true )
      {
         if (!noV4) 
         {
            sipStack.addTransport(TLS, tlsPort, V4, StunDisabled, Data::Empty, tlsDomain );
         }
	 //if (!noV6) sipStack.addTlsTransport(tlsPort,Data::Empty,Data::Empty,Data::Empty,V6);
      }
   }
#ifdef USE_DTLS
   if ( dtlsPort != 0 )
   {
      if ( noTls != true )
      {
         if (!noV4) 
         {
            sipStack.addTransport(DTLS, dtlsPort, V4, StunDisabled, Data::Empty, tlsDomain );
         }
      }
   }
#endif
#endif

   DebugLog( << "Done adding the transports " );   

   if (!haveContact)
   {
      // contact.port() = port;
      // contact.host() = sipStack.getHostname();
   }
   
   if ( haveAor )
   {
      if (!haveContact)
      {
         contact.user() = aor.user();
#ifdef USE_SSL
         if ( aor.scheme() == "sips" )
         {
            contact.scheme() = aor.scheme();
            //contact.port() = tlsPort;
         }
#endif
      }
   }
   else
   {
      aor.port() = port;
      aor.host() = sipStack.getHostname();
      aor.user() = Data("user");
   }

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

   if ( !sendMsg.empty() )
   {
         tuIM->sendPage( sendMsg , dest, sign , (encryp) ?
                         (dest.getAorNoPort()) : (Data::Empty) );
   }

   StdInWatcher watcher(&dest,sign,encryp);
   FdPollItemHandle wh=sipStack.getPollGrp()->addPollItem(fileno(stdin), FPEM_Read, &watcher);

   while (1)
   {
      try
      {
         sipStack.process( 50 );
      }
      catch (...)
      {
         ErrLog( << "Got a exception from sipStack::process" );
      }

      if(!watcher.keepGoing())
      {
         break;
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

   sipStack.getPollGrp()->delPollItem(wh);

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
