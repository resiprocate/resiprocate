#ifndef HeaderTypes_hxx
#define HeaderTypes_hxx

#include <sipstack/supported.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/Symbols.hxx>
#include <util/Data.hxx>

namespace Vocal2
{

class Headers
{
   public:
      // put headers that you want to appear early in the message early in
      // this set
      enum Type
      {
         CSeq, 
         Call_ID, 
         Contact, 
         Content_Length, 
         Expires, 
         From, 
         Max_Forwards, 
         Route, 
         Subject, 
         To, 
         Via, 
         Accept, 
         Accept_Encoding, 
         Accept_Language, 
         Alert_Info,
         Allow, 
         Authentication_Info, 
         Call_Info, 
         Content_Disposition, 
         Content_Encoding, 
         Content_Language, 
         Content_Type, 
         Date, 
         Error_Info, 
         In_Reply_To, 
         Min_Expires, 
         MIME_Version, 
         Organization, 
         Priority, 
         Proxy_Authenticate, 
         Proxy_Authorization, 
         Proxy_Require, 
         Record_Route, 
         Reply_To, 
         Require, 
         Retry_After, 
         Server, 
         Supported, 
         Timestamp, 
         Unsupported, 
         User_Agent, 
         Warning, 
         WWW_Authenticate,
         Subscription_State,
         Refer_To,
         Referred_By,
         Authorization, 
         Replaces,
         UNKNOWN,
         MAX_HEADERS
      };

      static bool CommaTokenizing[MAX_HEADERS];
      static Data HeaderNames[MAX_HEADERS];

      // get enum from header name
      static Type getHeaderType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
};

Headers::Type& Vocal2::operator++(Headers::Type&t);

// map enum to parser category via specialized templates
template <int T>
class Header
{
   public:
      Header()
      {
         int arr[0];
      }
};

template <int T>
class MultiHeader
{
   public:
      MultiHeader()
      {
         int arr[0];
      }
};

//====================
// Token:
//====================
class Header<Headers::Content_Disposition>
{
   public:
      typedef Token Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Content_Disposition] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Disposition] = Symbols::Content_Disposition;
      }
};
extern Header<Headers::Content_Disposition> h_ContentDisposition;

class Header<Headers::Content_Encoding>
{
   public:
      typedef Token Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Content_Encoding] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Encoding] = Symbols::Content_Encoding;
      }
};
extern Header<Headers::Content_Encoding> h_ContentEncoding;

class Header<Headers::MIME_Version>
{
   public:
      typedef Token Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::MIME_Version] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::MIME_Version] = Symbols::MIME_Version;
      }
};
extern Header<Headers::MIME_Version> h_MimeVersion;

class Header<Headers::Priority>
{
   public:
      typedef Token Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Priority] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Priority] = Symbols::Priority;
      }
};
extern Header<Headers::Priority> h_Priority;

//====================
// Tokens:
//====================
class MultiHeader<Headers::Accept_Encoding>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept_Encoding] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept_Encoding] = Symbols::Accept_Encoding;
      }
};
extern MultiHeader<Headers::Accept_Encoding> h_AcceptEncodings;

class MultiHeader<Headers::Accept_Language>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept_Language] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept_Language] = Symbols::Accept_Language;
      }
};
extern MultiHeader<Headers::Accept_Language> h_AcceptLanguages;

class MultiHeader<Headers::Allow>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Allow] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Allow] = Symbols::Allow;
      }
};
extern MultiHeader<Headers::Allow> h_Allows;

class MultiHeader<Headers::Content_Language>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Content_Language] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Language] = Symbols::Content_Language;
      }
};
extern MultiHeader<Headers::Content_Language> h_ContentLanguages;

class MultiHeader<Headers::Proxy_Require>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Proxy_Require] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Require] = Symbols::Proxy_Require;
      }
};
extern MultiHeader<Headers::Proxy_Require> h_ProxyRequires;

class MultiHeader<Headers::Require>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Require] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Require] = Symbols::Require;
      }
};
extern MultiHeader<Headers::Require> h_Requires;

class MultiHeader<Headers::Supported>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Supported] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Supported] = Symbols::Supported;
      }
};
extern MultiHeader<Headers::Supported> h_Supporteds;

class MultiHeader<Headers::Subscription_State>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Subscription_State] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Subscription_State] = Symbols::Subscription_State;
      }
};
extern MultiHeader<Headers::Subscription_State> h_SubscriptionStates;

class MultiHeader<Headers::Unsupported>
{
   public:
      typedef Token Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Unsupported] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Unsupported] = Symbols::Unsupported;
      }
};
extern MultiHeader<Headers::Unsupported> h_Unsupporteds;

//====================
// Mime
//====================
class MultiHeader<Headers::Accept>
{
   public:
      typedef Mime Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Accept] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Accept] = Symbols::Accept;
      }
};
extern MultiHeader<Headers::Accept> h_Accepts;

class Header<Headers::Content_Type>
{
   public:
      typedef Mime Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Content_Type] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Type] = Symbols::Content_Type;
      }
};
extern Header<Headers::Content_Type> h_ContentType;

//====================
// GenericURIs:
//====================
class MultiHeader<Headers::Call_Info>
{
   public:
      typedef GenericURI Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Call_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Call_Info] = Symbols::Alert_Info;
      }
};
extern MultiHeader<Headers::Call_Info> h_CallInfos;

class MultiHeader<Headers::Alert_Info>
{
   public:
      typedef GenericURI Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Alert_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Alert_Info] = Symbols::Alert_Info;
      }
};
extern MultiHeader<Headers::Alert_Info> h_AlertInfos;

class MultiHeader<Headers::Error_Info>
{
   public:
      typedef GenericURI Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Error_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Error_Info] = Symbols::Error_Info;
      }
};
extern MultiHeader<Headers::Error_Info> h_ErrorInfos;

//====================
// Url:
//====================
class MultiHeader<Headers::Record_Route>
{
   public:
      typedef Url Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Record_Route] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Record_Route] = Symbols::Record_Route;
      }
};
extern MultiHeader<Headers::Record_Route> h_RecordRoutes;

class MultiHeader<Headers::Route>
{
   public:
      typedef Url Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Route] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Route] = Symbols::Route;
      }
};
extern MultiHeader<Headers::Route> h_Routes;

class MultiHeader<Headers::Contact>
{
   public:
      typedef Url Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Contact] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Contact] = Symbols::Contact;
      }
};
extern MultiHeader<Headers::Contact> h_Contacts;

class Header<Headers::From>
{
   public:
      typedef Url Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::From] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::From] = Symbols::From;
      }
};
extern Header<Headers::From> h_From;

class Header<Headers::To>
{
   public:
      typedef Url Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::To] = Symbols::To;
      }
};
extern Header<Headers::To> h_To;

class Header<Headers::Reply_To>
{
   public:
      typedef Url Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Reply_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Reply_To] = Symbols::Reply_To;
      }
};
extern Header<Headers::Reply_To> h_ReplyTo;

class Header<Headers::Refer_To>
{
   public:
      typedef Url Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Refer_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Refer_To] = Symbols::Refer_To;
      }
};
extern Header<Headers::Refer_To> h_ReferTo;

class Header<Headers::Referred_By>
{
   public:
      typedef Url Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Referred_By] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Referred_By] = Symbols::Referred_By;
      }
};
extern Header<Headers::Referred_By> h_ReferredBy;

//====================
//String:
//====================
class Header<Headers::Organization>
{
   public:
      typedef StringComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Organization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Organization] = Symbols::Organization;
      }
};
extern Header<Headers::Organization> h_Organization;

class Header<Headers::Server>
{
   public:
      typedef StringComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Server] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Server] = Symbols::Server;
      }
};
extern Header<Headers::Server> h_Server;

class Header<Headers::Subject>
{
   public:
      typedef StringComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Subject] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Subject] = Symbols::Subject;
      }
};
extern Header<Headers::Subject> h_Subject;

class Header<Headers::User_Agent>
{
   public:
      typedef StringComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::User_Agent] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::User_Agent] = Symbols::User_Agent;
      }
};
extern Header<Headers::User_Agent> h_UserAgent;

class Header<Headers::Timestamp>
{
   public:
      typedef StringComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Timestamp] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Timestamp] = Symbols::Content_Type;
      }
};
extern Header<Headers::Timestamp> h_Timestamp;

//====================
// Integer:
//====================
class Header<Headers::Content_Length>
{
   public:
      typedef IntegerComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Content_Length] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Content_Length] = Symbols::Content_Length;
      }
};
extern Header<Headers::Content_Length> h_ContentLength;

class Header<Headers::Expires>
{
   public:
      typedef IntegerComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Expires] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Expires] = Symbols::Expires;
      }
};
extern Header<Headers::Expires> h_Expires;

class Header<Headers::Max_Forwards>
{
   public:
      typedef IntegerComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Max_Forwards] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Max_Forwards] = Symbols::Max_Forwards;
      }
};
extern Header<Headers::Max_Forwards> h_MaxForwards;

class Header<Headers::Min_Expires>
{
   public:
      typedef IntegerComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Min_Expires] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Min_Expires] = Symbols::Min_Expires;
      }
};
extern Header<Headers::Min_Expires> h_MinExpires;

// !dlb! this one is not quite right -- can have (comment) after field value
class Header<Headers::Retry_After>
{
   public:
      typedef IntegerComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Retry_After] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Retry_After] = Symbols::Retry_After;
      }
};
extern Header<Headers::Retry_After> h_RetryAfter;

//====================
// CallId:
//====================
class Header<Headers::Call_ID>
{
   public:
      typedef CallId Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Call_ID] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Call_ID] = Symbols::Call_ID;
      }
};
extern Header<Headers::Call_ID> h_CallId;

class Header<Headers::Replaces>
{
   public:
      typedef CallId Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Replaces] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Replaces] = Symbols::Replaces;
      }
};
extern Header<Headers::Replaces> h_Replaces;

//====================
// CallIds:
//====================
class Header<Headers::In_Reply_To>
{
   public:
      typedef CallId Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::In_Reply_To] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::In_Reply_To] = Symbols::In_Reply_To;
      }
};
extern Header<Headers::In_Reply_To> h_InReplyTo;

//====================
// Auth:
//====================
class Header<Headers::Authentication_Info>
{
   public:
      typedef Auth Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Authentication_Info] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Authentication_Info] = Symbols::Authentication_Info;
      }
};
extern Header<Headers::Authentication_Info> h_AuthenticationInfo;

class Header<Headers::Authorization>
{
   public:
      typedef Auth Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Authorization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Authorization] = Symbols::Authorization;
      }
};
extern Header<Headers::Authorization> h_Authorization;

class Header<Headers::Proxy_Authenticate>
{
   public:
      typedef Auth Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Proxy_Authenticate] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Authenticate] = Symbols::Proxy_Authenticate;
      }
};
extern Header<Headers::Proxy_Authenticate> h_ProxyAuthenticate;

class Header<Headers::Proxy_Authorization>
{
   public:
      typedef Auth Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Proxy_Authorization] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Proxy_Authorization] = Symbols::Proxy_Authorization;
      }
};
extern Header<Headers::Proxy_Authorization> h_ProxyAuthorization;

class Header<Headers::WWW_Authenticate>
{
   public:
      typedef Auth Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::WWW_Authenticate] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::WWW_Authenticate] = Symbols::WWW_Authenticate;
      }
};
extern Header<Headers::WWW_Authenticate> h_WWWAuthenticate;

//====================
// CSeqComponent:
//====================
class Header<Headers::CSeq>
{
   public:
      typedef CSeqComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::CSeq] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::CSeq] = Symbols::CSeq;
      }
};
extern Header<Headers::CSeq> h_CSeq;

//====================
// DateComponent:
//====================
class Header<Headers::Date>
{
   public:
      typedef DateComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Date] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Date] = Symbols::Date;
      }
};
extern Header<Headers::Date> h_Date;

//====================
// WarningComponent:
//====================
class Header<Headers::Warning>
{
   public:
      typedef WarningComponent Type;
      Header()
      {
         Headers::CommaTokenizing[Headers::Warning] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Warning] = Symbols::Warning;
      }
};
extern Header<Headers::Warning> h_Warning;

//====================
// Via
//====================
class MultiHeader<Headers::Via>
{
   public:
      typedef Via Type;
      MultiHeader()
      {
         Headers::CommaTokenizing[Headers::Via] = Type::isCommaTokenizing;
         Headers::HeaderNames[Headers::Via] = Symbols::Via;
      }
};
extern MultiHeader<Headers::Via> h_Vias;

class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;
 
}

#endif
