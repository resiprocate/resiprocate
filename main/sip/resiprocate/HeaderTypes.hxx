#ifndef MessageApi_hxx
#define MessageApi_hxx

#include <sip2/sipstack/support.hxx>
#include <sip2/sipstack/ParseCategory.hxx>

namespace Vocal2;
{

class Headers
{
   public:
      // put headers that you want to appear early in the message early in
      // this set
      enum Type
      {// RequestLine, lives in RequestMessage?
       //StatusLine, lives in StatusMessage?
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
       Unsupported, User_Agent, Warning, WWW_Authenticate 

       MAX_HEADERS,
       UNKOWN
      };

      // get enum from header name
      static int getHeaderType(char* name, int len);
};

// map enum to content type via 
template <int T>
class Header
{
   public:
      // typedef Mime Type;
};

// generate types from header enums
template <int T>
class HeaderTypeHolder
{
   public:
      HeaderTypeHolder() {}
      int getValue() {return T;}

      static Header<T>::Type* createParserCategory()
      {
         return new typename Header<T>::Type();
      }

      static Header<T>::Type* createParserCategory(HeaderFieldValueList& hfvs)
      {
         Header<T> header;
         if (header.isMulti())
         {
            return new typename Header<T>::Type(hfvs);
         }
         else
         {
            return new typename Header<T>::Type(hfvs.front());
         }
      }

   private:
      HeaderTypeHolder(const HeaderTypeHolder&);
      HeaderTypeHolder& operator=(const HeaderTypeHolder&);
};

//====================
// Token:
//====================
class Header<Headers::Content_Disposition>
{
   public:
      typedef Token Type;
      bool isMulti() {return false;}
};
Header<Headers::Content_Disposition> Content_Disposition;

class Header<Headers::Content_Encoding>
{
   public:
      typedef Token Type;
      bool isMulti() {return false;}
};
Header<Headers::Content_Encoding> Content_Encoding;

class Header<Headers::MIME_Version>
{
   public:
      typedef Token Type;
      bool isMulti() {return false;}
};
Header<Headers::MIME_Version> MIME_Version;

class Header<Headers::Priority>
{
   public:
      typedef Token Type;
      bool isMulti() {return false;}
};
Header<Headers::Priority> Priority;

//====================
// Tokens:
//====================
class Header<Headers::Accept_Encoding>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Accept_Encoding> Accept_Encoding;

class Header<Headers::Accept_Lenguage>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Accept_Lenguage> Accept_Lenguage;

class Header<Headers::Allow>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Allow> Allow;

class Header<Headers::Content_Language>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Content_Language> Content_Language;

class Header<Headers::Proxy_Require>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Proxy_Require> Proxy_Require;

class Header<Headers::Require>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Require> Require;

class Header<Headers::Supported>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Supported> Supported;

class Header<Headers::Unsupported>
{
   public:
      typedef Tokens Type;
      bool isMulti() {return true;}
};
Header<Headers::Unsupported> Unsupported;

//====================
// Mime
//====================
class Header<Headers::Accept>
{
   public:
      typedef Mimes Type;
      bool isMulti() {return true;}
};
Header<Headers::Accept> Accept;

class Header<Headers::ContentType>
{
   public:
      typedef Mime Type;
      bool isMulti() {return false;}
};
Header<Headers::ContentType> ContentType;

//====================
// GenericUris:
//====================
class Header<Headers::Alert_Info>
{
   public:
      typedef GenericUris Type;
      bool isMulti() {return true;}
};
Header<Headers::Alert_Info> Alert_Info;

class Header<Headers::Error_Info>
{
   public:
      typedef GenericUris Type;
      bool isMulti() {return true;}
};
Header<Headers::Error_Info> Error_Info;

//====================
// NameAddrs:
//====================
class Header<Headers::RecordRoute>
{
   public:
      typedef NameAddr Type;
      bool isMulti() {return true;}
};
Header<Headers::RecordRoute> RecordRoute;

class Header<Headers::Route>
{
   public:
      typedef NameAddr Type;
      bool isMulti() {return true;}
};
Header<Headers::Route> Route;

//====================
// Contact:
//====================
class Header<Headers::Contact>
{
   public:
      typedef Contacts Type;
      bool isMulti() {return true;}
};
Header<Headers::Contact> Contact;

//====================
// NameAddrOrAddrSpec:
//====================
class Header<Headers::From>
{
   public:
      typedef NameAddrOrAddrSpec Type;
      bool isMulti() {return false;}
};
Header<Headers::From> From;

class Header<Headers::Reply_To>
{
   public:
      typedef NameAddrOrAddrSpec Type;
      bool isMulti() {return false;}
};
Header<Headers::Reply_To> Reply_To;

class Header<Headers::To>
{
   public:
      typedef NameAddrOrAddrSpec Type;
      bool isMulti() {return false;}
};
Header<Headers::To> To;

//====================
//String:
//====================
class Header<Headers::Organization>
{
   public:
      typedef StringComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Organization> Organization;

class Header<Headers::Server>
{
   public:
      typedef StringComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Server> Server;

class Header<Headers::Subject>
{
   public:
      typedef StringComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Subject> Subject;

class Header<Headers::User_Agent>
{
   public:
      typedef StringComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::User_Agent> User_Agent;

//====================
// Integer:
//====================
class Header<Headers::Content_Length>
{
   public:
      typedef IntegerComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Content_Length> Content_Length;

class Header<Headers::Expires>
{
   public:
      typedef IntegerComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Expires> Expires;

class Header<Headers::Max_Forwards>
{
   public:
      typedef IntegerComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Max_Forwards> Max_Forwards;

class Header<Headers::Min_Expires>
{
   public:
      typedef IntegerComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Min_Expires> Min_Expires;

// !dlb! this one is not quite right -- can have (comment) after field value
class Header<Headers::Retry_After>
{
   public:
      typedef IntegerComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Retry_After> Retry_After;

//====================
// CallId:
//====================
class Header<Headers::Call_Id>
{
   public:
      typedef CallId Type;
      bool isMulti() {return false;}
};
Header<Headers::Call_Id> Call_Id;

//====================
// CallId:
//====================
class Header<Headers::In_Reply_To>
{
   public:
      typedef CallIds Type;
      bool isMulti() {return false;}
};
Header<Headers::In_Reply_To> In_Reply_To;

//====================
// Auth:
//====================
class Header<Headers::Authentication_Info>
{
   public:
      typedef Auth Type;
      bool isMulti() {return false;}
};
Header<Headers::Authentication_Info> Authentication_Info;

class Header<Headers::Authorization>
{
   public:
      typedef Auth Type;
      bool isMulti() {return false;}
};
Header<Headers::Authorization> Authorization;

class Header<Headers::Proxy_Authenticate>
{
   public:
      typedef Auth Type;
      bool isMulti() {return false;}
};
Header<Headers::Proxy_Authenticate> Proxy_Authenticate;

class Header<Headers::Proxy_Authorization>
{
   public:
      typedef Auth Type;
      bool isMulti() {return false;}
};
Header<Headers::Proxy_Authorization> Proxy_Authorization;

class Header<Headers::WWW_Authenticate>
{
   public:
      typedef Auth Type;
      bool isMulti() {return false;}
};
Header<Headers::WWW_Authenticate> WWW_Authenticate;

//====================
// CSeqComponent:
//====================
class Header<Headers::Cseq>
{
   public:
      typedef CSeqComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Cseq> Cseq;

//====================
// DateComponent:
//====================
class Header<Headers::Date>
{
   public:
      typedef DateComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Date> Date;

//====================
// WarningComponent:
//====================
class Header<Headers::Warning>
{
   public:
      typedef WarningComponent Type;
      bool isMulti() {return false;}
};
Header<Headers::Warning> Warning;

//====================
// Via
//====================
class Header<Headers::Warning>
{
   public:
      typedef Vias Type;
      bool isMulti() {return true;}
};
Header<Headers::Via> Via;

}


#endif
