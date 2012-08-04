#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined (HAVE_POPT_H)
#include <popt.h>
#endif

#include "CommandLineParser.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseException.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   const char* logType = "cout";
   const char* logLevel = "INFO";
   int encrypt=false;
   int sign=false;
   int genUserCert = false;
   char* tlsDomain = 0;
   
   int udpPort = 5160;
   int tcpPort = 5160;
   int tlsPort = 5161;
   int dtlsPort = 5161;
   
   mRegisterDuration = 3600;
   int noV4 = false;
   int noV6 = false;
   
   char* inputAor = 0;
   const char* password = "";
   
   char* inputOutboundProxy = 0;
   char* inputContact = 0;
   char* inputBuddies = 0;
   char* inputTarget = 0;
   char* passPhrase = 0;
   char* certPath = 0;
   Data basePath(getenv("HOME"));

#if defined(HAVE_POPT_H)
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
#endif

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
      mAor.user() = "test";
      mAor.host() = DnsUtil::getLocalHostName();
   }
   
   mPassword = password;
   mOutboundProxy = toUri(inputOutboundProxy, "outbound proxy");
   mContact = toUri(inputContact, "contact");
   mBuddies = toUriVector(inputBuddies, "buddies"); // was addList   
   mTarget = toUri(inputTarget, "target"); // was dest
   if (passPhrase) mPassPhrase = passPhrase;
   if (certPath) mCertPath = certPath;
   else mCertPath = basePath + "/.sipCerts";
   
   // pubList for publish targets

   // Free the option parsing context.
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
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
         //WarningLog (<< "No " << description << " specified");
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
