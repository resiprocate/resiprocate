#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined (HAVE_POPT_H)
#include <popt.h>
#endif

#include "basicClientCmdLineParser.hxx"
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/ParseException.hxx>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

BasicClientCmdLineParser::BasicClientCmdLineParser(int argc, char** argv)
{
   const char* logType = "cout";
   const char* logLevel = "INFO";
   char* tlsDomain = 0;
   char* certPath = 0;
   
   int udpPort = 5160;
   int tcpPort = 5160;
   int tlsPort = 5161;
   int dtlsPort = 5161;
   
   mRegisterDuration = 3600;
   int noV4 = false;
   int enableV6 = false;
   int hostFileLookupOnlyDnsMode = false;
   
   char* inputAor = 0;
   const char* password = "";
   
   char* inputOutboundProxy = 0;
   char* inputContact = 0;
   Data basePath(getenv("HOME"));

   int outboundEnabled = false;

   char* subscribeTarget = 0;
   char* callTarget = 0;

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",         'l', POPT_ARG_STRING, &logType,            0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",        'v', POPT_ARG_STRING, &logLevel,           0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
#if defined(USE_SSL)
      {"tls-domain",       't', POPT_ARG_STRING, &tlsDomain,          0, "act as a TLS server for specified domain", "example.com"},
      {"cert-path",          0, POPT_ARG_STRING, &certPath,           0, "path for certificates (default ~/.sipCerts)", 0},
#endif
      
      {"udp",                0, POPT_ARG_INT,    &udpPort,            0, "add UDP transport on specified port", "5160"},
      {"tcp",                0, POPT_ARG_INT,    &tcpPort,            0, "add TCP transport on specified port", "5160"},
#if defined(USE_SSL)
      {"tls",                0, POPT_ARG_INT,    &tlsPort,            0, "add TLS transport on specified port", "5161"},
#endif
#if defined(USE_DTLS)
      {"dtls",               0, POPT_ARG_INT,    &dtlsPort,           0, "add DTLS transport on specified port", "5161"},
#endif

      {"register-duration",'r', POPT_ARG_INT,    &mRegisterDuration,  0, "expires for register (0 for no reg)", "3600"},
      {"enable-v6",          0, POPT_ARG_NONE,   &enableV6,           0, "enable IPV6", 0},
      {"disable-v4",         0, POPT_ARG_NONE,   &noV4,               0, "disable IPV4", 0},
      {"enable-hostfileonly",0, POPT_ARG_NONE,   &hostFileLookupOnlyDnsMode,0, "enable hostfile only dns lookup mode", 0},

      {"aor",              'a', POPT_ARG_STRING, &inputAor,           0, "specify address of record", "sip:alice@example.com"},
      {"password",         'p', POPT_ARG_STRING, &password,           0, "specify password for address of record", "password"},
      {"outbound-proxy",   'o', POPT_ARG_STRING, &inputOutboundProxy, 0, "specify uri for outbound proxy (if none present, don't use)", "sip:outbound.example.com"},
      {"contact",          'c', POPT_ARG_STRING, &inputContact,       0, "override default contact", "sip:alice@contact.example.com"},      
      {"enable-outbound",  'b', POPT_ARG_NONE,   &outboundEnabled,    0, "enable RFC 5626 outbound support", 0},

      {"subtarget",          0, POPT_ARG_STRING, &subscribeTarget,    0, "specify a SIP URI to subscribe to", "sip:bob@example.com"},
      {"calltarget",         0, POPT_ARG_STRING, &callTarget,         0, "specify a SIP URI to call", "sip:carol@example.com"},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   mLogType = logType;
   mLogLevel = logLevel;
   if (tlsDomain) mTlsDomain = tlsDomain;
   if (certPath) mCertPath = certPath;
   else mCertPath = basePath + "/.sipCerts";

   mUdpPort = udpPort;
   mTcpPort = tcpPort;
   mTlsPort = tlsPort;
   mDtlsPort = dtlsPort;
   mNoV4 = noV4 != 0;
   mEnableV6 = enableV6 != 0;
   mHostFileLookupOnlyDnsMode = hostFileLookupOnlyDnsMode != 0;
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

   mOutboundEnabled = outboundEnabled != 0;

   if(subscribeTarget)
   {
      mSubscribeTarget = toUri(subscribeTarget, "subscribe target");
   }

   if(callTarget)
   {
      mCallTarget = toUri(callTarget, "call target");
   }

   // Free the option parsing context.
#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
}

resip::Uri 
BasicClientCmdLineParser::toUri(const char* input, const char* description)
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

   
/* ====================================================================

 Copyright (c) 2011, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
