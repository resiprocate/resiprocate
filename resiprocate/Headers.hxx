#ifndef Headers_hxx
#define Headers_hxx

#include <sipstack/supported.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/Symbols.hxx>
#include <util/Data.hxx>
#include <sipstack/HeaderTypes.hxx>

namespace Vocal2
{

class HeaderBase
{
   public:
      virtual Headers::Type getTypeNum() const = 0;
};

//====================
// Token:
//====================
class Content_Disposition_Header : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Content_Disposition;}
      Content_Disposition_Header()
      {
         Headers::CommaTokenizing[Headers::Content_Disposition] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Disposition] = Symbols::Content_Disposition;
      }
};
extern Content_Disposition_Header h_ContentDisposition;

class Content_Encoding_Header : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Content_Encoding;}
      Content_Encoding_Header()
      {
         Headers::CommaTokenizing[Headers::Content_Encoding] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Encoding] = Symbols::Content_Encoding;
      }
};
extern Content_Encoding_Header h_ContentEncoding;

class MIME_Version_Header : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::MIME_Version;}
      MIME_Version_Header()
      {
         Headers::CommaTokenizing[Headers::MIME_Version] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::MIME_Version] = Symbols::MIME_Version;
      }
};
extern MIME_Version_Header h_MimeVersion;

class Priority_Header : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Priority;}
      Priority_Header()
      {
         Headers::CommaTokenizing[Headers::Priority] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Priority] = Symbols::Priority;
      }
};
extern Priority_Header h_Priority;

//====================
// Tokens:
//====================
typedef ParserContainer<Token> Tokens;

class Accept_Encoding_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Accept_Encoding;}
      Accept_Encoding_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept_Encoding] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept_Encoding] = Symbols::Accept_Encoding;
      }
};
extern Accept_Encoding_MultiHeader h_AcceptEncodings;

class Accept_Language_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Accept_Language;}
      Accept_Language_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept_Language] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept_Language] = Symbols::Accept_Language;
      }
};
extern Accept_Language_MultiHeader h_AcceptLanguages;

class Allow_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Allow;}
      Allow_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Allow] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Allow] = Symbols::Allow;
      }
};
extern Allow_MultiHeader h_Allows;

class Content_Language_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Content_Language;}
      Content_Language_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Content_Language] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Language] = Symbols::Content_Language;
      }
};
extern Content_Language_MultiHeader h_ContentLanguages;

class Proxy_Require_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Proxy_Require;}
      Proxy_Require_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Proxy_Require] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Require] = Symbols::Proxy_Require;
      }
};
extern Proxy_Require_MultiHeader h_ProxyRequires;

class Require_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Require;}
      Require_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Require] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Require] = Symbols::Require;
      }
};
extern Require_MultiHeader h_Requires;

class Supported_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Supported;}
      Supported_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Supported] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Supported] = Symbols::Supported;
      }
};
extern Supported_MultiHeader h_Supporteds;

class Subscription_State_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Subscription_State;}
      Subscription_State_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Subscription_State] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Subscription_State] = Symbols::Subscription_State;
      }
};
extern Subscription_State_MultiHeader h_SubscriptionStates;

class Unsupported_MultiHeader : public HeaderBase
{
   public:
      typedef Token Type;
      virtual Headers::Type getTypeNum() const {return Headers::Unsupported;}
      Unsupported_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Unsupported] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Unsupported] = Symbols::Unsupported;
      }
};
extern Unsupported_MultiHeader h_Unsupporteds;

//====================
// Mime
//====================
typedef ParserContainer<Mime> Mimes;

class Accept_MultiHeader : public HeaderBase
{
   public:
      typedef Mime Type;
      virtual Headers::Type getTypeNum() const {return Headers::Accept;}
      Accept_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept] = Symbols::Accept;
      }
};
extern Accept_MultiHeader h_Accepts;

class Content_Type_Header : public HeaderBase
{
   public:
      typedef Mime Type;
      virtual Headers::Type getTypeNum() const {return Headers::Content_Type;}
      Content_Type_Header()
      {
         Headers::CommaTokenizing[Headers::Content_Type] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Type] = Symbols::Content_Type;
      }
};
extern Content_Type_Header h_ContentType;

//====================
// GenericURIs:
//====================
typedef ParserContainer<GenericURI> GenericURIs;

class Call_Info_MultiHeader : public HeaderBase
{
   public:
      typedef GenericURI Type;
      virtual Headers::Type getTypeNum() const {return Headers::Call_Info;}
      Call_Info_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Call_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Call_Info] = Symbols::Call_Info;
      }
};
extern Call_Info_MultiHeader h_CallInfos;

class Alert_Info_MultiHeader : public HeaderBase
{
   public:
      typedef GenericURI Type;
      virtual Headers::Type getTypeNum() const {return Headers::Alert_Info;}
      Alert_Info_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Alert_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Alert_Info] = Symbols::Alert_Info;
      }
};
extern Alert_Info_MultiHeader h_AlertInfos;

class Error_Info_MultiHeader : public HeaderBase
{
   public:
      typedef GenericURI Type;
      virtual Headers::Type getTypeNum() const {return Headers::Error_Info;}
      Error_Info_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Error_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Error_Info] = Symbols::Error_Info;
      }
};
extern Error_Info_MultiHeader h_ErrorInfos;

//====================
// NameAddr:
//====================
typedef ParserContainer<NameAddr> NameAddrs;

class Record_Route_MultiHeader : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Record_Route;}
      Record_Route_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Record_Route] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Record_Route] = Symbols::Record_Route;
      }
};
extern Record_Route_MultiHeader h_RecordRoutes;

class Route_MultiHeader : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Route;}
      Route_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Route] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Route] = Symbols::Route;
      }
};
extern Route_MultiHeader h_Routes;

class Contact_MultiHeader : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Contact;}
      Contact_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Contact] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Contact] = Symbols::Contact;
      }
};
extern Contact_MultiHeader h_Contacts;

class From_Header : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::From;}
      From_Header()
      {
         Headers::CommaTokenizing[Headers::From] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::From] = Symbols::From;
      }
};
extern From_Header h_From;

class To_Header : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::To;}
      To_Header()
      {
         Headers::CommaTokenizing[Headers::To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::To] = Symbols::To;
      }
};
extern To_Header h_To;

class Reply_To_Header : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Reply_To;}
      Reply_To_Header()
      {
         Headers::CommaTokenizing[Headers::Reply_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Reply_To] = Symbols::Reply_To;
      }
};
extern Reply_To_Header h_ReplyTo;

class Refer_To_Header : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Refer_To;}
      Refer_To_Header()
      {
         Headers::CommaTokenizing[Headers::Refer_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Refer_To] = Symbols::Refer_To;
      }
};
extern Refer_To_Header h_ReferTo;

class Referred_By_Header : public HeaderBase
{
   public:
      typedef NameAddr Type;
      virtual Headers::Type getTypeNum() const {return Headers::Referred_By;}
      Referred_By_Header()
      {
         Headers::CommaTokenizing[Headers::Referred_By] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Referred_By] = Symbols::Referred_By;
      }
};
extern Referred_By_Header h_ReferredBy;

//====================
//String:
//====================
typedef ParserContainer<StringCategory> StringCategories;

class Organization_Header : public HeaderBase
{
   public:
      typedef StringCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Organization;}
      Organization_Header()
      {
         Headers::CommaTokenizing[Headers::Organization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Organization] = Symbols::Organization;
      }
};
extern Organization_Header h_Organization;

class Server_Header : public HeaderBase
{
   public:
      typedef StringCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Server;}
      Server_Header()
      {
         Headers::CommaTokenizing[Headers::Server] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Server] = Symbols::Server;
      }
};
extern Server_Header h_Server;

class Subject_Header : public HeaderBase
{
   public:
      typedef StringCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Subject;}
      Subject_Header()
      {
         Headers::CommaTokenizing[Headers::Subject] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Subject] = Symbols::Subject;
      }
};
extern Subject_Header h_Subject;

class User_Agent_Header : public HeaderBase
{
   public:
      typedef StringCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::User_Agent;}
      User_Agent_Header()
      {
         Headers::CommaTokenizing[Headers::User_Agent] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::User_Agent] = Symbols::User_Agent;
      }
};
extern User_Agent_Header h_UserAgent;

class Timestamp_Header : public HeaderBase
{
   public:
      typedef StringCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Timestamp;}
      Timestamp_Header()
      {
         Headers::CommaTokenizing[Headers::Timestamp] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Timestamp] = Symbols::Timestamp;
      }
};
extern Timestamp_Header h_Timestamp;

//====================
// Integer:
//====================
typedef ParserContainer<IntegerCategory> IntegerCategories;

class Content_Length_Header : public HeaderBase
{
   public:
      typedef IntegerCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Content_Length;}
      Content_Length_Header()
      {
         Headers::CommaTokenizing[Headers::Content_Length] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Length] = Symbols::Content_Length;
      }
};
extern Content_Length_Header h_ContentLength;

class Expires_Header : public HeaderBase
{
   public:
      typedef IntegerCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Expires;}
      Expires_Header()
      {
         Headers::CommaTokenizing[Headers::Expires] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Expires] = Symbols::Expires;
      }
};
extern Expires_Header h_Expires;

class Max_Forwards_Header : public HeaderBase
{
   public:
      typedef IntegerCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Max_Forwards;}
      Max_Forwards_Header()
      {
         Headers::CommaTokenizing[Headers::Max_Forwards] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Max_Forwards] = Symbols::Max_Forwards;
      }
};
extern Max_Forwards_Header h_MaxForwards;

class Min_Expires_Header : public HeaderBase
{
   public:
      typedef IntegerCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Min_Expires;}
      Min_Expires_Header()
      {
         Headers::CommaTokenizing[Headers::Min_Expires] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Min_Expires] = Symbols::Min_Expires;
      }
};
extern Min_Expires_Header h_MinExpires;

// !dlb! this one is not quite right -- can have (comment) after field value
class Retry_After_Header : public HeaderBase
{
   public:
      typedef IntegerCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Retry_After;}
      Retry_After_Header()
      {
         Headers::CommaTokenizing[Headers::Retry_After] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Retry_After] = Symbols::Retry_After;
      }
};
extern Retry_After_Header h_RetryAfter;

//====================
// CallId:
//====================
class Call_ID_Header : public HeaderBase
{
   public:
      typedef CallId Type;
      virtual Headers::Type getTypeNum() const {return Headers::Call_ID;}
      Call_ID_Header()
      {
         Headers::CommaTokenizing[Headers::Call_ID] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Call_ID] = Symbols::Call_ID;
      }
};
extern Call_ID_Header h_CallId;

class Replaces_Header : public HeaderBase
{
   public:
      typedef CallId Type;
      virtual Headers::Type getTypeNum() const {return Headers::Replaces;}
      Replaces_Header()
      {
         Headers::CommaTokenizing[Headers::Replaces] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Replaces] = Symbols::Replaces;
      }
};
extern Replaces_Header h_Replaces;

class In_Reply_To_Header : public HeaderBase
{
   public:
      typedef CallId Type;
      virtual Headers::Type getTypeNum() const {return Headers::In_Reply_To;}
      In_Reply_To_Header()
      {
         Headers::CommaTokenizing[Headers::In_Reply_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::In_Reply_To] = Symbols::In_Reply_To;
      }
};
extern In_Reply_To_Header h_InReplyTo;

//====================
// Auth:
//====================
class Authentication_Info_Header : public HeaderBase
{
   public:
      typedef Auth Type;
      virtual Headers::Type getTypeNum() const {return Headers::Authentication_Info;}
      Authentication_Info_Header()
      {
         Headers::CommaTokenizing[Headers::Authentication_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Authentication_Info] = Symbols::Authentication_Info;
      }
};
extern Authentication_Info_Header h_AuthenticationInfo;

class Authorization_Header : public HeaderBase
{
   public:
      typedef Auth Type;
      virtual Headers::Type getTypeNum() const {return Headers::Authorization;}
      Authorization_Header()
      {
         Headers::CommaTokenizing[Headers::Authorization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Authorization] = Symbols::Authorization;
      }
};
extern Authorization_Header h_Authorization;

class Proxy_Authenticate_Header : public HeaderBase
{
   public:
      typedef Auth Type;
      virtual Headers::Type getTypeNum() const {return Headers::Proxy_Authenticate;}
      Proxy_Authenticate_Header()
      {
         Headers::CommaTokenizing[Headers::Proxy_Authenticate] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Authenticate] = Symbols::Proxy_Authenticate;
      }
};
extern Proxy_Authenticate_Header h_ProxyAuthenticate;

class Proxy_Authorization_Header : public HeaderBase
{
   public:
      typedef Auth Type;
      virtual Headers::Type getTypeNum() const {return Headers::Proxy_Authorization;}
      Proxy_Authorization_Header()
      {
         Headers::CommaTokenizing[Headers::Proxy_Authorization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Authorization] = Symbols::Proxy_Authorization;
      }
};
extern Proxy_Authorization_Header h_ProxyAuthorization;

class WWW_Authenticate_Header : public HeaderBase
{
   public:
      typedef Auth Type;
      virtual Headers::Type getTypeNum() const {return Headers::WWW_Authenticate;}
      WWW_Authenticate_Header()
      {
         Headers::CommaTokenizing[Headers::WWW_Authenticate] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::WWW_Authenticate] = Symbols::WWW_Authenticate;
      }
};
extern WWW_Authenticate_Header h_WWWAuthenticate;

//====================
// CSeqCategory:
//====================
class CSeq_Header : public HeaderBase
{
   public:
      typedef CSeqCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::CSeq;}
      CSeq_Header()
      {
         Headers::CommaTokenizing[Headers::CSeq] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::CSeq] = Symbols::CSeq;
      }
};
extern CSeq_Header h_CSeq;

//====================
// DateCategory:
//====================
class Date_Header : public HeaderBase
{
   public:
      typedef DateCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Date;}
      Date_Header()
      {
         Headers::CommaTokenizing[Headers::Date] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Date] = Symbols::Date;
      }
};
extern Date_Header h_Date;

//====================
// WarningCategory:
//====================
class Warning_Header : public HeaderBase
{
   public:
      typedef WarningCategory Type;
      virtual Headers::Type getTypeNum() const {return Headers::Warning;}
      Warning_Header()
      {
         Headers::CommaTokenizing[Headers::Warning] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Warning] = Symbols::Warning;
      }
};
extern Warning_Header h_Warning;

//====================
// Via
//====================
typedef ParserContainer<Via> Vias;

class Via_MultiHeader : public HeaderBase
{
   public:
      typedef Via Type;
      virtual Headers::Type getTypeNum() const {return Headers::Via;}
      Via_MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Via] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Via] = Symbols::Via;
      }
};
extern Via_MultiHeader h_Vias;

class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;
 
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
