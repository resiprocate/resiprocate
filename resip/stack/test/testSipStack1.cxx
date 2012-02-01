#include <iostream>
#include <memory>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"

#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/ParserCategories.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
    Log::Level l = Log::Debug;
    
    if (argc > 1)
    {
        switch(*argv[1])
        {
            case 'd': l = Log::Debug;
                break;
            case 'i': l = Log::Info;
                break;
            case 's': l = Log::Stack;
                break;
            case 'c': l = Log::Crit;
                break;
        }
        
    }
    
    Log::initialize(Log::Cout, l, argv[0]);
    CritLog(<<"Test Driver Starting");

   initNetwork();
	
   try
   {
      SipStack sipStack;
      SipMessage* msg=NULL;

      InfoLog ( << "Try to send a message" );
	      
      NameAddr dest;
      NameAddr from;
      NameAddr contact;
      from.uri().scheme() = "sip";
      from.uri().user() = "fluffy";
      from.uri().host() = "localhost";
      from.uri().port() = 5060;
            
      dest = from;
      contact = from;
            
      auto_ptr<SipMessage> message = auto_ptr<SipMessage>(Helper::makeInvite( dest, from, contact));
      DebugLog ( << "Sending msg:" << *message );
      sipStack.send( *message );

      while (1)
      {
         //DebugLog ( << "Try TO PROCESS " );
         sipStack.process(25);

         //DebugLog ( << "Try TO receive " );
         msg = sipStack.receive();
         if ( msg )
         {
            DebugLog ( << "got message: " << *msg);
            msg->encode(resipCout);	  
         }
      }
   }
   catch (Transport::Exception& e)
   {
      InfoLog (<< "Failed to create sip stack" << e);
      exit(-1);
   }
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
