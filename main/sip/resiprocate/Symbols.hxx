#ifndef Symbols_hxx
#define Symbols_hxx

#include "sip2/util/Data.hxx"

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
      
      static const Data ProtocolName;
      static const Data ProtocolVersion;
      static const Data UDP;
      static const Data TCP;
      static const Data TLS;
      static const Data SCTP;
      
      static const Data Sip;
      static const Data Sips;

      static const Data Accept;
      static const Data Accept_Encoding;
      static const Data Accept_Language;
      static const Data Alert_Info;
      static const Data Allow;
      static const Data Authentication_Info;
      static const Data Authorization;
      static const Data CSeq;
      static const Data Call_ID;
      static const Data Call_Info;
      static const Data Contact;
      static const Data Content_Disposition;
      static const Data Content_Encoding;
      static const Data Content_Language;
      static const Data Content_Length;
      static const Data Content_Type;
      static const Data Date;
      static const Data Error_Info;
      static const Data Expires;
      static const Data From;
      static const Data In_Reply_To;
      static const Data MIME_Version;
      static const Data Max_Forwards;
      static const Data Min_Expires;
      static const Data Organization;
      static const Data Priority;
      static const Data Proxy_Authenticate;
      static const Data Proxy_Authorization;
      static const Data Proxy_Require;
      static const Data Record_Route;
      static const Data Reply_To;
      static const Data Retry_After;
      static const Data Require;
      static const Data Route;
      static const Data Server;
      static const Data Subject;
      static const Data Subscription_State;
      static const Data Supported;
      static const Data Timestamp;
      static const Data To;
      static const Data Unsupported;
      static const Data User_Agent;
      static const Data Via;
      static const Data WWW_Authenticate;
      static const Data Warning;

      static const Data Ack;
      static const Data Bye;
      static const Data Cancel;
      static const Data Invite;
      static const Data Notify;
      static const Data Options;
      static const Data Refer;
      static const Data Refer_To;
      static const Data Referred_By;
      static const Data Replaces;
      static const Data Register;
      static const Data Subscribe;
      static const Data Event;
      static const Data Allow_Events;
      
      static const Data transport;
      static const Data user;
      static const Data method;
      static const Data ttl;
      static const Data maddr;
      static const Data lr;
      static const Data q;
      static const Data purpose;
      static const Data expires;
      static const Data handling;
      static const Data tag;
      static const Data toTag;
      static const Data fromTag;
      static const Data duration;
      static const Data branch;
      static const Data received;
      static const Data mobility;
      static const Data comp;
      static const Data rport;
      static const Data MagicCookie;
      static const Data Vocal2Cookie;

      static const Data algorithm;
      static const Data cnonce;
      static const Data domain;
      static const Data nonce;
      static const Data nc;
      static const Data opaque;
      static const Data realm;
      static const Data response;
      static const Data stale;
      static const Data username;
      static const Data qop;
      static const Data uri;
      
      static const Data id;
      static const Data retryAfter;
      static const Data reason;

      static const Data auth;

      static const int DefaultSipPort;
      static const int SipTlsPort;
      static const int DefaultSipsPort;
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
