#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_RADIUS_CLIENT

#include "Logger.hxx"
#include "RADIUSDigestAuthenticator.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

struct attr *RADIUSDigestAuthenticator::attrs = NULL;
struct val *RADIUSDigestAuthenticator::vals = NULL;
rc_handle *RADIUSDigestAuthenticator::rh = NULL;

inline void init_av(rc_handle *rh, struct attr *at, struct val *vl, const char *fn)
{
   int i;
   DICT_ATTR *da;
   DICT_VALUE *dv;

   for (i = 0; i < A_MAX; i++)
   {
      if (at[i].n == NULL)
      continue;
      da = rc_dict_findattr(rh, at[i].n);
      if (da == NULL)
      {
         ErrLog( <<"ERROR: " << Data(fn) << ": can't get code for the " << Data(at[i].n) << " attribute\n");
         throw;
      }
      at[i].v = da->value;
   }
   for (i = 0; i < V_MAX; i++)
   {
      if (vl[i].n == NULL)
      continue;
      dv = rc_dict_findval(rh, vl[i].n);
      if (dv == NULL)
      {
         ErrLog( <<"ERROR: " << fn << ": can't get code for the " << vl[i].n << " attribute value\n");
         throw;
      }
      vl[i].v = dv->value;
   }
}

RADIUSDigestAuthListener::~RADIUSDigestAuthListener() {
}


void RADIUSDigestAuthenticator::init(const char *radiusConfigFile)
{
   if(attrs != NULL)
   {
      WarningLog(<<"invoked more than once, ignoring");
      return;
   }

   if((attrs = (struct attr *)malloc(sizeof(struct attr) * A_MAX)) == NULL)
   {
      ErrLog( <<"malloc failed");
      throw;
   }
   if((vals = (struct val *)malloc(sizeof(struct val) * V_MAX)) == NULL)
   {
      ErrLog(<< "malloc failed");
      throw;
   }

   memset(attrs, 0, sizeof(struct attr) * A_MAX);
   memset(vals, 0, sizeof(struct val) * V_MAX);
   attrs[A_SERVICE_TYPE].n = "Service-Type";
   attrs[A_SIP_RPID].n = "Sip-RPId";
   attrs[A_SIP_URI_USER].n = "Sip-URI-User";
   attrs[A_DIGEST_RESPONSE].n = "Digest-Response";
   attrs[A_DIGEST_ALGORITHM].n = "Digest-Algorithm";
   attrs[A_DIGEST_BODY_DIGEST].n = "Digest-Body-Digest";
   attrs[A_DIGEST_CNONCE].n = "Digest-CNonce";
   attrs[A_DIGEST_NONCE_COUNT].n = "Digest-Nonce-Count";
   attrs[A_DIGEST_QOP].n = "Digest-QOP";
   attrs[A_DIGEST_METHOD].n = "Digest-Method";
   attrs[A_DIGEST_URI].n = "Digest-URI";
   attrs[A_DIGEST_NONCE].n = "Digest-Nonce";
   attrs[A_DIGEST_REALM].n = "Digest-Realm";
   attrs[A_DIGEST_USER_NAME].n = "Digest-User-Name";
   attrs[A_USER_NAME].n = "User-Name";
   attrs[A_CISCO_AVPAIR].n = NULL;
   //attrs[A_CISCO_AVPAIR].n                 = "Cisco-AVPair";
   vals[V_SIP_SESSION].n = "Sip-Session";

   const char *myRADIUSConfigFile = RADIUS_CONFIG;
   if(radiusConfigFile != NULL)
   {
      myRADIUSConfigFile = radiusConfigFile;
   }
   // FIXME: this should not use a cast, freeradius-client.h to be fixed
   // pull request submitted via github
   // https://github.com/FreeRADIUS/freeradius-client/pull/8
   if((rh = rc_read_config((char *)myRADIUSConfigFile)) == NULL)
   {
      ErrLog(<< "radius: Error opening configuration file \n");
      throw;
   }

   if (rc_read_dictionary(rh, rc_conf_str(rh, "dictionary")) != 0)
   {
      ErrLog(<< "radius: Error opening dictionary file \n");
      throw;
   }

   init_av(rh, attrs, vals, "radius");

}

RADIUSDigestAuthenticator::RADIUSDigestAuthenticator(
   const resip::Data& username,
   const resip::Data& digestUsername,
   const resip::Data& digestRealm,
   const resip::Data& digestNonce,
   const resip::Data& digestUri, const resip::Data& digestMethod,
   const resip::Data& digestResponse, RADIUSDigestAuthListener *listener) :
   username(username),
   digestUsername(digestUsername),
   digestRealm(digestRealm),
   digestNonce(digestNonce),
   digestUri(digestUri),
   digestMethod(digestMethod),
   digestQop(""),
   digestNonceCount(""),
   digestCNonce(""),
   digestBodyDigest(""),
   digestResponse(digestResponse),
   listener(listener)
{
}

// QoP auth
RADIUSDigestAuthenticator::RADIUSDigestAuthenticator(
   const resip::Data& username,
   const resip::Data& digestUsername,
   const resip::Data& digestRealm,
   const resip::Data& digestNonce,
   const resip::Data& digestUri,
   const resip::Data& digestMethod,
   const resip::Data& digestQop,
   const resip::Data& digestNonceCount,
   const resip::Data& digestCNonce,
   const resip::Data& digestResponse,
   RADIUSDigestAuthListener *listener) :
   username(username),
   digestUsername(digestUsername),
   digestRealm(digestRealm),
   digestNonce(digestNonce),
   digestUri(digestUri),
   digestMethod(digestMethod),
   digestQop(digestQop),
   digestNonceCount(digestNonceCount),
   digestCNonce(digestCNonce),
   digestBodyDigest(""),
   digestResponse(digestResponse),
   listener(listener)
{
}

// QoP auth-int
RADIUSDigestAuthenticator::RADIUSDigestAuthenticator(
   const resip::Data& username,
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
   RADIUSDigestAuthListener *listener) :
   username(username),
   digestUsername(digestUsername),
   digestRealm(digestRealm),
   digestNonce(digestNonce),
   digestUri(digestUri),
   digestMethod(digestMethod),
   digestQop(digestQop),
   digestNonceCount(digestNonceCount),
   digestCNonce(digestCNonce),
   digestBodyDigest(digestBodyDigest),
   digestResponse(digestResponse),
   listener(listener)
{
}

RADIUSDigestAuthenticator::~RADIUSDigestAuthenticator()
{
   DebugLog(<<"RADIUSDigestAuthenticator::~RADIUSDigestAuthenticator() entered");
   //terminate();
   DebugLog(<<"RADIUSDigestAuthenticator::~RADIUSDigestAuthenticator() done");
}

int RADIUSDigestAuthenticator::doRADIUSCheck()
{
   run();
   detach();
   return 0;
   // return detach();
}

void RADIUSDigestAuthenticator::thread()
{

   DebugLog(<<"RADIUSDigestAuthenticator::thread() entered");

   VALUE_PAIR *vp_s_start = createRADIUSRequest();
   VALUE_PAIR *vp_r_start;

   if(vp_s_start == NULL)
   {
      WarningLog(<<"vp_s_start == NULL");
      listener->onError();
      delete listener;
      //exit();
      delete this;
      return;
   }

   char msg[RADIUS_MSG_SIZE];
   int i;
   if ((i = rc_auth(rh, RADIUS_SIP_PORT, vp_s_start, &vp_r_start, msg)) == OK_RC)
   {
      DebugLog(<<"rc_auth success for " << username.c_str());
      rc_avpair_free(vp_s_start);
      Data rpid("");
      VALUE_PAIR *vp;
      if ((vp = rc_avpair_get(vp_r_start, attrs[A_SIP_RPID].v, 0)))
      {
         rpid = Data(vp->strvalue, vp->lvalue);
      }
      listener->onSuccess(rpid);
      rc_avpair_free(vp_r_start);
   }
   else
   {
      DebugLog(<<"rc_auth failure for " << username.c_str()
               <<", code = " << i);
      rc_avpair_free(vp_s_start);
      rc_avpair_free(vp_r_start);
      if(i == REJECT_RC)
      {
         listener->onAccessDenied();
      }
      else
      {
         listener->onError();
      }
   }
   delete listener;
   DebugLog(<<"RADIUSDigestAuthenticator::thread() exiting");
   //exit();
   delete this;
}

void RADIUSDigestAuthenticator::final()
{
   DebugLog(<<"RADIUSDigestAuthenticator::final() entered");
}

VALUE_PAIR *RADIUSDigestAuthenticator::createRADIUSRequest()
{
   VALUE_PAIR *vp_start = NULL;

   if(!rc_avpair_add(rh, &vp_start, attrs[A_USER_NAME].v, (char *)username.data(), username.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_USER_NAME].v, (char *)digestUsername.data(), digestUsername.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_REALM].v, (char *)digestRealm.data(), digestRealm.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_NONCE].v, (char *)digestNonce.data(), digestNonce.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_URI].v, (char *)digestUri.data(), digestUri.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_METHOD].v, (char *)digestMethod.data(), digestMethod.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }
   if(!digestQop.empty())
   {
      if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_QOP].v, (char *)digestQop.data(), digestQop.size(), 0))
      {
         rc_avpair_free(vp_start);
         return NULL;
      }
      if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_NONCE_COUNT].v, (char *)digestNonceCount.data(), digestNonceCount.size(), 0))
      {
         rc_avpair_free(vp_start);
         return NULL;
      }
      if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_CNONCE].v, (char *)digestCNonce.data(), digestCNonce.size(), 0))
      {
         rc_avpair_free(vp_start);
         return NULL;
      }
      if(!digestBodyDigest.empty())
      {
         if(!rc_avpair_add(rh, &vp_start, attrs[A_USER_NAME].v, (char *)username.data(), username.size(), 0))
         {
            rc_avpair_free(vp_start);
            return NULL;
         }
      }
   } // end QoP

   if(!rc_avpair_add(rh, &vp_start, attrs[A_DIGEST_RESPONSE].v, (char *)digestResponse.data(), digestResponse.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }

   UINT4 service = vals[V_SIP_SESSION].v;
   if (!rc_avpair_add(rh, &vp_start, attrs[A_SERVICE_TYPE].v, &service, -1, 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }

   /* Add SIP URI as a check item */
   // xxx
   if (!rc_avpair_add(rh, &vp_start, attrs[A_SIP_URI_USER].v, (char *)digestUsername.data(), digestUsername.size(), 0))
   {
      rc_avpair_free(vp_start);
      return NULL;
   }

   return vp_start;
}

TestRADIUSDigestAuthListener::TestRADIUSDigestAuthListener()
{
}

void TestRADIUSDigestAuthListener::onSuccess(const resip::Data& rpid)
{
   DebugLog(<<"TestRADIUSDigestAuthListener::onSuccess");
   if(!rpid.empty())
   {
      DebugLog(<<"TestRADIUSDigestAuthListener::onSuccess rpid = " << rpid);
   }
   else
   {
      DebugLog(<<"TestRADIUSDigestAuthListener::onSuccess, no rpid");
   }
}

void TestRADIUSDigestAuthListener::onAccessDenied()
{
   DebugLog(<<"TestRADIUSDigestAuthListener::onAccessDenied");
}

void TestRADIUSDigestAuthListener::onError()
{
   WarningLog(<<"TestRADIUSDigestAuthListener::onError");
}

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

