#ifndef Symbols_hxx
#define Symbols_hxx

#include "resiprocate/util/Data.hxx"

namespace Vocal2
{

class Symbols
{
   public:
      static const Data DefaultSipVersion;
      static const Data DefaultSipScheme;

      static const Data AT_SIGN;
      static const Data COLON;
      static const Data DASH;
      static const Data DASHDASH;
      static const Data DOT;
      static const Data COMMA_OR_EQUAL;
      static const Data CRLF;
      static const Data CRLFCRLF;
      static const Data CR;
      static const Data LF;
      static const Data TAB;
      static const Data DOUBLE_QUOTE;
      static const Data LA_QUOTE;
      static const Data RA_QUOTE;
      static const Data EQUALS;
      static const Data SEMI_COLON;
      static const Data SEMI_OR_EQUAL;
      static const Data SLASH;
      static const Data B_SLASH;
      static const Data SPACE;
      static const Data COMMA;
      static const Data ZERO;
      static const Data LPAREN;
      static const Data RPAREN;
      static const Data PERIOD;
      static const Data QUESTION;
      static const Data AMPERSAND;
      static const Data PERCENT;
      static const Data STAR;

      static const Data ProtocolName;
      static const Data ProtocolVersion;
      static const Data UDP;
      static const Data TCP;
      static const Data TLS;
      static const Data SCTP;
      
      static const Data Sip;
      static const Data Sips;
      static const Data Tel;
      
      static const Data Phone;
      static const Data Isub;
      static const Data Postd;

      static const Data auth;

      static const char * const MagicCookie;
      static const char * const Vocal2Cookie;

      static const int DefaultSipPort;
      static const int SipTlsPort;
      static const int DefaultSipsPort;

      static const Data SrvSip;
      static const Data SrvSips;
      static const Data SrvUdp;
      static const Data SrvTcp;
      static const Data NaptrSip;
      static const Data NaptrSips;
      static const Data NaptrUdp;
      static const Data NaptrTcp;

      static const Data audio;
      static const Data RTP_AVP;
};

}

#endif


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
