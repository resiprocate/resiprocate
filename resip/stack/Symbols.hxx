#if !defined(RESIP_SYMBOLS_HXX)
#define RESIP_SYMBOLS_HXX 


namespace resip
{

class Symbols
{
   public:
      static const char* DefaultSipVersion;
      static const char* DefaultSipScheme;

      static const char* AT_SIGN;
      static const char* COLON;
      static const char* BAR;
      static const char* DASH;
      static const char* DASHDASH;
      static const char* DOT;
      static const char* COMMA_OR_EQUAL;
      static const char* CRLF;
      static const char* CRLFCRLF;
      static const char* CR;
      static const char* LF;
      static const char* TAB;
      static const char* DOUBLE_QUOTE;
      static const char* LA_QUOTE;
      static const char* RA_QUOTE;
      static const char* EQUALS;
      static const char* SEMI_COLON;
      static const char* SEMI_OR_EQUAL;
      static const char* SLASH;
      static const char* B_SLASH;
      static const char* SPACE;
      static const char* COMMA;
      static const char* ZERO;
      static const char* LPAREN;
      static const char* RPAREN;
      static const char* LS_BRACKET;
      static const char* RS_BRACKET;
      static const char* PERIOD;
      static const char* QUESTION;
      static const char* AMPERSAND;
      static const char* PERCENT;
      static const char* STAR;
      static const char* UNDERSCORE;

      static const char* ProtocolName;
      static const char* ProtocolVersion;
      static const char* UDP;
      static const char* TCP;
      static const char* TLS;
      static const char* DTLS;
      static const char* SCTP;
      static const char* WS;
      static const char* WSS;
      static const char* SRVUDP;
      static const char* SRVTCP;
      static const char* SRVTLS;
      static const char* SRVSCTP;
      
      static const char* Sip;
      static const char* Sips;
      static const char* Tel;
      static const char* Pres;
      
      static const char* Phone;
      static const char* Isub;
      static const char* Postd;

      static const char* auth;
      static const char* authInt;
      static const char* Digest;
      static const char* Basic;
      
      static const char * const MagicCookie;
      static const char * const resipCookie;
      static const char * const WebsocketMagicGUID;

      static const int DefaultSipPort;
      static const int SipTlsPort;
      static const int SipWsPort;
      static const int SipWssPort;
      static const int DefaultSipsPort;

      static const char* SrvSip;
      static const char* SrvSips;
      static const char* SrvUdp;
      static const char* SrvTcp;
      static const char* NaptrSip;
      static const char* NaptrSips;
      static const char* NaptrUdp;
      static const char* NaptrTcp;

      static const char* audio;
      static const char* RTP_AVP;
      static const char* RTP_AVPF;
      static const char* RTP_SAVP;
      static const char* RTP_SAVPF;
      static const char* UDP_TLS_RTP_SAVP;

      static const char* Presence;
      static const char* Required;
      static const char* Optional;

      static const char* C100rel;
      static const char* Replaces;
      static const char* Timer;
      static const char* NoReferSub;
      static const char* AnswerMode;
      static const char* TargetDialog;
      static const char* Path;
      static const char* Outbound;
      static const char* Undefined;

      static const char* Pending;
      static const char* Active;
      static const char* Terminated;

      static const char* Certificate; // from draft-ietf-certs
      static const char* Credential; // from draft-ietf-certs
      static const char* SipProfile; //from draft-ietf-sipping-config-framework

      static const char* pathSep;
      static const char* id;

      static const char* Gruu;

      static const char* Uui; // from draft-ietf-cuss-sip-uui-17
      static const char* Hex; // from draft-ietf-cuss-sip-uui-17
      static const char* IsdnInterwork; // from draft-johnston-sipping-cc-uui-09
      static const char* IsdnUui; // from draft-johnston-sipping-cc-uui-09
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
