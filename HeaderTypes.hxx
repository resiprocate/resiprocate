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
         Body,

         CSeq, Call_ID, Contact, Content_Length, Expires, 
         From, Max_Forwards, Route, Subject, To, Via, 

         Accept, Accept_Encoding, Accept_Language, Alert_Info, 
         Allow, Authentication_Info, Authorization, Call_Info, 
         Content_Disposition, Content_Encoding, Content_Language, 
         Content_Type, Date, Error_Info, In_Reply_To, Min_Expires, 
         MIME_Version, Organization, Priority, Proxy_Authenticate, 
         Proxy_Authorization, Proxy_Require, Record_Route, Reply_To, 
         Require, Retry_After, Server, Supported, Timestamp, 
         Unsupported, User_Agent, Warning, WWW_Authenticate,
         Subscription_State,

         Refer_To,
         Referred_By,

         UNKNOWN,
         MAX_HEADERS
      };

      static bool CommaTokenizing[MAX_HEADERS];
      static Data HeaderNames[MAX_HEADERS];

      // get enum from header name
      static Type getHeaderType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
};

// map enum to parser category via specialized templates
template <int T>
class Header
{
};

template <int T>
class MultiHeader
{
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
};
extern Header<Headers::Content_Encoding> h_ContentEncoding;

class Header<Headers::MIME_Version>
{
   public:
      typedef Token Type;
};
extern Header<Headers::MIME_Version> h_MimeVersion;

class Header<Headers::Priority>
{
   public:
      typedef Token Type;
};
extern Header<Headers::Priority> h_Priority;

//====================
// Tokens:
//====================
class MultiHeader<Headers::Accept_Encoding>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Accept_Encoding> h_AcceptEncodings;

class MultiHeader<Headers::Accept_Language>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Accept_Language> h_AcceptLanguages;

class Header<Headers::Allow>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Allow> h_Allows;

class MultiHeader<Headers::Content_Language>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Content_Language> h_ContentLanguages;

class MultiHeader<Headers::Proxy_Require>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Proxy_Require> h_ProxyRequires;

class MultiHeader<Headers::Require>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Require> h_Requires;

class MultiHeader<Headers::Supported>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Supported> h_Supporteds;

class MultiHeader<Headers::Subscription_State>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Subscription_State> h_SubscriptionStates;

class MultiHeader<Headers::Unsupported>
{
   public:
      typedef Token Type;
};
extern MultiHeader<Headers::Unsupported> h_Unsupporteds;

//====================
// Mime
//====================
class MultiHeader<Headers::Accept>
{
   public:
      typedef Mime Type;
};
extern MultiHeader<Headers::Accept> h_Accepts;

class Header<Headers::Content_Type>
{
   public:
      typedef Mime Type;
};
extern Header<Headers::Content_Type> h_ContentType;

//====================
// GenericURIs:
//====================
class MultiHeader<Headers::Alert_Info>
{
   public:
      typedef GenericURI Type;
};
extern MultiHeader<Headers::Alert_Info> h_AlertInfos;

class MultiHeader<Headers::Error_Info>
{
   public:
      typedef GenericURI Type;
};
extern MultiHeader<Headers::Error_Info> h_ErrorInfos;

//====================
// Url:
//====================
class MultiHeader<Headers::Record_Route>
{
   public:
      typedef Url Type;
};
extern MultiHeader<Headers::Record_Route> h_RecordRoutes;

class MultiHeader<Headers::Route>
{
   public:
      typedef Url Type;
};
extern MultiHeader<Headers::Route> h_Routes;

class MultiHeader<Headers::Contact>
{
   public:
      typedef Url Type;
};
extern MultiHeader<Headers::Contact> h_Contacts;

class Header<Headers::From>
{
   public:
      typedef Url Type;
};
extern Header<Headers::From> h_From;

class Header<Headers::To>
{
   public:
      typedef Url Type;
};
extern Header<Headers::To> h_To;

class Header<Headers::Reply_To>
{
   public:
      typedef Url Type;
};
extern Header<Headers::Reply_To> h_ReplyTo;

class Header<Headers::Refer_To>
{
   public:
      typedef Url Type;
};
extern Header<Headers::Refer_To> h_ReferTo;

class Header<Headers::Referred_By>
{
   public:
      typedef Url Type;
};
extern Header<Headers::Referred_By> h_ReferredBy;

//====================
//String:
//====================
class Header<Headers::Organization>
{
   public:
      typedef StringComponent Type;
};
extern Header<Headers::Organization> h_Organization;

class Header<Headers::Server>
{
   public:
      typedef StringComponent Type;
};
extern Header<Headers::Server> h_Server;

class Header<Headers::Subject>
{
   public:
      typedef StringComponent Type;
};
extern Header<Headers::Subject> h_Subject;

class Header<Headers::User_Agent>
{
   public:
      typedef StringComponent Type;
};
extern Header<Headers::User_Agent> h_UserAgent;

//====================
// Integer:
//====================
class Header<Headers::Content_Length>
{
   public:
      typedef IntegerComponent Type;
};
extern Header<Headers::Content_Length> h_ContentLength;

class Header<Headers::Expires>
{
   public:
      typedef IntegerComponent Type;
};
extern Header<Headers::Expires> h_Expires;

class Header<Headers::Max_Forwards>
{
   public:
      typedef IntegerComponent Type;
};
extern Header<Headers::Max_Forwards> h_MaxForwards;

class Header<Headers::Min_Expires>
{
   public:
      typedef IntegerComponent Type;
};
extern Header<Headers::Min_Expires> h_MinExpires;

// !dlb! this one is not quite right -- can have (comment) after field value
class Header<Headers::Retry_After>
{
   public:
      typedef IntegerComponent Type;
};
extern Header<Headers::Retry_After> h_RetryAfter;

//====================
// CallId:
//====================
class Header<Headers::Call_ID>
{
   public:
      typedef CallId Type;
};
extern Header<Headers::Call_ID> h_CallId;

//====================
// CallIds:
//====================
class Header<Headers::In_Reply_To>
{
   public:
      typedef CallIds Type;
};
extern Header<Headers::In_Reply_To> h_InReplyTo;

//====================
// Auth:
//====================
class Header<Headers::Authentication_Info>
{
   public:
      typedef Auth Type;
};
extern Header<Headers::Authentication_Info> h_AuthenticationInfo;

class Header<Headers::Authorization>
{
   public:
      typedef Auth Type;
};
extern Header<Headers::Authorization> h_Authorization;

class Header<Headers::Proxy_Authenticate>
{
   public:
      typedef Auth Type;
};
extern Header<Headers::Proxy_Authenticate> h_ProxyAuthenticate;

class Header<Headers::Proxy_Authorization>
{
   public:
      typedef Auth Type;
};
extern Header<Headers::Proxy_Authorization> h_ProxyAuthorization;

class Header<Headers::WWW_Authenticate>
{
   public:
      typedef Auth Type;
};
extern Header<Headers::WWW_Authenticate> h_WWWAuthenticate;

//====================
// CSeqComponent:
//====================
class Header<Headers::CSeq>
{
   public:
      typedef CSeqComponent Type;
};
extern Header<Headers::CSeq> h_CSeq;

//====================
// DateComponent:
//====================
class Header<Headers::Date>
{
   public:
      typedef DateComponent Type;
};
extern Header<Headers::Date> h_Date;

//====================
// WarningComponent:
//====================
class Header<Headers::Warning>
{
   public:
      typedef WarningComponent Type;
};
extern Header<Headers::Warning> h_Warning;

//====================
// Via
//====================
class MultiHeader<Headers::Via>
{
   public:
      typedef Via Type;
};
extern MultiHeader<Headers::Via> h_Vias;

class RequestLineType {};
extern RequestLineType h_RequestLine;

class StatusLineType {};
extern StatusLineType h_StatusLine;
 
}


#endif
