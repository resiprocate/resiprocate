
#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "resiprocate/util/Socket.hxx"
#include "resiprocate/util/Logger.hxx"

#include "resiprocate/sipstack/SipStack.hxx"
#include "resiprocate/sipstack/Security.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{  
   Log::initialize(Log::COUT, Log::ERR, argv[0]);
   int port = 0;
   int dest = 0;
   
   InfoLog(<<"Test Driver for IM Starting");
    
   for ( int i=1; i<argc; i++)
   {
      if (!strcmp(argv[i],"-vv"))
      {
         Log::setLevel(Log::DEBUG);
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
      else if (!strcmp(argv[i],"-to"))
      {
         i++;
         assert( i<argc );
         dest = atoi( argv[i] );
      } 
      else
      { 
         ErrLog(<<"Bad command line opion: " << argv[i] );
         ErrLog(<<"options are: [-v] [-vv] [-port 1234] [-aor sip:flffuy@flouf.com] [-to sip:1@foo.com]" << argv[i] );
         assert(0);
      }
   }
   
   InfoLog( << "Using port " << port );
   
   SipStack sipStack;  

#ifdef USE_SSL
   assert( sipStack.security );
   bool ok = sipStack.security->loadAllCerts( Data("password") );
   if ( !ok )
   {
      ErrLog( << "Could not load the certificates" );
      assert( ok );
   }
#endif

   if ( port != 0 )
   {
      // do the server side
      Socket mFd = socket(PF_INET, SOCK_STREAM, 0);

      if ( mFd == INVALID_SOCKET )
      {
         InfoLog (<< "Failed to open socket: " << port);
      }
   
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY); 
      addr.sin_port = htons(port);
   
      if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR )
      {
         int err = errno;
         if ( err == EADDRINUSE )
         {
            InfoLog (<< "Address already in use");
         }
         else
         {
            InfoLog (<< "Could not bind to port: " << port);
         }
      
         assert(0);
      }

      // bind it to the local addr

      // make non blocking 
#if 0
#if WIN32
      unsigned long block = 0;
      int errNoBlock = ioctlsocket( mFd, FIONBIO , &block );
      assert( errNoBlock == 0 );
#else
      int flags  = fcntl( mFd, F_GETFL, 0);
      int errNoBlock = fcntl(mFd,F_SETFL, flags| O_NONBLOCK );
      assert( errNoBlock == 0 );
#endif
#endif

      ErrLog( << "Listening for connections " );;

      // do the listen
      int e = listen( mFd , /* qued requests */ 64 );
      if (e != 0 )
      {
         //int err = errno;
         // !jf! deal with errors
         assert(0);
      }
      struct sockaddr_in peer;
		
#ifdef __MACH__
      int peerLen=sizeof(peer);
#else
      socklen_t peerLen=sizeof(peer);
#endif
      Socket s = accept( mFd, (struct sockaddr*)&peer,&peerLen);
      if ( s == -1 )
      {
         //int err = errno;
         // !jf!
         assert(0);
      }
      ErrLog( << "did accept"  );

      TlsConnection tls( sipStack.security, s, /*server*/ true );
      ErrLog( << "Started TLS server"  );

      Data p =  tls.peerName();
      ErrLog( << "Connected to " << p  );
     
      ErrLog( << "Ready to read "  );

      while (true)
      {
         char buf[1024];
         
         int r = tls.read( buf, sizeof(buf) );
         buf[r] = 0;
         
         ErrLog( << "read " << r << " bytes" );
         ErrLog( << buf );
      }
       
   }
   else
   {
      // do the client side   
  
      // attempt to open
      int sock = socket( AF_INET, SOCK_STREAM, 0 );
      if ( sock == -1 )
      {
         assert(0);
      }
      
      struct sockaddr_in servaddr;
      
      memset( &servaddr, sizeof(servaddr), 0 );
      servaddr.sin_family = AF_INET;
      servaddr.sin_port = htons(dest);
      servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      
      ErrLog("trying to connect ");

      int e = connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
      if ( e == -1 ) 
      {
         ErrLog("connected failed");
         assert(0);
      }

      ErrLog("connected ");

      TlsConnection tls( sipStack.security, sock );
      ErrLog("Started TLS client ");

      Data p =  tls.peerName();
      ErrLog( << "Connected to " << p  );

      ErrLog( << "Ready to go "  );
      char* msg = "Hello World";
      int r = tls.write(msg,strlen(msg) );
      assert( r == (int)strlen(msg) );
      
      ErrLog( << "Ready to read "  );
      while (true)
      {
         char buf[1024];
         
         int r = tls.read( buf, sizeof(buf) );
         buf[r] =0;
         
         ErrLog( << "read " << r  );
      } 
   }
}

