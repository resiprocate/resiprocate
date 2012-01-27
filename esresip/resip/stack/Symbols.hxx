/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_SYMBOLS_HXX)
#define RESIP_SYMBOLS_HXX 

#include "rutil/Data.hxx"
namespace resip
{
#ifdef CRLF
#error Looks like you are including something that defines a CRLF macro, which \
will break this header file. (Apache is one such offender, but there are \
others out there) You should make sure that this file (Symbols.hxx) is \
included before this macro is defined (Or, if you are the one that defined \
this macro, stop defining it! This is macro abuse.)
#endif

/** @brief  This class provides a single place to store commonly-used string constants.
  * 
  **/
class Symbols
{
   public:
      static const Data DefaultSipVersion;
      static const Data DefaultSipScheme;

      static const Data AT_SIGN;
      static const Data COLON;
      static const Data BAR;
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
      static const char* LS_BRACKET;
      static const char* RS_BRACKET;
      static const Data PERIOD;
      static const Data QUESTION;
      static const Data AMPERSAND;
      static const Data PERCENT;
      static const Data STAR;
      static const Data UNDERSCORE;

      static const Data ProtocolName;
      static const Data ProtocolVersion;
      static const Data UDP;
      static const Data TCP;
      static const Data TLS;
      static const Data DTLS;
      static const Data SCTP;
      static const Data SRVUDP;
      static const Data SRVTCP;
      static const Data SRVTLS;
      static const Data SRVSCTP;
      
      static const Data Sip;
      static const Data Sips;
      static const Data Tel;
      static const Data Pres;
      
      static const Data Phone;
      static const Data Isub;
      static const Data Postd;

      static const Data auth;
      static const Data authInt;
      static const Data Digest;
      static const Data Basic;
      
      static const Data MagicCookie;
      static const Data resipCookie;

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
      static const char* RTP_SAVP;
      static const char* UDP_TLS_RTP_SAVP;

      static const Data Presence;
      static const Data Required;
      static const Data Optional;

      static const Data C100rel;
      static const Data Replaces;
      static const Data Timer;
      static const Data NoReferSub;
      static const Data AnswerMode;
      static const Data TargetDialog;
      static const Data Undefined;

      static const Data Pending;
      static const Data Active;
      static const Data Terminated;

      static const Data Certificate; // from draft-ietf-certs
      static const Data Credential; // from draft-ietf-certs
      static const Data SipProfile; //from draft-ietf-sipping-config-framework

      static const Data pathSep;
      static const Data id;

      static const Data Gruu;
};

}

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
