#ifndef HeaderTypes_hxx
#define HeaderTypes_hxx

#include <sipstack/supported.hxx>

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
         MAX_HEADERS = UNKNOWN
      };

      static bool CommaTokenizing[MAX_HEADERS];
      static Data HeaderNames[MAX_HEADERS];

      // get enum from header name
      static Type getType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
};
 
}

#endif
