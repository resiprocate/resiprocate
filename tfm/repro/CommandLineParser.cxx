#include <popt.h>

#include "tfm/repro/CommandLineParser.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseException.hxx"
#include "resip/stack/InteropHelper.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   const char* logType = "file";
   const char* logLevel = "INFO";
   int interactive = 0;
   const char* tlsDomain = 0;
   const char* proxyHostName=0;
   const char* userIPAddr=0;
   const char* multihomedAddrs=0;
   const char* recordRoute = "sip:127.0.0.1";
   int flowTokens = 0;
   const char* udpPorts = "5060";
   const char* tcpPorts = "5060";
   const char* tlsPorts = "5061";
   const char* dtlsPorts = 0;
   int noV4 = 0;
   int noV6 = 0;
   int disableThreadedStack = 0;
   int useCongestionManager = 0;
   const char* certPath = 0;
   int noChallenge = 0;
   int noWebChallenge =0;

   int noRegistrar = 0;
   int noCertServer = 0;

   const char* reqChainName = "default";
   const char* mySqlServer=0;
   int httpPort=5180;
   const char* enumSuffix="";

   int encrypt=false;
   int sign=false;
   int genUserCert = false;
   
   mRegisterDuration = 3600;
   
   const char* inputAor = 0;
   const char* password = "";
   
   const char* inputOutboundProxy = 0;
   const char* inputContact = 0;
   const char* inputBuddies = 0;
   const char* inputTarget = 0;
   const char* passPhrase = 0;
   Data basePath(getenv("HOME"));


   //!dcm! TODO - ability to set subsystem specfic log levels--prob. want a
   //!group options for everything in stack.  
   struct poptOption table[] = {
      {"log-type",     'l',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARN|ERROR|FATAL"},
      {"interactive",  'i',  POPT_ARG_NONE,   &interactive, 0, "enable interactive test mode", 0},
      {"record-route",  'r',  POPT_ARG_STRING, &recordRoute,  0, "specify uri to use as Record-Route", "sip:example.com"},
      {"enable-flow-tokens",  'f',  POPT_ARG_NONE, &flowTokens,  0, "enable flow token hack", 0},
      {"tls-domain",   't',  POPT_ARG_STRING, &tlsDomain,  0, "act as a TLS server for specified domain", "example.com"},
      {"host",   0,  POPT_ARG_STRING, &proxyHostName,  0, "test a proxy running at a given hostname", "example.com"},
      {"userIPAddr",   0,  POPT_ARG_STRING, &userIPAddr,  0, "User agents will bind to this interface", "192.168.0.12"},
      {"multihome",   0,  POPT_ARG_STRING, &multihomedAddrs,  0, "A comma-separated list of available IP addresses for this machine; this can be used to test SCTP multihoming, when it is supported", ""},
      {"mysqlServer",    'x',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &mySqlServer,  0, "enable MySQL and provide name of server", "localhost"},
      {"udp",            0,  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT,    &udpPorts, 0, "add UDP transport on specified port/s", "5060"},
      {"tcp",            0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &tcpPorts, 0, "add TCP transport on specified port/s", "5060"},
      {"tls",            0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &tlsPorts, 0, "add TLS transport on specified port/s", "5061"},
      {"dtls",           0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &dtlsPorts, 0, "add DTLS transport on specified port/s", "none (dtls disabled)"},
      {"disable-v6",   0,   POPT_ARG_NONE, &noV6, 0, "disable IPV6", 0},
      {"disable-v4",   0,   POPT_ARG_NONE, &noV4, 0, "disable IPV4", 0},
      {"disable-threaded-stack",   0,   POPT_ARG_NONE, &disableThreadedStack, 0, "disable multithreaded stack", 0},
      {"use-congestion-manager",   0,   POPT_ARG_NONE, &useCongestionManager, 0, "enable congestion manager", 0},
      {"disable-auth",   0,  POPT_ARG_NONE,   &noChallenge, 0, "disable DIGEST challenges", 0},
      {"disable-web-auth",0, POPT_ARG_NONE,   &noWebChallenge, 0, "disable HTTP challenges", 0},
      {"disable-reg",  0,    POPT_ARG_NONE,   &noRegistrar, 0, "disable registrar", 0},
      {"disable-cert-server", 0,POPT_ARG_NONE, &noCertServer, 0, "run a cert server", 0},
      {"cert-path",      0,   POPT_ARG_STRING, &certPath,  0, "path for certificates (default ~/.sipCerts)", 0},
      {"reqChainName",   0,  POPT_ARG_STRING, &reqChainName,  0, "name of request chain (default: default)", 0},
      {"http",            0,  POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &httpPort, 0, "run HTTP server on specified port", "5180"},
      {"enum-suffix",     'e',   POPT_ARG_STRING, &enumSuffix,  0, "specify enum suffix to search", "e164.arp"},
      {"encrypt",      0, POPT_ARG_NONE, &encrypt, 0, "whether to encrypt or not", 0},
      {"sign",         's', POPT_ARG_NONE, &sign, 0,   "signs messages you send", 0},
      {"gen-user-cert",'u', POPT_ARG_NONE, &genUserCert, 0, "generate a new user certificate", 0},
      {"register-duration",  0,   POPT_ARG_INT, &mRegisterDuration, 0, "expires for register (0 for no reg)", "3600"},
      // may want to be able to specify that PUBLISH will occur

      {"aor",         'a',  POPT_ARG_STRING, &inputAor,  0, "specify address of record", "sip:alice@example.com"},
      {"password",    'p',  POPT_ARG_STRING, &password,  0, "specify password for address of record", "password"},
      {"outbound-proxy",'o',  POPT_ARG_STRING, &inputOutboundProxy,  0, "specify uri for outbound proxy (if none present, don't use)", "sip:outbound.example.com"},
      {"contact",       'c',  POPT_ARG_STRING, &inputContact,  0, "override default contact", "sip:alice@contact.example.com"},      
      {"to",            't',  POPT_ARG_STRING, &inputTarget,  0, "specify target aor", "sip:jane@example.com"},      
      {"buddies",       'b',  POPT_ARG_STRING, &inputBuddies,  0, "list of buddy aors", "sip:b1@example.com,sip:b2@example.com"},

      {"pass-phrase",   'k',  POPT_ARG_STRING, &passPhrase,  0, "pass phrase for private key", 0},
      
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);

   mHttpPort = httpPort;
   mLogType = logType;
   mLogLevel = logLevel;
   mInteractive = interactive != 0;

   //Log::initialize(mLogType, mLogLevel, argv[0]);

   if (tlsDomain) 
   {
      mTlsDomain = tlsDomain;
   }

   if(proxyHostName)
   {
      mProxyHostName=proxyHostName;
   }
   
   if(userIPAddr)
   {
      mUserIPAddr=userIPAddr;
   }

   mMultihomedAddrs = toDataVector(multihomedAddrs, "IP addrs for multihoming tests");

   if (recordRoute) 
   {
      mRecordRoute = toUri(recordRoute, "Record-Route");
   }

   mEnableFlowTokenHack = flowTokens != 0;
   mUdpPorts = toIntSet(udpPorts, "udp ports");
   mTcpPorts = toIntSet(tcpPorts, "tcp ports");
   mTlsPorts = toIntSet(tlsPorts, "tls ports");
   mDtlsPorts = toIntSet(dtlsPorts, "dtls ports");
   mNoV4 = noV4 != 0;
   mNoV6 = noV6 != 0;
   mThreadedStack = disableThreadedStack == 0;
   mUseCongestionManager = useCongestionManager != 0;
   if (certPath) mCertPath = certPath;
   else mCertPath = basePath + "/.sipCerts";
   mNoChallenge = noChallenge != 0;
   mNoWebChallenge = noWebChallenge != 0;
   mNoRegistrar = noRegistrar != 0 ;
   mCertServer = noCertServer == 0 ;
   mRequestProcessorChainName=reqChainName;

   if (enumSuffix) mEnumSuffix = enumSuffix;
   
   if (mySqlServer) 
   {
      mMySqlServer = Data(mySqlServer);
   }

   mEncrypt = encrypt != 0;
   mSign = sign != 0;
   mGenUserCert = genUserCert != 0;
   if (inputAor)
   {
      mAor = toUri(inputAor, "aor");
   }
   else
   {
      mAor.user() = "test";
      mAor.host() = DnsUtil::getLocalHostName();
   }
   
   mPassword = password;
   mOutboundProxy = toUri(inputOutboundProxy, "outbound proxy");
   mContact = toUri(inputContact, "contact");
   mBuddies = toUriVector(inputBuddies, "buddies"); // was addList   
   mTarget = toUri(inputTarget, "target"); // was dest
   if (passPhrase) mPassPhrase = passPhrase;
   
   // pubList for publish targets

   // Free the option parsing context.
   poptFreeContext(context);
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
   std::vector<Uri> uris; 
   if (input)
   {
      char buffer[2048];
      strcpy(buffer, input);

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

std::set<int> 
CommandLineParser::toIntSet(const char* input, const char* description)
{
   std::set<int> arguments; 

   if (input)
   {
      Data buffer = input;
      if (input)
      {
         for (char* token = strtok(const_cast<char*>(buffer.c_str()), ","); token != 0; token = strtok(0, ","))
         {
            try
            {
               int temp;
               temp = Data(token).convertInt();
               arguments.insert(temp);
            } 
            catch (ParseException&)
            {
               exit(-1);
            }
            catch (...)
            {
            }
         }
      }
   }
   return arguments;
}

std::vector<Data> 
CommandLineParser::toDataVector(const char* input, const char* description)
{
   std::vector<Data> datas; 
   if (input)
   {
      char buffer[2048];
      strcpy(buffer, input);

      for (char* token = strtok(buffer, ","); token != 0; token = strtok(0, ","))
      {
         datas.push_back(Data(token));
      }
   }
   return datas;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
