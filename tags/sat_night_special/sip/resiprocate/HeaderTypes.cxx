#include <sipstack/Data.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;

bool
Headers::isCommaTokenizing(Type type)
{
   static bool once = true;
   if (once)
   {
      once = false;
      Headers::CommaTokenizing[Headers::Accept] = true;
      Headers::CommaTokenizing[Headers::Accept_Encoding] = true;
      Headers::CommaTokenizing[Headers::Accept_Language] = true;
      Headers::CommaTokenizing[Headers::Alert_Info] = true;
      Headers::CommaTokenizing[Headers::Allow] = true;
      Headers::CommaTokenizing[Headers::Authentication_Info] = false;
      Headers::CommaTokenizing[Headers::Authorization] = false;
      Headers::CommaTokenizing[Headers::CSeq] = false;
      Headers::CommaTokenizing[Headers::Call_ID] = false;
      Headers::CommaTokenizing[Headers::Contact] = true;
      Headers::CommaTokenizing[Headers::Content_Disposition] = true;
      Headers::CommaTokenizing[Headers::Content_Encoding] = true;
      Headers::CommaTokenizing[Headers::Content_Language] = true;
      Headers::CommaTokenizing[Headers::Content_Length] = false;
      Headers::CommaTokenizing[Headers::Content_Type] = true;
      Headers::CommaTokenizing[Headers::Date] = false;
      Headers::CommaTokenizing[Headers::Error_Info] = true;
      Headers::CommaTokenizing[Headers::Expires] = false;
      Headers::CommaTokenizing[Headers::From] = true;
      Headers::CommaTokenizing[Headers::In_Reply_To] = false;
      Headers::CommaTokenizing[Headers::MIME_Version] = true;
      Headers::CommaTokenizing[Headers::Max_Forwards] = false;
      Headers::CommaTokenizing[Headers::Min_Expires] = false;
      Headers::CommaTokenizing[Headers::Organization] = false;
      Headers::CommaTokenizing[Headers::Priority] = true;
      Headers::CommaTokenizing[Headers::Proxy_Authenticate] = false;
      Headers::CommaTokenizing[Headers::Proxy_Authorization] = false;
      Headers::CommaTokenizing[Headers::Proxy_Require] = true;
      Headers::CommaTokenizing[Headers::Record_Route] = true;
      Headers::CommaTokenizing[Headers::Reply_To] = true;
      Headers::CommaTokenizing[Headers::Require] = true;
      Headers::CommaTokenizing[Headers::Retry_After] = false;
      Headers::CommaTokenizing[Headers::Route] = true;
      Headers::CommaTokenizing[Headers::Server] = false;
      Headers::CommaTokenizing[Headers::Subject] = false;
      Headers::CommaTokenizing[Headers::Supported] = true;
      Headers::CommaTokenizing[Headers::To] = true;
      Headers::CommaTokenizing[Headers::Unsupported] = true;
      Headers::CommaTokenizing[Headers::User_Agent] = false;
      Headers::CommaTokenizing[Headers::Via] = true;
      Headers::CommaTokenizing[Headers::WWW_Authenticate] = false;
      Headers::CommaTokenizing[Headers::Warning] = true;
      Headers::CommaTokenizing[Headers::UNKNOWN] = false;
   }
   
      
   return CommaTokenizing[type];
}

// can't do  *(int*)"Acce" at compile time, so precalculated
static const unsigned int IntVal_Acce = 1701012289;
static const unsigned int IntVal_Aler = 1919249473;
static const unsigned int IntVal_Allo = 1869376577;
static const unsigned int IntVal_Auth = 1752462657;
static const unsigned int IntVal_Call = 1819042115;
static const unsigned int IntVal_Cont = 1953394499;
static const unsigned int IntVal_CSeq = 1902465859;
static const unsigned int IntVal_Date = 1702125892;
static const unsigned int IntVal_Erro = 1869771333;
static const unsigned int IntVal_Expi = 1768978501;
static const unsigned int IntVal_From = 1836020294;
static const unsigned int IntVal_In_R = 1378709065;
static const unsigned int IntVal_Max_ = 762863949;
static const unsigned int IntVal_Min_ = 762210637;
static const unsigned int IntVal_MIME = 1162692941;
static const unsigned int IntVal_Orga = 1634169423;
static const unsigned int IntVal_Prio = 1869181520;
static const unsigned int IntVal_Prox = 2020569680;
static const unsigned int IntVal_Reco = 1868784978;
static const unsigned int IntVal_Repl = 1819305298;
static const unsigned int IntVal_Retr = 1920230738;
static const unsigned int IntVal_Rout = 1953853266;
static const unsigned int IntVal_Serv = 1987208531;
static const unsigned int IntVal_Supp = 1886418259;
static const unsigned int IntVal_Time = 1701669204;
static const unsigned int IntVal_Unsu = 1970499157;
static const unsigned int IntVal_User = 1919251285;
static const unsigned int IntVal_Warn = 1852989783;
static const unsigned int IntVal_WWW_ = 760698711;

bool Headers::CommaTokenizing[] = {false};

Headers::Type
Headers::getHeaderType(const char* name, int len)
{
   if (len == 1)
   {
      switch (name[0])
      {
         case 'i' : return Call_ID;
         case 'm' : return Contact;
         case 'l' : return Content_Length;
         case 'f' : return From;
         case 's' : return Subject;
         case 't' : return To;
         case 'v' : return Via;
         default : return UNKNOWN;
      }
   }
   else if (len < 4)
   {
      if (strncasecmp(name, Symbols::To, len))
      {
         return To;
      }
      else if (strncasecmp(name, Symbols::Via, len))
      {
         return Via;
      }
      else
      {
         return UNKNOWN;
      }
   }
   else
   {
      switch (*(int*)name)
      {
         case IntVal_Acce : 
         {
            if (strncasecmp(name, Symbols::Accept, len) == 0)
            {
               return Accept;
            }
            if (strncasecmp(name, Symbols::Accept_Encoding, len) == 0)
            {
               return Accept_Encoding;
            }
            if (strncasecmp(name, Symbols::Accept_Language, len) == 0)
            {
               return Accept_Language;
            }
            return UNKNOWN;
         }
         case IntVal_Aler :
         {
            if (strncasecmp(name, Symbols::Alert_Info, len) == 0)
            {
               return Alert_Info;
            }
            return UNKNOWN;
         }
         case IntVal_Allo :
         {
            if (strncasecmp(name, Symbols::Allow, len) == 0)
            {
               return Allow;
            }
            return UNKNOWN;
         }
         case IntVal_Auth :
         {
            if (strncasecmp(name, Symbols::Authentication_Info, len) == 0)
            {
               return Authentication_Info;
            }
            if (strncasecmp(name, Symbols::Authorization, len) == 0)
            {
               return Authorization;
            }
            return UNKNOWN;
         }
         case IntVal_Call :
         {
            if (strncasecmp(name, Symbols::Call_ID, len) == 0)
            {
               return Call_ID;
            }
            if (strncasecmp(name, Symbols::Call_Info, len) == 0)
            {
               return Call_Info;
            }
            return UNKNOWN;
         }
         case IntVal_Cont :
         {
            if (strncasecmp(name, Symbols::Contact, len) == 0)
            {
               return Contact;
            }
            if (strncasecmp(name, Symbols::Content_Disposition, len) == 0)
            {
               return Content_Disposition;
            }
            if (strncasecmp(name, Symbols::Content_Encoding, len) == 0)
            {
               return Content_Encoding;
            }
            if (strncasecmp(name, Symbols::Content_Language, len) == 0)
            {
               return Content_Language;
            }
            if (strncasecmp(name, Symbols::Content_Length, len) == 0)
            {
               return Content_Length;
            }
            if (strncasecmp(name, Symbols::Content_Type, len) == 0)
            {
               return Content_Type;
            }
            return UNKNOWN;
         }
         case IntVal_CSeq :
         {
            if (strncasecmp(name, Symbols::CSeq, len) == 0)
            {
               return CSeq;
            }
            return UNKNOWN;
         }
         case IntVal_Date :
         {
            if (strncasecmp(name, Symbols::Date, len) == 0)
            {
               return Date;
            }
            return UNKNOWN;
         }
         case IntVal_Erro :
         {
            if (strncasecmp(name, Symbols::Error_Info, len) == 0)
            {
               return Error_Info;
            }
            return UNKNOWN;
         }
         case IntVal_Expi :
         {
            if (strncasecmp(name, Symbols::Expires, len) == 0)
            {
               return Expires;
            }
            return UNKNOWN;
         }
         case IntVal_From :
         {
            if (strncasecmp(name, Symbols::From, len) == 0)
            {
               return From;
            }
            return UNKNOWN;
         }
         case IntVal_In_R :
         {
            if (strncasecmp(name, Symbols::In_Reply_To, len) == 0)
            {
               return In_Reply_To;
            }
            return UNKNOWN;
         }
         case IntVal_Max_ :
         {
            if (strncasecmp(name, Symbols::Max_Forwards, len) == 0)
            {
               return Max_Forwards;
            }
            return UNKNOWN;
         }
         case IntVal_Min_ :
         {
            if (strncasecmp(name, Symbols::Min_Expires, len) == 0)
            {
               return Min_Expires;
            }
            return UNKNOWN;
         }
         case IntVal_MIME :
         {
            if (strncasecmp(name, Symbols::MIME_Version, len) == 0)
            {
               return MIME_Version;
            }
            return UNKNOWN;
         }
         case IntVal_Orga :
         {
            if (strncasecmp(name, Symbols::Organization, len) == 0)
            {
               return Organization;
            }
            return UNKNOWN;
         }
         case IntVal_Prio :
         {
            if (strncasecmp(name, Symbols::Priority, len) == 0)
            {
               return Priority;
            }
            return UNKNOWN;
         }
         case IntVal_Prox :
         {
            if (strncasecmp(name, Symbols::Proxy_Authenticate, len) == 0)
            {
               return Proxy_Authenticate;
            }
            if (strncasecmp(name, Symbols::Proxy_Authorization, len) == 0)
            {
               return Proxy_Authorization;
            }
            if (strncasecmp(name, Symbols::Proxy_Require, len) == 0)
            {
               return Proxy_Require;
            }
            return UNKNOWN;
         }
         case IntVal_Reco :
         {
            if (strncasecmp(name, Symbols::Record_Route, len) == 0)
            {
               return Record_Route;
            }
            return UNKNOWN;
         }
         case IntVal_Repl :
         {
            if (strncasecmp(name, Symbols::Reply_To, len) == 0)
            {
               return Reply_To;
            }
            return UNKNOWN;
         }
         case IntVal_Retr :
         {
            if (strncasecmp(name, Symbols::Retry_After, len) == 0)
            {
               return Retry_After;
            }
            return UNKNOWN;
         }
         case IntVal_Rout :
         {
            if (strncasecmp(name, Symbols::Route, len) == 0)
            {
               return Route;
            }
            return UNKNOWN;
         }
         case IntVal_Serv :
         {
            if (strncasecmp(name, Symbols::Server, len) == 0)
            {
               return Server;
            }
            return UNKNOWN;
         }
         case IntVal_Supp :
         {
            if (strncasecmp(name, Symbols::Supported, len) == 0)
            {
               return Supported;
            }
            return UNKNOWN;
         }
         case IntVal_Time :
         {
            if (strncasecmp(name, Symbols::Timestamp, len) == 0)
            {
               return Alert_Info;
            }
            return UNKNOWN;
         }
         case IntVal_Unsu :
         {
            if (strncasecmp(name, Symbols::Unsupported, len) == 0)
            {
               return Unsupported;
            }
            return UNKNOWN;
         }
         case IntVal_User :
         {
            if (strncasecmp(name, Symbols::User_Agent, len) == 0)
            {
               return User_Agent;
            }
            return UNKNOWN;
         }
         case IntVal_Warn :
         {
            if (strncasecmp(name, Symbols::Warning, len) == 0)
            {
               return Warning;
            }
            return UNKNOWN;
         }
         case IntVal_WWW_ :
         {
            if (strncasecmp(name, Symbols::WWW_Authenticate, len) == 0)
            {
               return WWW_Authenticate;
            }
            return UNKNOWN;
         }
         default : return UNKNOWN;
      }
   }
}

Header<Headers::Content_Disposition> Content_Disposition;
Header<Headers::Content_Encoding> Content_Encoding;
Header<Headers::MIME_Version> MIME_Version;
Header<Headers::Priority> Priority;
Header<Headers::Accept_Encoding> Accept_Encoding;
Header<Headers::Accept_Language> Accept_Language;
Header<Headers::Allow> Allow;
Header<Headers::Content_Language> Content_Language;
Header<Headers::Proxy_Require> Proxy_Require;
Header<Headers::Require> Require;
Header<Headers::Supported> Supported;
Header<Headers::Unsupported> Unsupported;
Header<Headers::Accept> Accept;
Header<Headers::Content_Type> Content_Type;
Header<Headers::Alert_Info> Alert_Info;
Header<Headers::Error_Info> Error_Info;
Header<Headers::Record_Route> Record_Route;
Header<Headers::Route> Route;
Header<Headers::Contact> Contact;
Header<Headers::From> From;
Header<Headers::Reply_To> Reply_To;
Header<Headers::To> To;
Header<Headers::Organization> Organization;
Header<Headers::Server> Server;
Header<Headers::Subject> Subject;
Header<Headers::User_Agent> User_Agent;
Header<Headers::Content_Length> Content_Length;
Header<Headers::Expires> Expires;
Header<Headers::Max_Forwards> Max_Forwards;
Header<Headers::Min_Expires> Min_Expires;
Header<Headers::Retry_After> Retry_After;
Header<Headers::Call_ID> Call_ID;
Header<Headers::In_Reply_To> In_Reply_To;
Header<Headers::Authentication_Info> Authentication_Info;
Header<Headers::Authorization> Authorization;
Header<Headers::Proxy_Authenticate> Proxy_Authenticate;
Header<Headers::Proxy_Authorization> Proxy_Authorization;
Header<Headers::WWW_Authenticate> WWW_Authenticate;
Header<Headers::CSeq> CSeq;
Header<Headers::Date> Date;
Header<Headers::Warning> Warning;
Header<Headers::Via> Via;

Vocal2::RequestLineType Vocal2::RequestLine;
Vocal2::StatusLineType Vocal2::StatusLine;
