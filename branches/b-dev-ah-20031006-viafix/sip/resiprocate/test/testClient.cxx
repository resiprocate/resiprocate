#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Dialog.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class Client
{
    public:
      Client(TransportType transport, const NameAddr& contact, const NameAddr& target) 
         : mStack(),
           mContact(contact),
           mTarget(target),
           mWaitingForBye200(false)
      {
         mStack.addTransport(transport, contact.uri().port());
         auto_ptr<SipMessage> message(Helper::makeInvite( target, mContact, mContact));
         mStack.send(*message);
      }

      void buildFdSet(FdSet& fdset)
      {
         mStack.buildFdSet(fdset);
      }
      
      void process(FdSet& fdset)
      {
         mStack.process(fdset);
         
         SipMessage* received = mStack.receive();
         if (received)
         {
            InfoLog (<< "Client received: " << received->brief());
            
            auto_ptr<SipMessage> forDel(received);
            if ( (received->isResponse()) )
            {
               if ( received->header(h_StatusLine).responseCode() == 200 )
               {
                  if (!mWaitingForBye200)
                  {
                     ErrLog(<< "Creating dialog.");
                     Dialog dlog(mContact);
                     
                     DebugLog(<< "Creating dialog as UAC.");
                     dlog.createDialogAsUAC(*received);
                     
                     DebugLog(<< "making ack.");
                     auto_ptr<SipMessage> ack(dlog.makeAck(*received) );
                     DebugLog(<< *ack);

                     DebugLog(<< "making bye.");
                     auto_ptr<SipMessage> bye(dlog.makeBye());
                     
                     DebugLog(<< "Sending ack: << " << endl << *ack);
                     mStack.send(*ack);
                     
                     DebugLog(<< "Sending bye: << " << endl << *bye);
                     mStack.send(*bye);
                     mWaitingForBye200 = true;
                  }
                  else
                  {
                     auto_ptr<SipMessage> message(Helper::makeInvite( mTarget, mContact, mContact));
                     mStack.send(*message);
                     mWaitingForBye200 = false;
                  }
               }
            }
         }
      }
   private:
      SipStack mStack;
      NameAddr mContact;
      NameAddr mTarget;
      bool mWaitingForBye200;
};

int
main(int argc, char* argv[])
{
   if (argc != 3)
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI" << endl;
      exit(-1);
   } 
   Log::initialize(Log::COUT, Log::toLevel(argv[1]), argv[0]);

   NameAddr target(argv[2]);

   NameAddr contact;
   contact.uri().host() = SipStack::getHostname();
   contact.uri().port() = 5080;
   contact.uri().user() = "yffulf";

   TransportType protocol;
   if (isEqualNoCase(target.uri().param(p_transport), "UDP"))
   {
      protocol = UDP;
   }
   else if (isEqualNoCase(target.uri().param(p_transport), "TCP"))
   {
      protocol = TCP;
   }
   else
   {
      cerr << argv[0] << " LOG_LEVEL TARGET_URI(must include transport parameter)" << endl;
      exit(-1);
   }
   
   Client c(protocol, contact, target);

   while (true)
   {
      FdSet fdset;
      c.buildFdSet(fdset);
      fdset.selectMilliSeconds(0);
      c.process(fdset);
   }
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
