#ifndef Symbols_hxx
#define Symbols_hxx

namespace Vocal2
{

class Symbols
{
   public:
      static const char * const DefaultSipVersion;

      static const char * const CRLF;
      static const char * const SPACE;
      static const char * const COLON;
      static const char * const EQUALS;
      static const char * const SEMI_COLON;
      static const char * const SLASH;

      static const char * const Accept;
      static const char * const Accept_Encoding;
      static const char * const Accept_Language;
      static const char * const Alert_Info;
      static const char * const Allow;
      static const char * const Authentication_Info;
      static const char * const Authorization;
      static const char * const CSeq;
      static const char * const Call_ID;
      static const char * const Call_Info;
      static const char * const Contact;
      static const char * const Content_Disposition;
      static const char * const Content_Encoding;
      static const char * const Content_Language;
      static const char * const Content_Length;
      static const char * const Content_Type;
      static const char * const Date;
      static const char * const Error_Info;
      static const char * const Expires;
      static const char * const From;
      static const char * const In_Reply_To;
      static const char * const MIME_Version;
      static const char * const Max_Forwards;
      static const char * const Min_Expires;
      static const char * const Organization;
      static const char * const Priority;
      static const char * const Proxy_Authenticate;
      static const char * const Proxy_Authorization;
      static const char * const Proxy_Require;
      static const char * const Record_Route;
      static const char * const Reply_To;
      static const char * const Retry_After;
      static const char * const Require;
      static const char * const Route;
      static const char * const Server;
      static const char * const Subject;
      static const char * const Subscription_State;
      static const char * const Supported;
      static const char * const Timestamp;
      static const char * const To;
      static const char * const Unsupported;
      static const char * const User_Agent;
      static const char * const Via;
      static const char * const WWW_Authenticate;
      static const char * const Warning;

      static const char * const Ack;
      static const char * const Bye;
      static const char * const Cancel;
      static const char * const Invite;
      static const char * const Notify;
      static const char * const Options;
      static const char * const Refer;
      static const char * const Refer_To;
      static const char * const Referred_By;
      static const char * const Register;
      static const char * const Subscribe;

      static const char * const transport;
      static const char * const user;
      static const char * const method;
      static const char * const ttl;
      static const char * const maddr;
      static const char * const lr;
      static const char * const q;
      static const char * const purpose;
      static const char * const expires;
      static const char * const handling;
      static const char * const tag;
      static const char * const toTag;
      static const char * const fromTag;
      static const char * const duration;
      static const char * const branch;
      static const char * const received;
      static const char * const comp;
      static const char * const rport;

      static const int DefaultSipPort;
};

}

#endif
