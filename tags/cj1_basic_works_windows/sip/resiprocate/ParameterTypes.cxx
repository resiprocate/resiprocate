#include "sip2/sipstack/ParameterTypes.hxx"
#include "sip2/util/compat.hxx"
#include <iostream>

int strncasecmp(char*,char*,int);

using namespace std;

using namespace Vocal2;

ParameterTypes::Factory ParameterTypes::ParameterFactories[ParameterTypes::MAX_PARAMETER] = {0};
Data ParameterTypes::ParameterNames[ParameterTypes::MAX_PARAMETER] = {"PARAMETER?"};

Transport_Param Vocal2::p_transport;
User_Param Vocal2::p_user;
Method_Param Vocal2::p_method;
Ttl_Param Vocal2::p_ttl;
Maddr_Param Vocal2::p_maddr;
Lr_Param Vocal2::p_lr;
Q_Param Vocal2::p_q;
Purpose_Param Vocal2::p_purpose;
Expires_Param Vocal2::p_expires;
Handling_Param Vocal2::p_handling;
Tag_Param Vocal2::p_tag;
ToTag_Param Vocal2::p_toTag;
FromTag_Param Vocal2::p_fromTag;
Duration_Param Vocal2::p_duration;
Branch_Param Vocal2::p_branch;
Received_Param Vocal2::p_received;
Mobility_Param Vocal2::p_mobility;
Comp_Param Vocal2::p_comp;
Rport_Param Vocal2::p_rport;

Id_Param Vocal2::p_id;
Reason_Param Vocal2::p_reason;
Retry_After_Param Vocal2::p_retryAfter;

Algorithm_Param Vocal2::p_algorithm;
Cnonce_Param Vocal2::p_cnonce;
Nonce_Param Vocal2::p_nonce;
Domain_Param Vocal2::p_domain;
Nc_Param Vocal2::p_nc;
Opaque_Param Vocal2::p_opaque;
Realm_Param Vocal2::p_realm;
Response_Param Vocal2::p_response;
Stale_Param Vocal2::p_stale;
Username_Param Vocal2::p_username;
Uri_Param Vocal2::p_uri;
Qop_Options_Param Vocal2::p_qopOptions;
Qop_Param Vocal2::p_qop;
Qop_Factory_Param Vocal2::p_qopFactory;

// to generate the perfect hash:
// call tolower() on instances of the source string
// change strcmp to strncasecmp and pass len-1
// will NOT work for non alphanum chars 
/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -t -k '*' parameters.gperf  */
struct params { char *name; ParameterTypes::Type type; };

#define TOTAL_KEYWORDS 34
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 11
#define MIN_HASH_VALUE 5
#define MAX_HASH_VALUE 108
/* maximum key range = 104, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
p_hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109,   0, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109,   0,  25,  15,
       20,   0,  35,  10,   5,   0, 109, 109,  45,  30,
        0,   0,   0,  40,   0,   0,   0,  30,  20, 109,
        5,   0, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 11:
        hval += asso_values[(unsigned char)tolower(str[10])];
      case 10:
        hval += asso_values[(unsigned char)tolower(str[9])];
      case 9:
        hval += asso_values[(unsigned char)tolower(str[8])];
      case 8:
        hval += asso_values[(unsigned char)tolower(str[7])];
      case 7:
        hval += asso_values[(unsigned char)tolower(str[6])];
      case 6:
        hval += asso_values[(unsigned char)tolower(str[5])];
      case 5:
        hval += asso_values[(unsigned char)tolower(str[4])];
      case 4:
        hval += asso_values[(unsigned char)tolower(str[3])];
      case 3:
        hval += asso_values[(unsigned char)tolower(str[2])];
      case 2:
        hval += asso_values[(unsigned char)tolower(str[1])];
      case 1:
        hval += asso_values[(unsigned char)tolower(str[0])];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
struct params *
p_in_word_set (register const char *str, register unsigned int len)
{
  static struct params wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""},
      {"rport", ParameterTypes::rport},
      {"reason", ParameterTypes::reason},
      {""},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {""}, {""},
      {"expires", ParameterTypes::expires},
      {"tag", ParameterTypes::tag},
      {""}, {""},
      {"to-tag", ParameterTypes::toTag},
      {"nc", ParameterTypes::nc},
      {""}, {""},
      {"nonce", ParameterTypes::nonce},
      {""},
      {"id", ParameterTypes::id},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {""},
      {"cnonce", ParameterTypes::cnonce},
      {"purpose", ParameterTypes::purpose},
      {""}, {""}, {""},
      {"q", ParameterTypes::q},
      {""},
      {"qop", ParameterTypes::qopFactory},
      {""}, {""},
      {"retry-after", ParameterTypes::retryAfter},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"comp", ParameterTypes::comp},
      {"stale", ParameterTypes::stale},
      {"branch", ParameterTypes::branch},
      {""}, {""}, {""}, {""},
      {"domain", ParameterTypes::domain},
      {""},
      {"duration", ParameterTypes::duration},
      {""}, {""},
      {"method", ParameterTypes::method},
      {""},
      {"received", ParameterTypes::received},
      {""}, {""}, {""}, {""},
      {"username", ParameterTypes::username},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"maddr", ParameterTypes::maddr},
      {"opaque", ParameterTypes::opaque},
      {""}, {""}, {""},
      {"realm", ParameterTypes::realm},
      {""}, {""},
      {"from-tag", ParameterTypes::fromTag},
      {""}, {""}, {""}, {""},
      {"handling", ParameterTypes::handling},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"algorithm", ParameterTypes::algorithm},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"mobility", ParameterTypes::mobility}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = p_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
            return &wordlist[key];
        }
    }
  return 0;
}


ParameterTypes::Type
ParameterTypes::getType(const char* name, unsigned int len)
{
   struct params* p;
   p = p_in_word_set(name, len);
   return p ? p->type : ParameterTypes::UNKNOWN;
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
