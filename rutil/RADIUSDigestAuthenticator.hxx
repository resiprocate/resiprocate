
#ifndef __RADIUSDigestAuthenticator_h
#define __RADIUSDigestAuthenticator_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "compat.hxx"

#ifdef USE_RADIUS_CLIENT

#ifdef RESIP_HAVE_RADCLI
#include <radcli/radcli.h>
typedef UInt32 UINT4;
#else
#ifdef RESIP_HAVE_FREERADIUS_CLIENT
#include <freeradius-client.h>
typedef UInt32 UINT4;
#else
#include <radiusclient-ng.h>
#endif
#endif

#include "rutil/Data.hxx"
#include "rutil/ThreadIf.hxx"

#define RADIUS_CONFIG "/etc/radiusclient/radiusclient.conf"
#define RADIUS_MSG_SIZE 4096
#define RADIUS_SIP_PORT 5060

/* 

  Class for performing RADIUS authentication of SIP users

  Based largely on the auth_radius module in SER - http://iptel.org/ser

  Permission has been given by Jan Janak, the author of auth_radius, for
  this code to be redistributed under a BSD-like license.

  see http://www.iptel.org/ietf/aaa/draft-schulzrinne-sipping-radius-accounting-00.txt */

/*
 * WARNING: Don't forget to update the dictionary if you update this file !!!
 */

namespace resip
{

struct attr
{
   const char *n;
   int v;
};

struct val
{
   const char *n;
   int v;
};

#define A_USER_NAME                     0
#define A_SERVICE_TYPE                  1
#define A_CALLED_STATION_ID             2
#define A_CALLING_STATION_ID            3
#define A_ACCT_STATUS_TYPE              4
#define A_ACCT_SESSION_ID               5
#define A_SIP_METHOD                    6
#define A_SIP_RESPONSE_CODE             7
#define A_SIP_CSEQ                      8
#define A_SIP_TO_TAG                    9
#define A_SIP_FROM_TAG                  10
#define A_SIP_TRANSLATED_REQUEST_URI    11
#define A_DIGEST_RESPONSE               12
#define A_DIGEST_ATTRIBUTES             13
#define A_SIP_URI_USER                  14
#define A_SIP_RPID                      15
#define A_DIGEST_REALM                  16
#define A_DIGEST_NONCE                  17
#define A_DIGEST_METHOD                 18
#define A_DIGEST_URI                    19
#define A_DIGEST_QOP                    20
#define A_DIGEST_ALGORITHM              21
#define A_DIGEST_BODY_DIGEST            22
#define A_DIGEST_CNONCE                 23
#define A_DIGEST_NONCE_COUNT            24
#define A_DIGEST_USER_NAME              25
#define A_SIP_GROUP                     26
#define A_CISCO_AVPAIR                  27
#define A_VM_EMAIL                      28
#define A_VM_LANGUAGE                   29
#define A_MAX                           30

#define V_STATUS_START                  0
#define V_STATUS_STOP                   1
#define V_STATUS_FAILED                 2
#define V_CALL_CHECK                    3
#define V_EMERGENCY_CALL                4
#define V_SIP_SESSION                   5
#define V_GROUP_CHECK                   6
#define V_VM_INFO                       7
#define V_MAX                           8


// An instance of this class is notified when the RADIUSDigestAuthenticator
// has done it's work
class RADIUSDigestAuthListener
{
   public:
      virtual ~RADIUSDigestAuthListener();
      // These methods will be called from a separate thread of execution
      virtual void onSuccess(const resip::Data& rpid) = 0;
      virtual void onAccessDenied() = 0;
      virtual void onError() = 0;
};

class TestRADIUSDigestAuthListener : public RADIUSDigestAuthListener
{
   public:
      TestRADIUSDigestAuthListener();
      void onSuccess(const resip::Data& rpid);
      void onAccessDenied();
      void onError();
};

// An instance of this class will attempt to authenticate the request
// using RADIUS
class RADIUSDigestAuthenticator : public resip::ThreadIf {
   //class RADIUSDigestAuthenticator : public ost::Thread {

   private:

      resip::Data username;	// username from ProxyAuth header
      resip::Data digestUsername; // username from digest header
      resip::Data digestRealm; // realm from digest header
      resip::Data digestNonce; // nonce from digest header
      resip::Data digestUri; // request URI from request line
      resip::Data digestMethod; // request method (e.g. INVITE)
      resip::Data digestQop; // QoP is one of "", "auth", "auth-int"
      resip::Data digestNonceCount; // nonce count or ""
      resip::Data digestCNonce; // cnonce or ""
      resip::Data digestBodyDigest; // value of opaque or ""
      resip::Data digestResponse; // digest string submitted by client

      RADIUSDigestAuthListener *listener;

   public:

      static void init(const char *radiusConfigFile);
   
      // No QoP
      RADIUSDigestAuthenticator(const resip::Data& username, 
                                const resip::Data& digestUsername,
                                const resip::Data& digestRealm,
                                const resip::Data& digestNonce,
                                const resip::Data& digestUri,
                                const resip::Data& digestMethod,
                                const resip::Data& digestResponse, 
                                RADIUSDigestAuthListener *listener);
   
      // QoP auth
      RADIUSDigestAuthenticator(const resip::Data& username,
                                const resip::Data& digestUsername,
                                const resip::Data& digestRealm,
                                const resip::Data& digestNonce,
                                const resip::Data& digestUri,
                                const resip::Data& digestMethod,
                                const resip::Data& digestQop,
                                const resip::Data& digestNonceCount,
                                const resip::Data& digestCNonce,
                                const resip::Data& digestResponse,
                                RADIUSDigestAuthListener *listener);
   
      // QoP auth-int
      RADIUSDigestAuthenticator(const resip::Data& username,
                                const resip::Data& digestUsername,
                                const resip::Data& digestRealm,
                                const resip::Data& digestNonce,
                                const resip::Data& digestUri,
                                const resip::Data& digestMethod,
                                const resip::Data& digestQop,
                                const resip::Data& digestNonceCount,
                                const resip::Data& digestCNonce,
                                const resip::Data& digestBodyDigest,
                                const resip::Data& digestResponse,
                                RADIUSDigestAuthListener *listener); 
   
      virtual ~RADIUSDigestAuthenticator();
   
      int doRADIUSCheck();

   protected:

      static struct attr *attrs;
      static struct val *vals;
      static rc_handle *rh;
   
      void thread();
      void final();
   
      VALUE_PAIR *createRADIUSRequest();

};


}

#endif

#endif


/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

