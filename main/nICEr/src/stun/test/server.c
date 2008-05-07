/*
Copyright (c) 2007, Adobe Systems, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Adobe Systems, Network Resonance nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

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
*/

#ifdef __cplusplus
#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdlib>   
#else
#ifndef WIN32
#include <string.h>
#endif
#endif

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#endif

#include "stun.h"
#include "udp.h"
#include "r_types.h"
#include "nr_startup.h"
#include "registry.h"
#include "stun_test_util.h"

#include "nr_crypto_openssl.h"

typedef struct
{
      UINT2 port;
      UINT4 addr;
} StunAddress4;

typedef struct
{
      UCHAR pad;
      UCHAR family;
      StunAddress4 ipv4;
} StunAtrAddress4;

static int alternate_server = 0;


#define MAGIC_COOKIE       0x2112A442

#define MAX_MEDIA_RELAYS 500
#define MAX_RTP_MSG_SIZE 1500
#define MEDIA_RELAY_TIMEOUT 3*60


int
nr_stun_parse_server_name( char* name, StunAddress4* addr)
{
   int r,_status;

   assert(name);

   if ((r=nr_stun_parse_host_name( name, &addr->addr, &addr->port, 3478))) {
       addr->port=0xFFFF;
       ABORT(r);
   }

   _status=0;
 abort: 
   return _status;
}


typedef struct
{
      int relayPort;       // media relay port
      int fd;              // media relay file descriptor
      StunAddress4 destination; // NAT IP:port
      time_t expireTime;      // if no activity after time, close the socket
} StunMediaRelay;

typedef struct
{
      StunAddress4 myAddr;
      Socket myFd;
} StunServerInfo;

void nr_stun_stop_server(StunServerInfo* info);

void 
usage()
{
   fprintf(stderr,"Usage: \n"
        " ./server [-v] [-b] [-h IP_Address] [-p port] [-a]\n"
        " \n"
        " STUN servers need two IP addresses and two ports, these can be specified with:\n"
        "  -h sets the primary IP\n"
        "  -p sets the primary port and defaults to 3478\n"
        "  -a tells the server to return an ALTERNATE-SERVER\n"
        "     for requests received at addresses other than -h\n"
        "  -b makes the program run in the backgroud\n"
        "  -v runs in verbose mode\n");
   exit(1);
}

static char *
nr_ip4toa(StunAddress4 *ipv4Addr)
{
    static char buf[256];
    int cnt;

    cnt = sprintf(buf,"%u.%u.%u.%u",
                      ((unsigned char *)&ipv4Addr->addr)[0],
                      ((unsigned char *)&ipv4Addr->addr)[1],
                      ((unsigned char *)&ipv4Addr->addr)[2],
                      ((unsigned char *)&ipv4Addr->addr)[3]);
    if (ipv4Addr->port != 0) {
        sprintf(&buf[cnt],":%hu",(unsigned int)ipv4Addr->port);
    }

    return buf;
}

static int returnpassword(void *arg, nr_stun_message *msg, Data **password)
{
    *password = (Data*)arg;
    return 0;
}

int
nr_stun_server_process(StunServerInfo* info)
{
   int r,_status;
   char msg[NR_STUN_MAX_MESSAGE_SIZE];
   int msgLen = sizeof(msg);

   char ok = 0;
   char recvAltIp =0;
   char recvAltPort = 0;

   fd_set fdSet;
   Socket maxFd=0;
   struct timeval tv;
   nr_stun_message *req;
   nr_stun_message *resp;
   Data password = { (UCHAR*)"hello", 5 };
   nr_transport_addr from2;

   FD_ZERO(&fdSet);
   FD_SET(info->myFd,&fdSet);
   if ( info->myFd >= maxFd ) maxFd=info->myFd+1;

   tv.tv_sec = 1;
   tv.tv_usec = 0;

   int e = select( maxFd, &fdSet, NULL,NULL, &tv );
   if (e < 0)
   {
      r_log_e(NR_LOG_STUN, LOG_WARNING, "Error on select");
   }
   else if (e >= 0)
   {
      StunAddress4 from;

      if (FD_ISSET(info->myFd,&fdSet))
      {
         r_log(NR_LOG_STUN, LOG_DEBUG, "received on A1:P1");
         recvAltIp = 0;
         recvAltPort = 0;
         ok = 1;
         if (getMessage( info->myFd, msg, &msgLen, &from.addr, &from.port))
             ok = 0;
      }
      else
      {
         goto done;
      }

      if ( !ok )
      {
         r_log(NR_LOG_STUN, LOG_DEBUG, "Get message did not return a valid message");
         goto done;
      }

      r_log(NR_LOG_STUN, LOG_DEBUG, "Got request (len=%u) from %s",msgLen,nr_ip4toa(&from));

      if ( msgLen <= 0 )
      {
         goto done;
      }


      if ((r=nr_stun_message_create2(&req, (UCHAR*)msg, msgLen))) {
        fprintf(stderr,"Error in nr_stun_message_create2\n");
        exit(1);
      }
     
      if ((r=nr_stun_decode_message(req, returnpassword, &password))) {
        fprintf(stderr,"Error in nr_stun_parse_message\n");
        exit(1);
      }

      if ((r=nr_stun_message_create(&resp))) {
        fprintf(stderr,"Error in nr_stun_message_create\n");
        exit(1);
      }

      nr_ip4_port_to_transport_addr(from.addr, from.port, IPPROTO_UDP, &from2);

      if ((r=nr_stun_receive_message(0, req))) {
         r_log(NR_LOG_STUN, LOG_DEBUG, "Failed to receive message");
         nr_stun_form_error_response(req, resp, 400, "Bad Message");
      }
      else if (alternate_server && from.addr != info->myAddr.addr) {
/* NOTE: this code is broken because we don't want 'from', we want 'to',
 * but UNIX doesn't provide a way to easily get that information, so
 * the -a flag only works when the client and server reside on the
 * same machine */
          /* send ALTERNATE-SERVER */
          nr_transport_addr alternate_server;
          nr_stun_form_error_response(req, resp, 300, "Alternate Server");
          nr_ip4_port_to_transport_addr(info->myAddr.addr, info->myAddr.port, IPPROTO_UDP, &alternate_server);
          nr_stun_message_add_alternate_server_attribute(resp, &alternate_server);
      }
      else if ((r=nr_stun_process_request(req,resp))) {
         r_log(NR_LOG_STUN, LOG_DEBUG, "Failed to process request");
         /* continue, even though failed to parse message, to send error message */
      }
      else {
          nr_stun_form_success_response(req, &from2, &password, resp);
      }

      if ((r=nr_stun_encode_message(resp)))
      {
          /* ignore this failure */
      }
      else if (from.addr != 0 && from.port != 0)
      {
         if ( info->myFd != INVALID_SOCKET )
         {
            sendMessage(info->myFd, (char*)resp->buffer, resp->length, from.addr, from.port);
         }
      }
   }

 done:
   _status=0;
// abort:
   return _status;
}
int
nr_stun_init_server(StunServerInfo* info, StunAddress4* myAddr)
{
   int _status;

   assert( myAddr->port != 0 );
   assert( myAddr->addr  != 0 );

   info->myAddr = *myAddr;

   info->myFd = INVALID_SOCKET;

   if ((info->myFd = openPort(myAddr->port, myAddr->addr)) == INVALID_SOCKET)
   {
      r_log(NR_LOG_STUN, LOG_WARNING, "Can't open %s", nr_ip4toa(myAddr));
      nr_stun_stop_server(info);

      ABORT(R_FAILED);
   }

   _status=0;
 abort:
   return _status;
}

void
nr_stun_stop_server(StunServerInfo* info)
{
   if (info->myFd > 0) closesocket(info->myFd);
}

int
main(int argc, char* argv[])
{
   StunAddress4 myAddr;
   int myPort = 0;
   char verbose=0;
   char background=0;
   
   printf("STUN server version %s\n",NR_STUN_VERSION);
   fflush(stdout);

   nr_crypto_openssl_set();

   nr_app_startup("server",NR_APP_STARTUP_INIT_LOGGING|NR_APP_STARTUP_REGISTRY_LOCAL,&NR_LOG_STUN,0,0);
   NR_reg_set_char("logging.stderr.enabled", 1);
   NR_reg_set_char("logging.syslog.enabled", 1);
   NR_reg_set_string("logging.syslog.facility.server.level", "debug");
   NR_reg_set_string("logging.syslog.facility.stun.level", "debug");
   NR_reg_set_string("logging.stderr.facility.client.level", "debug");
   NR_reg_set_string("logging.stderr.facility.stun.level", "debug");

   if (nr_stun_startup()) {
       printf("Failed to start STUN\n");
       exit(1);
   }
      
   if (initNetwork()) {
       printf("Failed to start STUN\n");
       exit(1);
   }
 
   myAddr.addr = 0;
   myAddr.port = NR_STUN_PORT;
 
   if ( argc <= 2 ) 
   {
      usage();
      exit(-1);
   }

   int arg;
   for ( arg = 1; arg<argc; arg++ )
   {
      if ( !strcmp( argv[arg] , "-v" ) )
      {
         verbose = 1;
      }
      else if ( !strcmp( argv[arg] , "-b" ) )
      {
         background = 1;
      }
      else if ( !strcmp( argv[arg] , "-a" ) )
      {
         alternate_server = 1;
      }
      else if ( !strcmp( argv[arg] , "-h" ) )
      {
         arg++;
         if ( argc <= arg ) 
         {
            usage();
            exit(-1);
         }

         if (nr_stun_parse_server_name(argv[arg], &myAddr))
         {
            usage();
            exit(-1);
         }
      }
      else if ( !strcmp( argv[arg] , "-p" ) )
      {
         arg++;
         if ( argc <= arg ) 
         {
            usage();
            exit(-1);
         }
         myPort = (UINT2)(strtol( argv[arg], NULL, 10));
      }
      else
      {
         usage();
         exit(-1);
      }
   }

   if ( myPort != 0 )
   {
      myAddr.port = myPort;
   }
 
   if (
      ( myAddr.addr  == 0 ) ||
      ( myAddr.port  == 0 ) 
      )
   {
      fprintf(stderr,"Bad command line\n");
      fflush(stderr);
      exit(1);
   }
   
#if defined(WIN32)
   int pid=0;

   if ( background )
   {
      fprintf(stderr,"The -b background option does not work in windows\n");
      fflush(stderr);
      exit(-1);
   }
#else
   pid_t pid=0;

   if ( background )
   {
      pid = fork();

      if (pid < 0)
      {
         fprintf(stderr,"fork: unable to fork\n");
	 fflush(stderr);
         exit(-1);
      }
   }
#endif

   if (pid == 0) //child or not using background
   {
      StunServerInfo info;
      char ok = 1;
      if (nr_stun_init_server(&info, &myAddr))
          ok = 0;
      
      int c=0;
      while (ok)
      {
         if (nr_stun_server_process(&info)) {
             fprintf(stderr,"Failed\n");
             exit(1);
         }
         c++;
         if ( c%10000 == 0 ) 
         {
            printf("*");
	    fflush(stdout);
         }
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


// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:
