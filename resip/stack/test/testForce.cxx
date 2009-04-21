#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx" 
#endif

#include <iostream>

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include "resip/stack/SipStack.hxx"
//#include "resip/stack/UdpTransport.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/MethodHash.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   char* logType = 0;
   char* logLevel = 0;
   int sipPort = 5060;
   char * ruri = 0;
   char * toUri = 0;
   char * fromUri = 0;
   char * contactUri = 0;
   char * targetUri = 0;
   char * method = 0;
   int seltime = 500;

#if defined (HAVE_POPT_H) 
   struct poptOption table[] = {
       {"sip-port",   'p', POPT_ARG_INT,    &sipPort,   0, "Port for SIP stack to listen / send on.",0 },
       {"select-time", 'T', POPT_ARG_INT,    &seltime,  0, "Select upper bound in ms.", 0},
       {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
       {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
       {"ruri",        'r', POPT_ARG_STRING, &ruri,      0, "The Request-URI for this message", 0},
       {"to",          't', POPT_ARG_STRING, &toUri,     0, "The To: Header URI", 0},
       {"from",        'f', POPT_ARG_STRING, &fromUri,   0, "The From: Header URI", 0},
       {"ctc",         'm', POPT_ARG_STRING, &contactUri,0, "The Contact: URI", 0},
       {"target-uri",  'g', POPT_ARG_STRING, &targetUri, 0, "The target (forced) URI", 0},
       {"method",      'M', POPT_ARG_STRING, &method,    0, "The method to use", "REGISTER|INVITE|OPTIONS|MESSAGE|CANCEL|ACK|BYE"},
       POPT_AUTOHELP
       { 0, 0, 0, 0, 0 }
   };
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
   poptFreeContext(context);
#endif

   Log::initialize(logType, logLevel, argv[0]);
   
   if (!ruri)
   {
       ErrLog(<<"Not much to do with an RURI");
       return -1;
   }
   
   if (!toUri)
   {
       InfoLog(<<"No To URI, using URI." << ruri);
       toUri = ruri;
   }
   if (!fromUri)
   {
       InfoLog(<<"No From URI, using To URI." << toUri);
       fromUri = toUri;
   }

   MethodTypes meth(OPTIONS);

   if (method)
   {
       meth=getMethodType(method);
       if (meth == UNKNOWN)
       {
           ErrLog(<<"Unknown method, using OPTIONS for now: " << method );
           meth = OPTIONS;
       }
   }

   InfoLog(<<"Using method: " << resip::MethodNames[meth]);

   // resolve method.
   
   Fifo<Message> txFifo;

   auto_ptr<SipStack> stack( new SipStack );

   stack->addTransport(resip::UDP,sipPort);
   stack->addTransport(resip::TCP,sipPort);

   NameAddr ruriNA(ruri);
   
   NameAddr from(fromUri);
   NameAddr to(toUri);

   InfoLog (<< "Creating messages");

   auto_ptr<SipMessage> msg(new SipMessage);

   NameAddr contact;
   if (!contactUri)
   {
       InfoLog(<<"Warning: leaving contact: header empty.");
   }
   else
   {
       contact = NameAddr(contactUri);
       msg->header(h_Contacts).push_front(contact);
   }

   if (targetUri)
   {
       Uri forceTarget(targetUri);
       InfoLog(<<"Setting force target to " << forceTarget);
       msg->setForceTarget(forceTarget);
   }

   if (meth != UNKNOWN)
   {
       msg->header(h_RequestLine) = RequestLine(meth);
       msg->header(h_CSeq).method() = meth;
   }
   else
   {
       msg->header(h_RequestLine) = RequestLine(UNKNOWN);
       msg->header(h_RequestLine).unknownMethodName() = method;
       msg->header(h_CSeq).method() = UNKNOWN;
       msg->header(h_CSeq).unknownMethodName() = Data(method);
   }

   msg->header(h_RequestLine).uri() = ruriNA.uri();
   msg->header(h_To) = to;
   msg->header(h_From) = from;
   msg->header(h_CSeq).sequence() = 1;

   Via v; 
   msg->header(h_Vias).push_front(v);
   msg->header(h_CallId).value() = Helper::computeCallId();



   //DnsUtil::inet_pton("127.0.0.1", in);
   //Tuple dest(in, target.uri().port(), UDP);
   //InfoLog (<< "Sending to " << dest);

   InfoLog(<<"Sending Message: " << msg->brief());
   DebugLog(<<"Sending Message:" << endl << *msg);

   stack->send(*msg);
   bool done = false;
   while(!done)
   {
       FdSet fdset; 
       stack->buildFdSet(fdset);
       fdset.selectMilliSeconds(seltime); 
       stack->process(fdset);
       SipMessage* reply = stack->receive();
       if (!reply) continue;

       InfoLog(<<"Got reply: " << reply->brief());
       DebugLog(<<"Got reply:" << endl << *reply);
       ++done;
       
   }

   InfoLog (<< "Finished ");
   

   return 0;
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
