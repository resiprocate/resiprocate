#include <popt.h>

#include "CommandLineParser.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/ParseException.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   char* logType = "cout";
   char* logLevel = "INFO";
   char* tlsDomain = 0;
   int udpPort = 5060;
   int tcpPort = 5060;
   int tlsPort = 5061;
   int dtlsPort = 5061;
   int noV4 = false;
   int noV6 = false;
   char* domains = 0;
   char* certPath = "~/.sipCerts";

   struct poptOption table[] = {
      {"log-type",     'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"tls-domain",   't', POPT_ARG_STRING, &tlsDomain,  0, "act as a TLS server for specified domain", "example.com"},
      {"udp",          0,   POPT_ARG_INT, &udpPort, 0, "add UDP transport on specified port", "5060"},
      {"tcp",          0,   POPT_ARG_INT, &tcpPort, 0, "add TCP transport on specified port", "5060"},
      {"tls",          0,   POPT_ARG_INT, &tlsPort, 0, "add TLS transport on specified port", "5061"},
      {"dtls",         0,   POPT_ARG_INT, &dtlsPort, 0, "add DTLS transport on specified port", "5061"},
      {"disable-v6",   '6',   POPT_ARG_NONE, &noV6, 0, "disable IPV6", 0},
      {"disable-v4",   '4',   POPT_ARG_NONE, &noV4, 0, "disable IPV4", 0},
      {"domains",     'd',  POPT_ARG_STRING, &domains,  0, "specify domains that this proxy is authorative", "example.com,foo.com"},
      {"cert-path",   'c',   POPT_ARG_STRING, &certPath,  0, "path for certificates (default ~/.sipCerts)", 0},
      
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);

   mLogType = logType;
   mLogLevel = logLevel;
   if (tlsDomain) mTlsDomain = tlsDomain;
   mUdpPort = udpPort;
   mTcpPort = tcpPort;
   mTlsPort = tlsPort;
   mDtlsPort = dtlsPort;
   mNoV4 = noV4;
   mNoV6 = noV6;
   mDomains = toUriVector(domains, "domains"); 
   mCertPath = certPath;
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
   
