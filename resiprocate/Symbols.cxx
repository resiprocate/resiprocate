#include <sipstack/Symbols.hxx>

using namespace Vocal2;

const char * const Symbols::DefaultSipVersion = "SIP/2.0";
const char * const Symbols::DefaultSipScheme = "sip";

const char * const Symbols::CRLF = "\r\n";
const char * const Symbols::CR = "\r";
const char * const Symbols::LF = "\n";
const char * const Symbols::TAB = "\t";

const char * const Symbols::AT_SIGN = "@";
const char * const Symbols::SPACE = " ";
const char * const Symbols::COLON = ":";
const char * const Symbols::EQUALS = "=";
const char * const Symbols::SEMI_COLON = ";";
const char * const Symbols::SLASH = "/";
const char * const Symbols::B_SLASH = "\\";
const char * const Symbols::DOUBLE_QUOTE = "\"";
const char * const Symbols::LA_QUOTE = "<";
const char * const Symbols::RA_QUOTE = ">";
const char * const Symbols::COMMA = ",";
const char * const Symbols::SEMI_OR_EQUAL = ";=";
const char * const Symbols::COMMA_OR_EQUAL = ",=";

const char * const Symbols::ProtocolName = "SIP";
const char * const Symbols::ProtocolVersion = "2.0";
const char * const Symbols::UDP = "UDP";
const char * const Symbols::TCP = "TCP";
const char * const Symbols::TLS = "TLS";
const char * const Symbols::SCTP = "SCTP";

const char * const Symbols::Sip = "sip";
const char * const Symbols::Sips = "sips";

const char * const Symbols::Accept = "Accept";
const char * const Symbols::Accept_Encoding = "Accept-Encoding";
const char * const Symbols::Accept_Language = "Accept-Language";
const char * const Symbols::Alert_Info = "Alert-Info";
const char * const Symbols::Allow = "Allow";
const char * const Symbols::Authentication_Info = "Authentication-Info";
const char * const Symbols::Authorization = "Authorization";
const char * const Symbols::CSeq = "CSeq";
const char * const Symbols::Call_ID = "Call-ID";
const char * const Symbols::Call_Info = "Call-Info";
const char * const Symbols::Contact = "Contact";
const char * const Symbols::Content_Disposition = "Content-Disposition";
const char * const Symbols::Content_Encoding = "Content-Encoding";
const char * const Symbols::Content_Language = "Content-Language";
const char * const Symbols::Content_Length = "Content-Length";
const char * const Symbols::Content_Type = "Content-Type";
const char * const Symbols::Date = "Date";
const char * const Symbols::Error_Info = "Error-Info";
const char * const Symbols::Expires = "Expires";
const char * const Symbols::From = "From";
const char * const Symbols::In_Reply_To = "In-Reply-To";
const char * const Symbols::MIME_Version = "MIME-Version";
const char * const Symbols::Max_Forwards = "Max-Forwards";
const char * const Symbols::Min_Expires = "Min-Expires";
const char * const Symbols::Organization = "Organization";
const char * const Symbols::Priority = "Priority";
const char * const Symbols::Proxy_Authenticate = "Proxy-Authenticate";
const char * const Symbols::Proxy_Authorization = "Proxy-Authorization";
const char * const Symbols::Proxy_Require = "Proxy-Require";
const char * const Symbols::Record_Route = "Record-Route";
const char * const Symbols::Reply_To = "Reply-To";
const char * const Symbols::Retry_After = "Retry-After";
const char * const Symbols::Require = "Require";
const char * const Symbols::Route = "Route";
const char * const Symbols::Server = "Server";
const char * const Symbols::Subject = "Subject";
const char * const Symbols::Subscription_State = "Subscription-State";
const char * const Symbols::Supported = "Supported";
const char * const Symbols::Timestamp = "Timestamp";
const char * const Symbols::To = "To";
const char * const Symbols::Unsupported = "Unsupported";
const char * const Symbols::User_Agent = "User-Agent";
const char * const Symbols::Via = "Via";
const char * const Symbols::WWW_Authenticate = "WWW-Authenticate";
const char * const Symbols::Warning = "Warning";

const char * const Symbols::Ack = "Ack";
const char * const Symbols::Bye = "Bye";
const char * const Symbols::Cancel = "Cancel";
const char * const Symbols::Invite = "Invite";
const char * const Symbols::Notify = "Notify";
const char * const Symbols::Options = "Options";
const char * const Symbols::Refer = "Refer";
const char * const Symbols::Refer_To = "Refer-To";
const char * const Symbols::Referred_By = "Referred-By";
const char * const Symbols::Replaces = "Replaces";
const char * const Symbols::Register = "Register";
const char * const Symbols::Subscribe = "Subscribe";

const char * const Symbols::transport = "transport";
const char * const Symbols::user = "user";
const char * const Symbols::method = "method";
const char * const Symbols::ttl = "ttl";
const char * const Symbols::maddr = "maddr";
const char * const Symbols::lr = "lr";
const char * const Symbols::q = "q";
const char * const Symbols::purpose = "purpose";
const char * const Symbols::expires = "expires";
const char * const Symbols::handling = "handling";
const char * const Symbols::tag = "tag";
const char * const Symbols::toTag = "to-tag";
const char * const Symbols::fromTag = "from-tag";
const char * const Symbols::duration = "duration";
const char * const Symbols::branch = "branch";
const char * const Symbols::received = "received";
const char * const Symbols::mobility = "mobility";
const char * const Symbols::comp = "comp";
const char * const Symbols::rport = "rport";

const int Symbols::DefaultSipPort = 5060;


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
