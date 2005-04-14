#include <popt.h>

#include "CommandLineParser.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/ParseException.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   char* logType = "cout";
   char* logLevel = "INFO";
   int encrypt=false;
   int sign=false;
   int genUserCert = false;
   char* tlsDomain = 0;
   
   int udpPort = 5060;
   int tcpPort = 5060;
   int tlsPort = 5061;
   int dtlsPort = 5061;
   
   mRegisterDuration = 3600;
   int noV4 = false;
   int noV6 = false;
   
   char* inputAor = 0;
   char* password = "";
   
   char* inputOutboundProxy = 0;
   char* inputContact = 0;
   char* inputBuddies = 0;
   char* inputTarget = 0;
   char* passPhrase = 0;
   char* certPath = "~/.sipCerts";


   struct poptOption table[] = {
      {"log-type",     'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"encrypt",      'e', POPT_ARG_NONE, &encrypt, 0, "whether to encrypt or not", 0},
      {"sign",         's', POPT_ARG_NONE, &sign, 0,   "signs messages you send", 0},
      {"gen-user-cert",'u', POPT_ARG_NONE, &genUserCert, 0, "generate a new user certificate", 0},
      {"tls-domain",   't', POPT_ARG_STRING, &tlsDomain,  0, "act as a TLS server for specified domain", "example.com"},
      
      {"udp",          0,   POPT_ARG_INT, &udpPort, 0, "add UDP transport on specified port", "5060"},
      {"tcp",          0,   POPT_ARG_INT, &tcpPort, 0, "add TCP transport on specified port", "5060"},
      {"tls",          0,   POPT_ARG_INT, &tlsPort, 0, "add TLS transport on specified port", "5061"},
      {"dtls",         0,   POPT_ARG_INT, &dtlsPort, 0, "add DTLS transport on specified port", "5061"},

      {"register-duration",  0,   POPT_ARG_INT, &mRegisterDuration, 0, "expires for register (0 for no reg)", "3600"},
      {"disable-v6",   0,   POPT_ARG_NONE, &noV6, 0, "disable IPV6", 0},
      {"disable-v4",   0,   POPT_ARG_NONE, &noV4, 0, "disable IPV4", 0},
      // may want to be able to specify that PUBLISH will occur

      {"aor",         'a',  POPT_ARG_STRING, &inputAor,  0, "specify address of record", "sip:alice@example.com"},
      {"password",    'p',  POPT_ARG_STRING, &password,  0, "specify password for address of record", "password"},
      {"outbound-proxy",'o',  POPT_ARG_STRING, &inputOutboundProxy,  0, "specify uri for outbound proxy (if none present, don't use)", "sip:outbound.example.com"},
      {"contact",       'c',  POPT_ARG_STRING, &inputContact,  0, "override default contact", "sip:alice@contact.example.com"},      
      {"to",            't',  POPT_ARG_STRING, &inputTarget,  0, "specify target aor", "sip:jane@example.com"},      
      {"buddies",       'b',  POPT_ARG_STRING, &inputBuddies,  0, "list of buddy aors", "sip:b1@example.com,sip:b2@example.com"},

      {"pass-phrase",   'k',  POPT_ARG_STRING, &passPhrase,  0, "pass phrase for private key", 0},
      {"cert-path",      0,   POPT_ARG_STRING, &certPath,  0, "path for certificates (default ~/.sipCerts)", 0},
      
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);

   mLogType = logType;
   mLogLevel = logLevel;
   mEncrypt = encrypt;
   mSign = sign;
   mGenUserCert = genUserCert;
   if (tlsDomain) mTlsDomain = tlsDomain;
   mUdpPort = udpPort;
   mTcpPort = tcpPort;
   mTlsPort = tlsPort;
   mDtlsPort = dtlsPort;
   mNoV4 = noV4;
   mNoV6 = noV6;
   if (inputAor)
   {
      mAor = toUri(inputAor, "aor");
   }
   else
   {
      mAor.user() = "user";
      mAor.host() = DnsUtil::getLocalHostName();
   }
   
   mPassword = password;
   mOutboundProxy = toUri(inputOutboundProxy, "outbound proxy");
   mContact = toUri(inputContact, "contact");
   mBuddies = toUriVector(inputBuddies, "buddies"); // was addList   
   mTarget = toUri(inputTarget, "target"); // was dest
   if (passPhrase) mPassPhrase = passPhrase;
   mCertPath = certPath;
   
   // pubList for publish targets
}

resip::Uri 
CommandLineParser::toUri(const char* input, const char* description)
{
   resip::Uri uri;
   try
   {
      if (input)
      {
         uri = Uri(input);
      }
      else
      {
         WarningLog (<< "No " << description << " specified");
      }
   } 
   catch (ParseException& e)
   {
      InfoLog (<< "Caught: " << e);
      WarningLog (<< "Can't parse " << description << " : " << input);
      exit(-1);
   }
   return uri;
}

std::vector<resip::Uri> 
CommandLineParser::toUriVector(const char* input, const char* description)
{
   char buffer[2048];
   strcpy(buffer, input);
   
   std::vector<Uri> uris; 
   if (input)
   {
      for (char* token = strtok(buffer, ","); token != 0; token = strtok(0, ","))
      {
         try
         {
            uris.push_back(Uri(token));
         } 
         catch (ParseException& e)
         {
            InfoLog (<< "Caught: " << e);
            WarningLog (<< "Can't parse " << description << " : " << token);
            exit(-1);
         }
      }
   }
   return uris;
}
   
