#include "resiprocate/Symbols.hxx"

using namespace Vocal2;

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
const Data Symbols::PERIOD = ".";
const Data Symbols::QUESTION = "?";
const Data Symbols::AMPERSAND = "&";
const Data Symbols::PERCENT = "%";
const Data Symbols::STAR ="*";

const Data Symbols::ProtocolName = "SIP";
const Data Symbols::ProtocolVersion = "2.0";
const Data Symbols::UDP = "UDP";
const Data Symbols::TCP = "TCP";
const Data Symbols::TLS = "TLS";
const Data Symbols::SCTP = "SCTP";

const Data Symbols::Sip = "sip";
const Data Symbols::Sips = "sips";
const Data Symbols::Tel = "tel";

const Data Symbols::Phone = "phone";
const Data Symbols::Isub = "isub=";
const Data Symbols::Postd = "postd=";

const Data Symbols::auth = "auth";

const char * const Symbols::MagicCookie = "z9hG4bK";
const char * const Symbols::Vocal2Cookie= "-c87542-";

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
