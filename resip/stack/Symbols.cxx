#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Symbols.hxx"

using namespace resip;

const char* Symbols::DefaultSipVersion = "SIP/2.0";
const char* Symbols::DefaultSipScheme = "sip";

const char* Symbols::CRLF = "\r\n";
const char* Symbols::CRLFCRLF = "\r\n\r\n";
const char* Symbols::CR = "\r";
const char* Symbols::LF = "\n";
const char* Symbols::TAB = "\t";

const char* Symbols::AT_SIGN = "@";
const char* Symbols::SPACE = " ";
const char* Symbols::DASH = "-";
const char* Symbols::BAR = "|";
const char* Symbols::DASHDASH = "--";
const char* Symbols::DOT = ".";
const char* Symbols::COLON = ":";
const char* Symbols::EQUALS = "=";
const char* Symbols::SEMI_OR_EQUAL = ";=";
const char* Symbols::COMMA_OR_EQUAL = ",=";
const char* Symbols::SEMI_COLON = ";";
const char* Symbols::SLASH = "/";
const char* Symbols::B_SLASH = "\\";
const char* Symbols::DOUBLE_QUOTE = "\"";
const char* Symbols::LA_QUOTE = "<";
const char* Symbols::RA_QUOTE = ">";
const char* Symbols::COMMA = ",";
const char* Symbols::ZERO = "0";
const char* Symbols::LPAREN = "(";
const char* Symbols::RPAREN = ")";
const char* Symbols::LS_BRACKET = "[";
const char* Symbols::RS_BRACKET = "]";
const char* Symbols::PERIOD = ".";
const char* Symbols::QUESTION = "?";
const char* Symbols::AMPERSAND = "&";
const char* Symbols::PERCENT = "%";
const char* Symbols::STAR ="*";
const char* Symbols::UNDERSCORE ="_";

const char* Symbols::ProtocolName = "SIP";
const char* Symbols::ProtocolVersion = "2.0";
const char* Symbols::UDP = "UDP";
const char* Symbols::TCP = "TCP";
const char* Symbols::TLS = "TLS";
const char* Symbols::DTLS = "DTLS";
const char* Symbols::SCTP = "SCTP";
const char* Symbols::WS = "WS";
const char* Symbols::WSS = "WSS";
const char* Symbols::SRVUDP = "_udp.";
const char* Symbols::SRVTCP = "_tcp.";
const char* Symbols::SRVTLS = "_tls.";
const char* Symbols::SRVSCTP = "_sctp.";

const char* Symbols::Sip = "sip";
const char* Symbols::Sips = "sips";
const char* Symbols::Tel = "tel";
const char* Symbols::Pres = "pres";

const char* Symbols::Phone = "phone";
const char* Symbols::Isub = "isub=";
const char* Symbols::Postd = "postd=";

const char* Symbols::auth = "auth";
const char* Symbols::authInt = "auth-int";
const char* Symbols::Digest = "Digest";
const char* Symbols::Basic = "Basic";

const char * const Symbols::MagicCookie = "z9hG4bK";
const char * const Symbols::resipCookie= "-524287-";
const char * const Symbols::WebsocketMagicGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

const int Symbols::DefaultSipPort = 5060;
const int Symbols::SipTlsPort = 5061;
const int Symbols::SipWsPort = 80;
const int Symbols::SipWssPort = 443;
const int Symbols::DefaultSipsPort = 5061;

const char* Symbols::SrvSip = "_sip";
const char* Symbols::SrvSips = "_sips";
const char* Symbols::SrvUdp = "_udp";
const char* Symbols::SrvTcp = "_tcp";
const char* Symbols::NaptrSip = "SIP";
const char* Symbols::NaptrSips = "SIPS";
const char* Symbols::NaptrUdp = "D2U";
const char* Symbols::NaptrTcp = "D2T";

const char* Symbols::audio = "audio";
const char* Symbols::RTP_AVP = "RTP/AVP";
const char* Symbols::RTP_AVPF = "RTP/AVPF";
const char* Symbols::RTP_SAVP = "RTP/SAVP"; // used for SRTP
const char* Symbols::RTP_SAVPF = "RTP/SAVPF"; // used for SRTP, usually WebRTC
const char* Symbols::UDP_TLS_RTP_SAVP = "UDP/TLS/RTP/SAVP";  // used for DTLS-SRTP

const char* Symbols::Presence = "presence";
const char* Symbols::Required = "required";
const char* Symbols::Optional = "optional";

const char* Symbols::C100rel      = "100rel";
const char* Symbols::Replaces     = "replaces";
const char* Symbols::Timer        = "timer";
const char* Symbols::NoReferSub   = "norefersub";
const char* Symbols::AnswerMode   = "answermode";
const char* Symbols::TargetDialog = "tdialog";
const char* Symbols::Path         = "path";
const char* Symbols::Outbound     = "outbound";
const char* Symbols::Undefined    = "UNDEFINED";

const char* Symbols::Pending = "pending";
const char* Symbols::Active = "active";
const char* Symbols::Terminated = "terminated";

const char* Symbols::Certificate = "certificate";
const char* Symbols::Credential = "credential";

const char* Symbols::SipProfile = "sip-profile";

const char* Symbols::id = "id"; // from RFC 3323

const char* Symbols::Gruu = "gruu";

const char* Symbols::Uui = "uui";
const char* Symbols::Hex = "hex";
const char* Symbols::IsdnInterwork = "isdn-interwork";
const char* Symbols::IsdnUui = "isdn-uui";

#if defined(WIN32)
const char *Symbols::pathSep = "\\";
#else
const char *Symbols::pathSep = "/";
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
