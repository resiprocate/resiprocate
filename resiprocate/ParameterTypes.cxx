#include "sip2/sipstack/ParameterTypes.hxx"
#include "sip2/util/compat.hxx"
#include <iostream>

#undef defineParamType
#define defineParamType(_class, _enum, _name, _type, _RFC_ref_ignored)          \
ParameterTypes::Type                                                            \
_class::getTypeNum() const {return ParameterTypes::_enum;}                      \
_class::_class()                                                                \
{                                                                               \
   ParameterTypes::ParameterFactories[ParameterTypes::_enum] = Type::decode;    \
   ParameterTypes::ParameterNames[ParameterTypes::_enum] = _name;               \
}                                                                               \
_class Vocal2::p_##_enum

int strncasecmp(char*,char*,int);

using namespace std;

using namespace Vocal2;

ParameterTypes::Factory ParameterTypes::ParameterFactories[ParameterTypes::MAX_PARAMETER] = {0};
Data ParameterTypes::ParameterNames[ParameterTypes::MAX_PARAMETER] = {"PARAMETER?"};

   defineParamType(Transport_Param, transport, "transport", DataParameter, "RFC ????");
   defineParamType(User_Param, user, "user", DataParameter, "RFC ????");
   defineParamType(Method_Param, method, "method", DataParameter, "RFC ????");
   defineParamType(Ttl_Param, ttl, "ttl", IntegerParameter, "RFC ????");
   defineParamType(Maddr_Param, maddr, "maddr", DataParameter, "RFC ????");
   defineParamType(Lr_Param, lr, "lr", ExistsParameter, "RFC ????");
   defineParamType(Q_Param, q, "q", FloatParameter, "RFC ????");
   defineParamType(Purpose_Param, purpose, "purpose", DataParameter, "RFC ????");
   defineParamType(Handling_Param, handling, "handling", DataParameter, "RFC ????");
   defineParamType(Expires_Param, expires, "expires", IntegerParameter, "RFC ????");
   defineParamType(Tag_Param, tag, "tag", DataParameter, "RFC ????");
   defineParamType(ToTag_Param, toTag, "to-tag", DataParameter, "RFC ????");
   defineParamType(FromTag_Param, fromTag, "from-tag", DataParameter, "RFC ????");
   defineParamType(Duration_Param, duration, "duration", IntegerParameter, "RFC ????");
   defineParamType(Branch_Param, branch, "branch", BranchParameter, "RFC ????");
   defineParamType(Rport_Param, rport, "rport", RportParameter, "RFC ????");
   defineParamType(Received_Param, received, "received", DataParameter, "RFC ????");
   defineParamType(Mobility_Param, mobility, "mobility", DataParameter, "RFC ????");
   defineParamType(Comp_Param, comp, "comp", DataParameter, "RFC ????");
   defineParamType(Id_Param, id, "id", DataParameter, "RFC ????");
   defineParamType(Reason_Param, reason, "reason", DataParameter, "RFC ????");
   defineParamType(Retry_After_Param, retryAfter, "retry-after", IntegerParameter, "RFC ????");

   defineParamType(Algorithm_Param, algorithm, "algorithm", DataParameter, "RFC ????");
   defineParamType(Cnonce_Param, cnonce, "cnonce", QuotedDataParameter, "RFC ????");
   defineParamType(Nonce_Param, nonce, "nonce", QuotedDataParameter, "RFC ????");
   defineParamType(Domain_Param, domain, "domain", QuotedDataParameter, "RFC ????");
   defineParamType(Nc_Param, nc, "nc", DataParameter, "RFC ????");
   defineParamType(Opaque_Param, opaque, "opaque", QuotedDataParameter, "RFC ????");
   defineParamType(Realm_Param, realm, "realm", QuotedDataParameter, "RFC ????");
   defineParamType(Username_Param, username, "username", DataParameter, "RFC ????");

   defineParamType(Response_Param, response, "response", QuotedDataParameter, "RFC ????");
   defineParamType(Stale_Param, stale, "stale", DataParameter, "RFC ????");
   defineParamType(Uri_Param, uri, "uri", QuotedDataParameter, "RFC ????");

   // peculiar case
ParameterTypes::Type
Qop_Options_Param::getTypeNum() const {return ParameterTypes::qopOptions;}
Qop_Options_Param::Qop_Options_Param()
{
   ParameterTypes::ParameterNames[ParameterTypes::qopOptions] = "qop";
}
Qop_Options_Param Vocal2::p_qopOptions;

ParameterTypes::Type
Qop_Param::getTypeNum() const {return ParameterTypes::qop;}
Qop_Param:: Qop_Param()
{
   ParameterTypes::ParameterNames[ParameterTypes::qop] = "qop";
}
Qop_Param Vocal2::p_qop;

Qop_Factory_Param::Qop_Factory_Param()
{
   ParameterTypes::ParameterFactories[ParameterTypes::qopFactory] = Type::decode;
   ParameterTypes::ParameterNames[ParameterTypes::qopFactory] = "qop";
}
Qop_Factory_Param Vocal2::p_qopFactory;

   defineParamType(Digest_Algorithm_Param, dAlg, "d-alg", DataParameter, "rfc 3329");
   defineParamType(Digest_Qop_Param, dQop, "d-qop", DataParameter, "RFC ????");
   defineParamType(Digest_Verify_Param, dVer, "d-ver", QuotedDataParameter, "RFC ????");

   defineParamType(Smime_Type_Param, smimeType, "smime-type", DataParameter, "RFC 2633");
   defineParamType(Name_Param, name, "name", DataParameter, "RFC 2046");
   defineParamType(Filename_Param, filename, "filename", DataParameter, "RFC ????");
   defineParamType(Protocol_Param, protocol, "protocol", DataParameter, "RFC 1847");
   defineParamType(Micalg_Param, micalg, "micalg", DataParameter, "RFC 1847");
   defineParamType(Boundary_Param, boundary, "boundary", DataParameter, "RFC 2046");
   defineParamType(Expiration_Param, expiration, "expiration", IntegerParameter, "RFC 2046");
   defineParamType(Size_Param, size, "size", DataParameter, "RFC 2046");
   defineParamType(Permission_Param, permission, "permission", DataParameter, "RFC 2046");
   defineParamType(Site_Param, site, "site", DataParameter, "RFC 2046");
   defineParamType(Directory_Param, directory, "directory", DataParameter, "RFC 2046");
   defineParamType(Mode_Param, mode, "mode", DataParameter, "RFC 2046");
   defineParamType(Server_Param, server, "server", DataParameter, "RFC 2046");
   defineParamType(Charset_Param, charset, "charset", DataParameter, "RFC 2045");
   defineParamType(Access_Type_Param, accessType, "access-type", DataParameter, "RFC 2046");

/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -g -Z ParamHash -E -L C++ -t -k '*' -D parameters.gperf  */
struct params { char *name; ParameterTypes::Type type; };
/* maximum key range = 141, duplicates = 1 */

class ParamHash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static struct params *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
ParamHash::hash (register const char *str, register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145,   0, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145,   0,  25,  15,
       20,   0,  35,  15,   5,   0, 145, 145,  45,  56,
        0,   0,   0,  56,   0,   0,   0,  26,  20, 145,
        5,   0,  10, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
      145, 145, 145, 145, 145, 145
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

struct params *
ParamHash::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 53,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 144
    };

  static struct params wordlist[] =
    {
      {"site", ParameterTypes::site},
      {"rport", ParameterTypes::rport},
      {"reason", ParameterTypes::reason},
      {"response", ParameterTypes::response},
      {"transport", ParameterTypes::transport},
      {"expires", ParameterTypes::expires},
      {"size", ParameterTypes::size},
      {"expiration", ParameterTypes::expiration},
      {"nc", ParameterTypes::nc},
      {"tag", ParameterTypes::tag},
      {"nonce", ParameterTypes::nonce},
      {"to-tag", ParameterTypes::toTag},
      {"id", ParameterTypes::id},
      {"server", ParameterTypes::server},
      {"charset", ParameterTypes::charset},
      {"uri", ParameterTypes::uri},
      {"user", ParameterTypes::user},
      {"purpose", ParameterTypes::purpose},
      {"cnonce", ParameterTypes::cnonce},
      {"access-type", ParameterTypes::accessType},
      {"directory", ParameterTypes::directory},
      {"d-ver", ParameterTypes::dVer},
      {"retry-after", ParameterTypes::retryAfter},
      {"lr", ParameterTypes::lr},
      {"ttl", ParameterTypes::ttl},
      {"stale", ParameterTypes::stale},
      {"branch", ParameterTypes::branch},
      {"duration", ParameterTypes::duration},
      {"q", ParameterTypes::q},
      {"qop", ParameterTypes::qopFactory},
      {"name", ParameterTypes::name},
      {"name", ParameterTypes::name},
      {"received", ParameterTypes::received},
      {"permission", ParameterTypes::permission},
      {"protocol", ParameterTypes::protocol},
      {"comp", ParameterTypes::comp},
      {"boundary", ParameterTypes::boundary},
      {"mode", ParameterTypes::mode},
      {"d-qop", ParameterTypes::dQop},
      {"domain", ParameterTypes::domain},
      {"d-alg", ParameterTypes::dAlg},
      {"method", ParameterTypes::method},
      {"opaque", ParameterTypes::opaque},
      {"username", ParameterTypes::username},
      {"handling", ParameterTypes::handling},
      {"maddr", ParameterTypes::maddr},
      {"realm", ParameterTypes::realm},
      {"from-tag", ParameterTypes::fromTag},
      {"smime-type", ParameterTypes::smimeType},
      {"algorithm", ParameterTypes::algorithm},
      {"mobility", ParameterTypes::mobility},
      {"micalg", ParameterTypes::micalg},
      {"filename", ParameterTypes::filename}
    };

  static signed char lookup[] =
    {
        -1,   -1,   -1,   -1,    0,    1,    2,   -1,
         3,    4,   -1,   -1,    5,   -1,    6,    7,
        -1,    8,    9,   -1,   10,   11,   12,   -1,
        -1,   -1,   13,   14,   -1,   15,   16,   -1,
        -1,   17,   -1,   -1,   18,   -1,   -1,   -1,
        -1,   19,   -1,   -1,   20,   21,   22,   23,
        24,   -1,   25,   26,   -1,   -1,   27,   -1,
        -1,   28,   -1,   29, -115,  -23,   -2,   32,
        -1,   -1,   33,   -1,   34,   -1,   -1,   -1,
        -1,   -1,   -1,   35,   -1,   -1,   -1,   36,
        37,   38,   39,   -1,   -1,   40,   -1,   41,
        42,   -1,   43,   -1,   -1,   44,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   45,   -1,   -1,
        -1,   -1,   46,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   47,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   48,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   49,   -1,   -1,   -1,   50,   -1,
        -1,   51,   -1,   -1,   -1,   -1,   -1,   -1,
        52
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
                return &wordlist[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register struct params *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct params *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (tolower(*str) == *s && !strncasecmp (str + 1, s + 1, len-1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}

ParameterTypes::Type
ParameterTypes::getType(const char* name, unsigned int len)
{
   struct params* p;
   p = ParamHash::in_word_set(name, len);
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
