#include <string.h>
#include <sip2/sipstack/HeaderTypes.hxx>

int
Headers::getHeaderType(char* name, int len)
{
   if (len == 1)
   {
      switch (name[0])
      {
         case 'i' : return CALL_ID;
         case 'm' : return CONTACT;
         case 'l' : return CONTENT_LENGTH;
         case 'f' : return FROM;
         case 's' : return SUBJECT;
         case 't' : return TO;
         case 'v' : return VIA;
         default : return UNKNOWN;
      }
   }
   else if (len < 4)
   {
      if (strncasecmp(name, Symbol::To, len))
      {
         return TO;
      }
      else if (strncasecmp(name, Symbol::Via, len))
      {
         return VIA;
      }
      else
      {
         return UNKNOWN;
      }
   }
   else
   {
      switch ((int)*name)
      {
         case (int)*"Acce" : 
         {
            if (strncasecmp(name, Symbol::Accept, len) == 0)
            {
               return ACCEPT;
            }
            if (strncasecmp(name, Symbol::Accept_Encoding, len) == 0)
            {
               return ACCEPT_ENCODING;
            }
            if (strncasecmp(name, Symbol::Accept_Language, len) == 0)
            {
               return ACCEPT_LANGUAGE;
            }
            return UNKNOWN;
         }
         case (int)*"Aler" :
         {
            if (strncasecmp(name, Symbol::Alert, len) == 0)
            {
               return ALERT;
            }
            return UNKNOWN;
         }
         case (int)*"Allo" :
         {
            if (strncasecmp(name, Symbol::Allow, len) == 0)
            {
               return ALLOW;
            }
            return UNKNOWN;
         }
         case (int)*"Auth" :
         {
            if (strncasecmp(name, Symbol::Authentication_Info, len) == 0)
            {
               return AUTHENTICATION_INFO;
            }
            if (strncasecmp(name, Symbol::Authorization, len) == 0)
            {
               return AUTHORIZATION;
            }
            return UNKNOWN;
         }
         case (int)*"Call" :
         {
            if (strncasecmp(name, Symbol::Call_ID, len) == 0)
            {
               return CALL_ID;
            }
            if (strncasecmp(name, Symbol::Call_Info, len) == 0)
            {
               return CALL_INFO;
            }

            return UNKNOWN;
         }
         case (int)*"Cont" :
         {
            if (strncasecmp(name, Symbol::Contact, len) == 0)
            {
               return CONTACT;
            }
            if (strncasecmp(name, Symbol::Content_Disposition, len) == 0)
            {
               return CONTENT_DISPOSITION;
            }
            if (strncasecmp(name, Symbol::Content_Encoding, len) == 0)
            {
               return CONTENT_ENCODING;
            }
            if (strncasecmp(name, Symbol::Content_Language, len) == 0)
            {
               return CONTENT_LANGUAGE;
            }
            if (strncasecmp(name, Symbol::Content_Length, len) == 0)
            {
               return CONTENT_LENGTH;
            }
            if (strncasecmp(name, Symbol::Content_Type, len) == 0)
            {
               return CONTENT_TYPE;
            }
            return UNKNOWN;
         }
         case (int)*"CSeq" :
         {
            if (strncasecmp(name, Symbol::CSeq, len) == 0)
            {
               return CSEQ;
            }
            return UNKNOWN;
         }
         case (int)*"Date" :
         {
            if (strncasecmp(name, Symbol::Date, len) == 0)
            {
               return DATE;
            }
            return UNKNOWN;
         }
         case (int)*"Erro" :
         {
            if (strncasecmp(name, Symbol::Error_Info, len) == 0)
            {
               return ERROR_INFO;
            }
            return UNKNOWN;
         }
         case (int)*"Expi" :
         {
            if (strncasecmp(name, Symbol::Expires, len) == 0)
            {
               return EXPIRES;
            }
            return UNKNOWN;
         }
         case (int)*"From" :
         {
            if (strncasecmp(name, Symbol::From, len) == 0)
            {
               return FROM;
            }
            return UNKNOWN;
         }
         case (int)*"In-R" :
         {
            if (strncasecmp(name, Symbol::In_Reply_To, len) == 0)
            {
               return IN_REPLY_TO;
            }
            return UNKNOWN;
         }
         case (int)*"Max-" :
         {
            if (strncasecmp(name, Symbol::Max_Forwards, len) == 0)
            {
               return MAX_FORWARDS;
            }
            return UNKNOWN;
         }
         case (int)*"Min-" :
         {
            if (strncasecmp(name, Symbol::Min_Expires, len) == 0)
            {
               return MIN_EXPIRES;
            }
            return UNKNOWN;
         }
         case (int)*"MIME" :
         {
            if (strncasecmp(name, Symbol::MIME_Version, len) == 0)
            {
               return MIME_VERSION;
            }
            return UNKNOWN;
         }
         case (int)*"Orga" :
         {
            if (strncasecmp(name, Symbol::Organization, len) == 0)
            {
               return ORGANIZATION;
            }
            return UNKNOWN;
         }
         case (int)*"Prio" :
         {
            if (strncasecmp(name, Symbol::Priority, len) == 0)
            {
               return PRIORITY;
            }
            return UNKNOWN;
         }
         case (int)*"Prox" :
         {
            if (strncasecmp(name, Symbol::Proxy_Authenticate, len) == 0)
            {
               return PROXY_AUTHENTICATE;
            }
            if (strncasecmp(name, Symbol::Proxy_Authorization, len) == 0)
            {
               return PROXY_AUTHORIZATION;
            }
            if (strncasecmp(name, Symbol::Proxy_Require, len) == 0)
            {
               return PROXY_REQUIRE;
            }
            return UNKNOWN;
         }
         case (int)*"Reco" :
         {
            if (strncasecmp(name, Symbol::Record_Route, len) == 0)
            {
               return RECORD_ROUTE;
            }
            return UNKNOWN;
         }
         case (int)*"Repl" :
         {
            if (strncasecmp(name, Symbol::Reply_To, len) == 0)
            {
               return REPLY_TO;
            }
            return UNKNOWN;
         }
         case (int)*"Retr" :
         {
            if (strncasecmp(name, Symbol::Retry_After, len) == 0)
            {
               return Retry_After;
            }
            return UNKNOWN;
         }
         case (int)*"Rout" :
         {
            if (strncasecmp(name, Symbol::Retry_After, len) == 0)
            {
               return RETRY_AFTER;
            }
            return UNKNOWN;
         }
         case (int)*"Serv" :
         {
            if (strncasecmp(name, Symbol::Server, len) == 0)
            {
               return Server;
            }
            return UNKNOWN;
         }
         case (int)*"Supp" :
         {
            if (strncasecmp(name, Symbol::Supported, len) == 0)
            {
               return Supported;
            }
            return UNKNOWN;
         }
         case (int)*"Time" :
         {
            if (strncasecmp(name, Symbol::Timestamp, len) == 0)
            {
               return ALERT;
            }
            return UNKNOWN;
         }
         case (int)*"Unsu" :
         {
            if (strncasecmp(name, Symbol::Unsupported, len) == 0)
            {
               return UNSUPPORTED;
            }
            return UNKNOWN;
         }
         case (int)*"User" :
         {
            if (strncasecmp(name, Symbol::User_Agent, len) == 0)
            {
               return USER_AGENT;
            }
            return UNKNOWN;
         }
         case (int)*"Warn" :
         {
            if (strncasecmp(name, Symbol::Warning, len) == 0)
            {
               return WARNING;
            }
            return UNKNOWN;
         }
         case (int)*"WWW-" :
         {
            if (strncasecmp(name, Symbol::WWW_Authenticate, len) == 0)
            {
               return ALERT;
            }
            return UNKNOWN;
         }
         default : return UNKNOWN;
      }
   }
}
