#include "sip2/sipstack/Symbols.hxx"

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

const Data Symbols::ProtocolName = "SIP";
const Data Symbols::ProtocolVersion = "2.0";
const Data Symbols::UDP = "UDP";
const Data Symbols::TCP = "TCP";
const Data Symbols::TLS = "TLS";
const Data Symbols::SCTP = "SCTP";

const Data Symbols::Sip = "sip";
const Data Symbols::Sips = "sips";

const Data Symbols::Accept = "Accept";
const Data Symbols::Accept_Encoding = "Accept-Encoding";
const Data Symbols::Accept_Language = "Accept-Language";
const Data Symbols::Alert_Info = "Alert-Info";
const Data Symbols::Allow = "Allow";
const Data Symbols::Authentication_Info = "Authentication-Info";
const Data Symbols::Authorization = "Authorization";
const Data Symbols::CSeq = "CSeq";
const Data Symbols::Call_ID = "Call-ID";
const Data Symbols::Call_Info = "Call-Info";
const Data Symbols::Contact = "Contact";
const Data Symbols::Content_Disposition = "Content-Disposition";
const Data Symbols::Content_Encoding = "Content-Encoding";
const Data Symbols::Content_Language = "Content-Language";
const Data Symbols::Content_Length = "Content-Length";
const Data Symbols::Content_Type = "Content-Type";
const Data Symbols::Date = "Date";
const Data Symbols::Error_Info = "Error-Info";
const Data Symbols::Expires = "Expires";
const Data Symbols::From = "From";
const Data Symbols::In_Reply_To = "In-Reply-To";
const Data Symbols::MIME_Version = "MIME-Version";
const Data Symbols::Max_Forwards = "Max-Forwards";
const Data Symbols::Min_Expires = "Min-Expires";
const Data Symbols::Organization = "Organization";
const Data Symbols::Priority = "Priority";
const Data Symbols::Proxy_Authenticate = "Proxy-Authenticate";
const Data Symbols::Proxy_Authorization = "Proxy-Authorization";
const Data Symbols::Proxy_Require = "Proxy-Require";
const Data Symbols::Record_Route = "Record-Route";
const Data Symbols::Reply_To = "Reply-To";
const Data Symbols::Retry_After = "Retry-After";
const Data Symbols::Require = "Require";
const Data Symbols::Route = "Route";
const Data Symbols::Server = "Server";
const Data Symbols::Subject = "Subject";
const Data Symbols::Subscription_State = "Subscription-State";
const Data Symbols::Supported = "Supported";
const Data Symbols::Timestamp = "Timestamp";
const Data Symbols::To = "To";
const Data Symbols::Unsupported = "Unsupported";
const Data Symbols::User_Agent = "User-Agent";
const Data Symbols::Via = "Via";
const Data Symbols::WWW_Authenticate = "WWW-Authenticate";
const Data Symbols::Warning = "Warning";

const Data Symbols::Ack = "Ack";
const Data Symbols::Bye = "Bye";
const Data Symbols::Cancel = "Cancel";
const Data Symbols::Invite = "Invite";
const Data Symbols::Notify = "Notify";
const Data Symbols::Options = "Options";
const Data Symbols::Refer = "Refer";
const Data Symbols::Refer_To = "Refer-To";
const Data Symbols::Referred_By = "Referred-By";
const Data Symbols::Replaces = "Replaces";
const Data Symbols::Register = "Register";
const Data Symbols::Subscribe = "Subscribe";
const Data Symbols::Event = "Event";
const Data Symbols::Allow_Events = "Allow-Events";

const Data Symbols::transport = "transport";
const Data Symbols::user = "user";
const Data Symbols::method = "method";
const Data Symbols::ttl = "ttl";
const Data Symbols::maddr = "maddr";
const Data Symbols::lr = "lr";
const Data Symbols::q = "q";
const Data Symbols::purpose = "purpose";
const Data Symbols::expires = "expires";
const Data Symbols::handling = "handling";
const Data Symbols::tag = "tag";
const Data Symbols::toTag = "to-tag";
const Data Symbols::fromTag = "from-tag";
const Data Symbols::duration = "duration";
const Data Symbols::branch = "branch";
const Data Symbols::received = "received";
const Data Symbols::mobility = "mobility";
const Data Symbols::comp = "comp";
const Data Symbols::rport = "rport";
const Data Symbols::MagicCookie = "z9hG4bK";
const Data Symbols::Vocal2Cookie= "-c87542-";

const Data Symbols::algorithm = "algorithm";
const Data Symbols::cnonce = "cnonce";
const Data Symbols::domain = "domain";
const Data Symbols::nonce = "nonce";
const Data Symbols::nc = "nc";
const Data Symbols::opaque = "opaque";
const Data Symbols::realm = "realm";
const Data Symbols::response = "response";
const Data Symbols::stale = "stale";
const Data Symbols::username = "username";
const Data Symbols::qop = "qop";
const Data Symbols::uri = "uri";
const Data Symbols::id = "id";
const Data Symbols::retryAfter = "retry-after";
const Data Symbols::reason = "reason";

const Data Symbols::auth = "auth";

const int Symbols::DefaultSipPort = 5060;
const int Symbols::SipTlsPort = 5061;
const int Symbols::DefaultSipsPort = 5061;


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
