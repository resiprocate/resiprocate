
#if HAVE_POPT_H
#include <popt.h>
#endif

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
   int dtlsPort = 0;
   int disableV4 = false;
   int enableV6 = false;
   char* domains = 0;
   char certPathBuf[256];
   char* certPath = certPathBuf;
   int noChallenge = false;
   int noWebChallenge = false;
   
   int noRegistrar = false;
   int certServer = true;

   char* reqChainName = "default";
   char* mySqlServer = 0;
   int httpPort = 5080;
   
#ifdef WIN32
   noChallenge = 1;
   logLevel = "ERR";
   strcpy(certPath,"C:\\sipCerts");
#else
   strcpy(certPath, getenv("HOME"));
   strcat(certPath, "/.sipCerts");
#endif

#ifdef HAVE_POPT_H
   struct poptOption table[] = {
      {"log-type",     'l',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"tls-domain",   't',  POPT_ARG_STRING, &tlsDomain,  0, "act as a TLS server for specified domain", "example.com"},
      {"mysqlServer",    'x',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &mySqlServer,  0, "enable MySQL and provide name of server", "localhost"},
      {"udp",            0,  POPT_ARG_INT| POPT_ARGFLAG_SHOW_DEFAULT,    &udpPort, 0, "add UDP transport on specified port", "5060"},
      {"tcp",            0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,    &tcpPort, 0, "add TCP transport on specified port", "5060"},
      {"tls",            0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,    &tlsPort, 0, "add TLS transport on specified port", "5061"},
      {"dtls",           0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,    &dtlsPort, 0, "add DTLS transport on specified port", "5061"},
      {"enable-v6",      0,  POPT_ARG_NONE,   &enableV6, 0, "disable IPV6", 0},
      {"disable-v4",     0,  POPT_ARG_NONE,   &disableV4, 0, "disable IPV4", 0},
      {"disable-auth",   0,  POPT_ARG_NONE,   &noChallenge, 0, "disable DIGEST challenges", 0},
      {"disable-web-auth",0, POPT_ARG_NONE,   &noWebChallenge, 0, "disable HTTP challenges", 0},
      {"disable-reg",  0,    POPT_ARG_NONE,   &noRegistrar, 0, "disable registrar", 0},
      {"enable-cert-server", 0,POPT_ARG_NONE, &certServer, 0, "run a cert server", 0},
      {"domains",     'd',   POPT_ARG_STRING, &domains,  0, "specify domains that this proxy is authorative", "example.com,foo.com"},
      {"cert-path",   'c',   POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &certPath,  0, "path for certificates (default: ~/.sipCerts)", 0},
      {"reqChainName",   0,  POPT_ARG_STRING, &reqChainName,  0, "name of request chain (default: default)", 0},
      {"http",            0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &httpPort, 0, "run HTTP server on specified port", "5080"},
      POPT_AUTOHELP 
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   if (poptGetNextOpt(context) < -1)
   {
      cerr << "Bad command line argument entered" << endl;
      poptPrintHelp(context, stderr, 0);
      exit(-1);
   }
#endif

   mHttpPort = httpPort;
   mLogType = logType;
   mLogLevel = logLevel;
   if (tlsDomain) mTlsDomain = tlsDomain;
   mUdpPort = udpPort;
   mTcpPort = tcpPort;
   mTlsPort = tlsPort;
   mDtlsPort = dtlsPort;
   mUseV4 = !disableV4;
   mUseV6 = enableV6?true:false;
   mDomains = toVector(domains, "domains"); 
   mCertPath = certPath;
   mNoChallenge = noChallenge != 0;
   mNoWebChallenge = noWebChallenge != 0;
   mNoRegistrar = noRegistrar != 0 ;
   mCertServer = certServer !=0 ;
   mRequestProcessorChainName=reqChainName;
   if (mySqlServer) 
   {
      mMySqlServer = Data(mySqlServer);
   }
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

std::vector<resip::Data> 
CommandLineParser::toVector(const char* input, const char* description)
{
   std::vector<Data> domains; 

   if (input)
   {
      Data buffer = input;
      if (input)
      {
         for (char* token = strtok(const_cast<char*>(buffer.c_str()), ","); token != 0; token = strtok(0, ","))
         {
            try
            {
               domains.push_back(token);
            } 
            catch (ParseException& e)
            {
               InfoLog (<< "Caught: " << e);
               WarningLog (<< "Can't parse " << description << " : " << token);
               exit(-1);
            }
            catch (...)
            {
               InfoLog (<< "Caught some exception" );
               WarningLog (<< "Some problem parsing " << description << " : " << token);
            }
         }
      }
   }
   return domains;
}
   
