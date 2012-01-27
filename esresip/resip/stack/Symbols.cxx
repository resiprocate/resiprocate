/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Symbols.hxx"

using namespace resip;

const Data Symbols::DefaultSipVersion = "SIP/2.0";
const Data Symbols::DefaultSipScheme = "sip";

const Data Symbols::CRLF = "\r\n";
const Data Symbols::CRLFCRLF = "\r\n\r\n";
const Data Symbols::CR = "\r";
const Data Symbols::LF = "\n";
const Data Symbols::TAB = "\t";

const Data Symbols::AT_SIGN = "@";
const Data Symbols::SPACE = " ";
const Data Symbols::DASH = "-";
const Data Symbols::BAR = "|";
const Data Symbols::DASHDASH = "--";
const Data Symbols::DOT = ".";
const Data Symbols::COLON = ":";
const Data Symbols::EQUALS = "=";
const Data Symbols::SEMI_OR_EQUAL = ";=";
const Data Symbols::COMMA_OR_EQUAL = ",=";
const Data Symbols::SEMI_COLON = ";";
const Data Symbols::SLASH = "/";
const Data Symbols::B_SLASH = "\\";
const Data Symbols::DOUBLE_QUOTE = "\"";
const Data Symbols::LA_QUOTE = "<";
const Data Symbols::RA_QUOTE = ">";
const Data Symbols::COMMA = ",";
const Data Symbols::ZERO = "0";
const Data Symbols::LPAREN = "(";
const Data Symbols::RPAREN = ")";
const char* Symbols::LS_BRACKET = "[";
const char* Symbols::RS_BRACKET = "]";
const Data Symbols::PERIOD = ".";
const Data Symbols::QUESTION = "?";
const Data Symbols::AMPERSAND = "&";
const Data Symbols::PERCENT = "%";
const Data Symbols::STAR ="*";
const Data Symbols::UNDERSCORE ="_";

const Data Symbols::ProtocolName = "SIP";
const Data Symbols::ProtocolVersion = "2.0";
const Data Symbols::UDP = "UDP";
const Data Symbols::TCP = "TCP";
const Data Symbols::TLS = "TLS";
const Data Symbols::DTLS = "DTLS";
const Data Symbols::SCTP = "SCTP";
const Data Symbols::SRVUDP = "_udp.";
const Data Symbols::SRVTCP = "_tcp.";
const Data Symbols::SRVTLS = "_tls.";
const Data Symbols::SRVSCTP = "_sctp.";

const Data Symbols::Sip = "sip";
const Data Symbols::Sips = "sips";
const Data Symbols::Tel = "tel";
const Data Symbols::Pres = "pres";

const Data Symbols::Phone = "phone";
const Data Symbols::Isub = "isub=";
const Data Symbols::Postd = "postd=";

const Data Symbols::auth = "auth";
const Data Symbols::authInt = "auth-int";
const Data Symbols::Digest = "Digest";
const Data Symbols::Basic = "Basic";

const Data Symbols::MagicCookie = "z9hG4bK";
const Data Symbols::resipCookie= "-d8754z-";

const int Symbols::DefaultSipPort = 5060;
const int Symbols::SipTlsPort = 5061;
const int Symbols::DefaultSipsPort = 5061;

const Data Symbols::SrvSip = "_sip";
const Data Symbols::SrvSips = "_sips";
const Data Symbols::SrvUdp = "_udp";
const Data Symbols::SrvTcp = "_tcp";
const Data Symbols::NaptrSip = "SIP";
const Data Symbols::NaptrSips = "SIPS";
const Data Symbols::NaptrUdp = "D2U";
const Data Symbols::NaptrTcp = "D2T";

const Data Symbols::audio = "audio";
const Data Symbols::RTP_AVP = "RTP/AVP";
const char* Symbols::RTP_SAVP = "RTP/SAVP"; // used for SRTP
const char* Symbols::UDP_TLS_RTP_SAVP = "UDP/TLS/RTP/SAVP";  // used for DTLS-SRTP

const Data Symbols::Presence = "presence";
const Data Symbols::Required = "required";
const Data Symbols::Optional = "optional";

const Data Symbols::C100rel      = "100rel";
const Data Symbols::Replaces     = "replaces";
const Data Symbols::Timer        = "timer";
const Data Symbols::NoReferSub   = "norefersub";
const Data Symbols::AnswerMode   = "answermode";
const Data Symbols::TargetDialog = "tdialog";
const Data Symbols::Undefined    = "UNDEFINED";

const Data Symbols::Pending = "pending";
const Data Symbols::Active = "active";
const Data Symbols::Terminated = "terminated";

const Data Symbols::Certificate = "certificate";
const Data Symbols::Credential = "credential";

const Data Symbols::SipProfile = "sip-profile";

const Data Symbols::id = "id"; // from RFC 3323

const Data Symbols::Gruu = "gruu";


#if defined(WIN32)
const Data Symbols::pathSep = "\\";
#else
const Data Symbols::pathSep = "/";
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
