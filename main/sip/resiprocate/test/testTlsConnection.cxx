#if defined(HAVE_CONFIG_HXX)
#include "resiprocate/config.hxx"
#endif

#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"

#include "resiprocate/SipStack.hxx"
#include "resiprocate/Security.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

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
