
#ifdef WIN32
#include "sip2/util/Socket.hxx"
#endif

#include "sip2/sipstack/SipStack.hxx"
#include "sip2/util/Data.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/Random.hxx"
#include "sip2/sipstack/TuIM.hxx"
#include "sip2/sipstack/Contents.hxx"
#include "sip2/sipstack/ParserCategories.hxx"
#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/sipstack/Pkcs7Contents.hxx"
#include "sip2/sipstack/Security.hxx"

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;


void
TuIM::PageCallback::receivedPage(const Data& msg, const Uri& from )
{
   assert(0);
}


void
TuIM::ErrCallback::sendPageFailed(const Uri& dest )
{
   assert(0);
}


TuIM::ErrCallback::~ErrCallback()
{
}


TuIM::PageCallback::~PageCallback()
{
}


TuIM::TuIM(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           PageCallback* msgCallback, 
           ErrCallback* errCallback)
   : mPageCallback(msgCallback),
     mErrCallback(errCallback),
     mStack(stack),
     mAor(aor),
     mContact(contact)
{
   assert( mStack );
   assert(mPageCallback);
   assert(mErrCallback);
}

      
void TuIM::sendPage(const Data& text, const Uri& dest )
{
   DebugLog( << "send to <" << dest << ">" << "\n" << text );

   NameAddr target;
   target.uri() = dest;
   
   NameAddr from;
   from.uri() = mAor;

   NameAddr contact;
   contact.uri() = mContact;
      
   SipMessage* msg = Helper::makeRequest(target, from, contact, MESSAGE);
   assert( msg );

#if 0
   PlainContents body(text);
   msg->setContents(&body);
#else
   Security* sec = mStack->security;
   assert(sec);
    
   PlainContents body(text);

   Pkcs7Contents* sBody = sec->sign( &body );

   //cerr << endl << endl << "Result encoded is -----------<";
   //sBody->encodeParsed( cerr );
   //cerr << ">--------------------------" << endl;
   
   msg->setContents( sBody );
#endif

   mStack->send( *msg );
}


void 
TuIM::process()
{
   assert( mStack );
   
   SipMessage* msg = mStack->receive();
   if ( msg )
   {
      DebugLog ( << "got message: " << *msg);
   
      if ( msg->isResponse() )
      { 
         int number = msg->header(h_StatusLine).responseCode();
         DebugLog ( << "got response of type " << number );

         if ( number >= 300 )
         {
            Uri dest = msg->header(h_To).uri();
            mErrCallback->sendPageFailed( dest );
         }
      }
      
      if ( msg->isRequest() )
      { 
         if ( msg->header(h_RequestLine).getMethod() == MESSAGE )
         {  
            NameAddr contact; 
            contact.uri() = mContact;
            
            SipMessage* response = Helper::makeResponse(*msg, 200, contact, "OK");

            mStack->send( *response );

            delete response;
            
            Contents* contents = msg->getContents();
            assert( contents );
            Mime mime = contents->getType();
            DebugLog ( << "got body of type  " << mime.type() << "/" << mime.subType() );

            PlainContents* body = dynamic_cast<PlainContents*>(contents);
            if ( body )
            {
               assert( body );
               const Data& text = body->text();
               
               Uri from = msg->header(h_From).uri();
               DebugLog ( << "got message from " << from );
               
               mPageCallback->receivedPage( text, from );
            }
            
            Pkcs7Contents* sBody = dynamic_cast<Pkcs7Contents*>(contents);
            if ( sBody )
            {
               assert( sBody );
               Security* sec = mStack->security;
               assert(sec);

               Contents* uBody = sec->uncode( sBody );
               
               if ( uBody )
               {
                  PlainContents* body = dynamic_cast<PlainContents*>(uBody);
                  if ( body )
                  {
                     const Data& text = body->text();
                     
                     Uri from = msg->header(h_From).uri();
                     DebugLog ( << "got message from " << from );
                     
                     mPageCallback->receivedPage( text, from );
                  }
                  else
                  {
                     assert(0);
                  }
               }
            }
             
            if ( (!body) && (!sBody) )
            {
               ErrLog ( << "Can not hangle type " << mime.type() << "/" << mime.subType() );
               assert(0);
            }
            
         }
      }

      delete msg;
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
