#include <popt.h>

#include "tfm/remoteproxy/CommandLineParser.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseException.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

CommandLineParser::CommandLineParser(int argc, char** argv)
{
   char* logType = "cout";
   char* logLevel = "INFO";
   char* proxyHostName=0;
   char* userIPAddr=0;
   char* udpPorts = "5060";
   char* tcpPorts = "5060";
   char* tlsPorts = "5061";
   char* dtlsPorts = 0;
   int redirectServer=0;


   struct poptOption table[] = {
      {"log-type",     'l',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",    'v',  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARN|ERROR|FATAL"},
      {"host",   0,  POPT_ARG_STRING, &proxyHostName,  0, "test a proxy running at a given hostname", "example.com"},
      {"userIPAddr",   0,  POPT_ARG_STRING, &userIPAddr,  0, "User agents will bind to this interface", "192.168.0.12"},
      {"udp",            0,  POPT_ARG_STRING| POPT_ARGFLAG_SHOW_DEFAULT,    &udpPorts, 0, "udp ports that we expect the proxy to be listening on", "5060"},
      {"tcp",            0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &tcpPorts, 0, "tcp ports that we expect the proxy to be listening on", "5060"},
      {"tls",            0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &tlsPorts, 0, "tls ports that we expect the proxy to be listening on", "5061"},
      {"dtls",           0,  POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,    &dtlsPorts, 0, "dtls ports that we expect the proxy to be listening on", "none (dtls disabled)"},
      {"enable-redirect-server", 0,POPT_ARG_NONE, &redirectServer, 0, "is the proxy running as a redirect server?", 0},      
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);

   mLogType = logType;
   mLogLevel = logLevel;
   //Log::initialize(mLogType, mLogLevel, argv[0]);

   if(proxyHostName)
   {
      mProxyHostName=proxyHostName;
   }
   
   if(userIPAddr)
   {
      mUserIPAddr=userIPAddr;
   }
   
   mUdpPorts = toIntSet(udpPorts, "udp ports");
   mTcpPorts = toIntSet(tcpPorts, "tcp ports");
   mTlsPorts = toIntSet(tlsPorts, "tls ports");
   mDtlsPorts = toIntSet(dtlsPorts, "dtls ports");
   mRedirectServer = redirectServer!=0;

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
 
      }
   } 
   catch (ParseException& e)
   {


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
            catch (ParseException& e)
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


/* Copyright 2007 Estacado Systems */

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
