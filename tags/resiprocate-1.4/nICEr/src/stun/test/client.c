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

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "stun.h"
#include "udp.h"
#include "r_types.h"
#include "nr_startup.h"
#include "registry.h"
#include "byteorder.h"
#include "stun_test_util.h"

#include "nr_crypto_openssl.h"

#define MAGIC_COOKIE       0x2112A442
#define MAGIC_COOKIE2      0xc5cb4e1d  /* used recognize old stun messages */

int nr_stun_randomPort(int *port);

static int stun_mode = NR_STUN_MODE_STUN;
static int auth_rule = NR_STUN_AUTH_RULE_SHORT_TERM;

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

typedef struct
{
      char value[NR_STUN_MAX_STRING_SIZE];
      UINT2 sizeValue;
} StunAtrString;

void
usage()
{
   printf("Usage:\n"
#ifdef USE_STUND_0_96
	  "    ./client stunServerHostname [-v] [-c] [-n|-s|-l] [-i nicAddr [-p srcPort]]\n"
#else
	  "    ./client stunServerHostname [-v] [-n|-s|-l] [-i nicAddr [-p srcPort]]\n"
#endif
	  "For example, if the STUN server was larry.gloo.net, you could do:\n"
	  "    ./client larry.gloo.net\n");
   fflush(stdout);
       
}

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


#define MAX_NIC 8

static void
nr_stun_send_test( Socket myFd, StunAddress4* dest,
              StunAtrString* username, StunAtrString* password,
              int testNum);


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

int returnpassword(void *arg, nr_stun_message *msg, Data **password)
{
    *password = (Data*)arg;
    return 0;
}

int
nr_stun_process_response(nr_stun_message *req, nr_stun_message *res,
                         Data *password)
{
    int _status;
    nr_stun_message_attribute *attr;

    /* draft-ietf-behave-rfc3489bis-07.txt S 10.1.3 */

    if (res->header.magic_cookie != req->header.magic_cookie) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Rejecting message: Invalid Cookie");
        ABORT(R_REJECTED);
    }

    if (memcmp(&res->header.id, &req->header.id, sizeof(res->header.id))) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Rejecting message: Unrecognized Transaction");
        ABORT(R_REJECTED);
    }

    if (res->header.magic_cookie == NR_STUN_MAGIC_COOKIE && NR_STUN_GET_TYPE_CLASS(res->header.type) == NR_CLASS_RESPONSE) {
        if (!nr_stun_message_has_attribute(res, NR_STUN_ATTR_FINGERPRINT, &attr)) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Rejecting message: Missing Fingerprint");
            ABORT(R_REJECTED);
        }

        if (! attr->u.fingerprint.valid) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Rejecting message: Invalid Fingerprint");
            ABORT(R_REJECTED);
        }
    }

    if (NR_STUN_GET_TYPE_CLASS(res->header.type) != NR_CLASS_ERROR_RESPONSE) {
        if (password != 0
         && nr_stun_message_has_attribute(res, NR_STUN_ATTR_MESSAGE_INTEGRITY, &attr)
         && !attr->u.message_integrity.valid) {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Rejecting message: Bad MessageIntegrity");
            ABORT(R_REJECTED);
        }
    }

    _status=0;
 abort:
    if (_status) {
        r_log(NR_LOG_STUN, LOG_INFO, "Response failure");
    }
    else {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Response accepted");
    }
    return _status;
}

int
nr_stun_request( StunAddress4* dest, int testNum, StunAddress4* sAddr )
{
   int r,_status;
   Data password2;
   int port;
   UINT4 interfaceIp=0;
   nr_stun_message *resp;
   Socket myFd;
   StunAtrString username;
   StunAtrString password;
   char msg[NR_STUN_MAX_MESSAGE_SIZE];
   int msgLen = NR_STUN_MAX_MESSAGE_SIZE;
   StunAddress4 from;
   nr_stun_message_attribute *attr;

   assert( dest->addr != 0 );
   assert( dest->port != 0 );

   if ((r=nr_stun_randomPort(&port)))
     ABORT(r);

   if (sAddr)
   {
      interfaceIp = sAddr->addr;
      if ( sAddr->port != 0 )
      {
        port = sAddr->port;
      }
   }

   myFd = openPort(port,interfaceIp);

   assert(myFd != -1);

   strcpy(username.value,"hello");
   strcpy(password.value,"hello");
   username.sizeValue=5;
   password.sizeValue=5;

   nr_stun_send_test( myFd, dest, &username, &password, testNum);

   getMessage( myFd,
               msg,
               &msgLen,
               &from.addr,
               &from.port);

    r_log(NR_LOG_STUN, LOG_DEBUG, "Got a response");

    if ((r=nr_stun_message_create2(&resp, (UCHAR*)msg, msgLen)))
       ABORT(r);

    password2.data = (UCHAR*)password.value;
    password2.len  = password.sizeValue;

    if ((r=nr_stun_decode_message(resp, returnpassword, &password2))) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Failed to parse message");
        ABORT(r);
    }

    if (nr_stun_message_has_attribute(resp, NR_STUN_ATTR_ERROR_CODE, &attr)) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Received error response");

        if (attr->u.error_code.number == 300) {
            if (!nr_stun_message_has_attribute(resp, NR_STUN_ATTR_ALTERNATE_SERVER, &attr)) {
                r_log(NR_LOG_STUN, LOG_DEBUG, "Missing Alternate Server");
                ABORT(R_FAILED);
            }

            dest->addr = ntohl(attr->u.alternate_server.u.addr4.sin_addr.s_addr);
            dest->port = ntohs(attr->u.alternate_server.u.addr4.sin_port);

            if ((r=nr_stun_request(dest, testNum, sAddr)))
                ABORT(r);
            return 0;
        }
    }

    if ((r=nr_stun_process_response(resp, resp, &password2))) {
        r_log(NR_LOG_STUN, LOG_DEBUG, "Failed to process message");
        ABORT(r);
   }

   if (nr_stun_message_has_attribute(resp, NR_STUN_ATTR_FINGERPRINT, &attr)) {
       r_log(NR_LOG_STUN, LOG_DEBUG, "Validating Fingerprint");
       if (!attr->u.fingerprint.valid) {
           r_log(NR_LOG_STUN, LOG_WARNING, "Fingerprint is bad");
           ABORT(R_FAILED);
       }
   }
   else if (resp->header.magic_cookie != MAGIC_COOKIE
         && resp->header.magic_cookie != MAGIC_COOKIE2) {
       r_log(NR_LOG_STUN, LOG_WARNING, "Missing Magic Cookie");
       ABORT(R_FAILED);
   }

   if (1) {
       if (nr_stun_message_has_attribute(resp, NR_STUN_ATTR_MAPPED_ADDRESS, &attr)) {
           r_log(NR_LOG_STUN, LOG_DEBUG, "     MappedAddr=%s", attr->u.mapped_address.as_string);
       }
       if (nr_stun_message_has_attribute(resp, NR_STUN_ATTR_XOR_MAPPED_ADDRESS, &attr)) {
           r_log(NR_LOG_STUN, LOG_DEBUG, "     XorMappedAddr=%s", attr->u.xor_mapped_address.unmasked.as_string);
       }
   }

   _status=0;
 abort:
   return _status;
}

UINT4
nr_stun_rand()
{
  UINT4 rb;

  nr_crypto_random_bytes((void *)&rb,sizeof(rb));

  return rb;
}

/// return a random number to use as a port
// TODO: obsolete, clean up later
int
nr_stun_randomPort(int *port)
{
   int min=0x4000;
   int max=0x7FFF;
/* TODO: this should grab a port -- instead of assuming that it'll pick
 * a port that isn't already in use -- or better, have openPort do the
 * right thing */
   int ret = nr_stun_rand();
   ret = ret|min;
   ret = ret&max;

   *port = ret;

   return 0;
}

static void
nr_stun_send_test( Socket myFd, StunAddress4* dest,
              StunAtrString* username, StunAtrString* password,
              int testNum)
{
   int r,_status;
   nr_stun_message *msg;
   nr_stun_client_stun_binding_request_params params;
   Data password2;

   assert( dest->addr != 0 );
   assert( dest->port != 0 );

   params.username = username->value;
   password2.data = (UCHAR*)password->value;
   password2.len = password->sizeValue;
   params.password = &password2;

   switch (stun_mode) {
   default:
       switch (auth_rule) {
       case NR_STUN_AUTH_RULE_OPTIONAL:
           if ((r=nr_stun_build_req_no_auth(&params, &msg)))
               ABORT(r);
           break;
       case NR_STUN_AUTH_RULE_SHORT_TERM:
           if ((r=nr_stun_build_req_st_auth(&params, &msg)))
               ABORT(r);
           break;
       case NR_STUN_AUTH_RULE_LONG_TERM:
           if ((r=nr_stun_build_req_lt_auth(&params, &msg)))
               ABORT(r);
           break;
       }
       break;
#ifdef USE_STUND_0_96
   case NR_STUN_MODE_STUND_0_96:
       if ((r=nr_stun_build_req_stund_0_96(0, &msg)))
           ABORT(r);
       break;
#endif /* USE_STUND_0_96 */
   }

   if ((r=nr_stun_encode_message(msg)))
       ABORT(r);

   r_log(NR_LOG_STUN, LOG_DEBUG, "About to send msg of len %d to %s",msg->length,nr_ip4toa(dest));

   sendMessage( myFd, (char*)msg->buffer, msg->length, dest->addr, dest->port);

   _status=0;
 abort:
//   return _status;
   return;
}

int
main(int argc, char* argv[])
{
   int verbose = 0;
   const int maxaddr = 256;
   nr_transport_addr addresses[maxaddr];
   int has_address = 0;
   StunAddress4 address;
   int numAddresses = 0;
   int i;
   int testNum;
   int srcPort = 0;
   
   printf("STUN client version %s\n",NR_STUN_VERSION);
   fflush(stdout);

   nr_crypto_openssl_set();

   nr_app_startup("client",NR_APP_STARTUP_INIT_LOGGING|NR_APP_STARTUP_REGISTRY_LOCAL,&NR_LOG_STUN,0,0);
   NR_reg_set_char("logging.stderr.enabled", 1);
   NR_reg_set_char("logging.syslog.enabled", 1);
   NR_reg_set_string("logging.syslog.facility.client.level", "debug");
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

 
   StunAddress4 stunServerAddr;
   stunServerAddr.addr=0;

   int arg;
   for ( arg = 1; arg<argc; arg++ )
   {
      if ( !strcmp( argv[arg] , "-v" ) )
      {
         verbose = 1;
      }
      else if ( !strcmp( argv[arg] , "-n" ) )
      {
         auth_rule = NR_STUN_AUTH_RULE_OPTIONAL;
      }
      else if ( !strcmp( argv[arg] , "-s" ) )
      {
         auth_rule = NR_STUN_AUTH_RULE_SHORT_TERM;
      }
      else if ( !strcmp( argv[arg] , "-l" ) )
      {
         auth_rule = NR_STUN_AUTH_RULE_LONG_TERM;
      }
#ifdef USE_STUND_0_96
      else if ( !strcmp( argv[arg] , "-c" ) )
      {
         stun_mode = NR_STUN_MODE_STUND_0_96;
      }
#endif /* USE_STUND_0_96 */
      else if ( !strcmp( argv[arg] , "-p" ) )
      {
         arg++;
         if ( argc <= arg ) 
         {
            usage();
            exit(-1);
         }
         srcPort = strtol( argv[arg], NULL, 10);
      }
      else if ( !strcmp( argv[arg] , "-i" ) )
      {
         arg++;
         if ( argc <= arg ) 
         {
            usage();
            exit(-1);
         }
           if (nr_stun_parse_server_name( argv[arg], &address))
           {
              fprintf(stderr,"%s is not a valid host name \n",argv[arg]);
              fflush(stderr);
              usage();
              exit(-1);
           }
           has_address = 1;
      }
      else    
      {
        char* ptr;
        int t =  strtol( argv[arg], &ptr, 10 );
        if ( *ptr == 0 )
        { 
           // conversion worked
           testNum = t;
           printf("running test number %d\n",testNum);
           fflush(stdout);
        }
        else
        {
           if (nr_stun_parse_server_name( argv[arg], &stunServerAddr))
           {
              fprintf(stderr,"%s is not a valid host name \n",argv[arg]);
              usage();
              exit(-1);
           }
	}	
      }
   }

   if (has_address) {
       if (nr_ip4_port_to_transport_addr(address.addr, srcPort, IPPROTO_UDP, &addresses[0]))
       {
              fprintf(stderr,"bad address\n");
              exit(1);
       }
       numAddresses=1;
   }

   if (numAddresses == 0) {
       if (nr_stun_find_local_addresses(addresses, maxaddr, &numAddresses))
           numAddresses = 0;
   }
   else if ( srcPort == 0 )
   {
      if (nr_stun_randomPort(&srcPort)) {
          fprintf(stderr,"Failed\n");
          exit(1);
      }
   }

   if (numAddresses == 0) {
       address.addr = 0;
       address.port = srcPort;
       nr_stun_request(&stunServerAddr,1,&address);
   }
   else for (i = 0; i < numAddresses; ++i) {
       address.addr = ntohl(addresses[i].u.addr4.sin_addr.s_addr);
       address.port = ntohs(addresses[i].u.addr4.sin_port);
       nr_stun_request(&stunServerAddr,1,&address);
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
