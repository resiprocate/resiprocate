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
      static const Data DASHDASH;
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

      static const char * const Accept;
      static const char * const Accept_Encoding;
      static const char * const Accept_Language;
      static const char * const Alert_Info;
      static const char * const Allow;
      static const char * const Authentication_Info;
      static const char * const Authorization;
      static const char * const CSeq;
      static const char * const Call_ID;
      static const char * const Call_Info;
      static const char * const Contact;
      static const char * const Content_Disposition;
      static const char * const Content_Encoding;
      static const char * const Content_Language;
      static const char * const Content_Length;
      static const char * const Content_Type;
      static const char * const Date;
      static const char * const Error_Info;
      static const char * const Expires;
      static const char * const From;
      static const char * const In_Reply_To;
      static const char * const MIME_Version;
      static const char * const Max_Forwards;
      static const char * const Min_Expires;
      static const char * const Organization;
      static const char * const Priority;
      static const char * const Proxy_Authenticate;
      static const char * const Proxy_Authorization;
      static const char * const Proxy_Require;
      static const char * const Record_Route;
      static const char * const Reply_To;
      static const char * const Retry_After;
      static const char * const Require;
      static const char * const Route;
      static const char * const Security_Client;
      static const char * const Security_Server;
      static const char * const Security_Verify;
      static const char * const Server;
      static const char * const Subject;
      static const char * const Subscription_State;
      static const char * const Supported;
      static const char * const Timestamp;
      static const char * const To;
      static const char * const Unsupported;
      static const char * const User_Agent;
      static const char * const Via;
      static const char * const WWW_Authenticate;
      static const char * const Warning;

      static const char * const Ack;
      static const char * const Bye;
      static const char * const Cancel;
      static const char * const Invite;
      static const char * const Notify;
      static const char * const Options;
      static const char * const Refer;
      static const char * const Refer_To;
      static const char * const Referred_By;
      static const char * const Replaces;
      static const char * const Register;
      static const char * const Subscribe;
      static const char * const Event;
      static const char * const Allow_Events;
      
      static const char * const transport;
      static const char * const user;
      static const char * const method;
      static const char * const ttl;
      static const char * const maddr;
      static const char * const lr;
      static const char * const q;
      static const char * const purpose;
      static const char * const expires;
      static const char * const handling;
      static const char * const tag;
      static const char * const toTag;
      static const char * const fromTag;
      static const char * const duration;
      static const char * const branch;
      static const char * const received;
      static const char * const mobility;
      static const char * const comp;
      static const char * const rport;
      static const char * const MagicCookie;
      static const char * const Vocal2Cookie;

      static const char * const algorithm;
      static const char * const cnonce;
      static const char * const domain;
      static const char * const nonce;
      static const char * const nc;
      static const char * const opaque;
      static const char * const realm;
      static const char * const response;
      static const char * const stale;
      static const char * const username;
      static const char * const qop;
      static const char * const uri;
      
      static const char * const id;
      static const char * const retryAfter;
      static const char * const reason;

      static const char * const auth;

      static const char * const dAlg;
      static const char * const dQop;
      static const char * const dVer;

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
