
#include <cassert>


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
#include "sip2/sipstack/Helper.hxx"

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;


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
           ErrCallback* errCallback,
           PresCallback* pressCallback)
   : mPageCallback(msgCallback),
     mErrCallback(errCallback),
     mPressCallback(pressCallback),
     mStack(stack),
     mAor(aor),
     mContact(contact),
     mPassword( Data::Empty ),
     mRegistrationDialog(NameAddr(contact)),
     mNextTimeToRegister(0)
{
   assert( mStack );
   assert(mPageCallback);
   assert(mErrCallback);
}


TuIM::PresCallback::~PresCallback()
{
}


void TuIM::sendPage(const Data& text, const Uri& dest, bool sign, const Data& encryptFor)
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

   PlainContents plainBody(text);
   Contents* body = &plainBody;
   
#if USE_SSL
   if ( sign )
   {
      Security* sec = mStack->security;
      assert(sec);
    
      Contents* old = body;
      body = sec->sign( body );
      if (old!=&plainBody) delete old;
   }
   
   if ( !encryptFor.empty() )
   {
      Security* sec = mStack->security;
      assert(sec);
      
      Contents* old = body;
      body = sec->encrypt( body, encryptFor );
      if (old!=&plainBody) delete old;
   }
#endif

   msg->setContents(body);
   mStack->send( *msg );

   if ( body != &plainBody )
   {
      delete body;
   }
}


void 
TuIM::process()
{
   assert( mStack );

   UInt64 now = Timer::getTimeMs();
   
   // check if register needs refresh
   if ( now > mNextTimeToRegister )
   {
      if ( mRegistrationDialog.isCreated() )
      {
         auto_ptr<SipMessage> msg( mRegistrationDialog.makeRegister() );
         mStack->send( *msg );
      }
      mNextTimeToRegister = Timer::getRandomFutureTimeMs( 10*60*1000 /*10 minutes*/ );
   }
   
   // check if any subscribes need refresh
   for ( int i=0; i<getNumBudies(); i++)
   {
      if (  now > mBuddy[i].mNextTimeToSubscribe )
      {
         Buddy& b = mBuddy[i];
         
         assert(  b.presDialog );
         if ( b.presDialog->isCreated() )
         {
            auto_ptr<SipMessage> msg( b.presDialog->makeSubscribe() );
            mStack->send( *msg );
         }
         mBuddy[i].mNextTimeToSubscribe = Timer::getRandomFutureTimeMs( 5*60*1000 /*5 minutes*/ );
      }
   }
   
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
            if ( !contents )
            {
               ErrLog( "Receiveed Message message with no contents" );
               delete msg; msg=0;
               return;
            }

            assert( contents );
            Mime mime = contents->getType();
            DebugLog ( << "got body of type  " << mime.type() << "/" << mime.subType() );

            Data signedBy;
            Security::SignatureStatus sigStat = Security::none;
            bool encrypted=false;

#ifdef USE_SSL
            Pkcs7Contents* sBody = dynamic_cast<Pkcs7Contents*>(contents);
            if ( sBody )
            {
               assert( sBody );
               Security* sec = mStack->security;
               assert(sec);

               contents = sec->uncode( sBody, &signedBy, &sigStat, &encrypted );
               if ( !contents )
               {
                  ErrLog( << "Some problem decoding SMIME message");
               }
            }
#endif

            if ( contents )
            {
               PlainContents* body = dynamic_cast<PlainContents*>(contents);
               if ( body )
               {
                  assert( body );
                  const Data& text = body->text();
                  DebugLog ( << "got message from with text of <" << text << ">" );
                 
                  Uri from = msg->header(h_From).uri();
                  DebugLog ( << "got message from " << from );
                  
                  assert( mPageCallback );
                  mPageCallback->receivedPage( text, from, signedBy, sigStat, encrypted );
               }
               else
               {
                  ErrLog ( << "Can not handle type " << contents->getType() );
                  assert(0);
               }
            }
         }
      }

      delete msg;
   }
}


void 
TuIM::registerAor( const Uri& uri, const Data& password )
{  
   mRegistrationPassword = password;
   
   //const NameAddr aorName;
   //const NameAddr contactName;
   //aorName.uri() = uri;
   //contactName.uri() = mContact;
   //SipMessage* msg = Helper::makeRegister(aorName,aorName,contactName);

   auto_ptr<SipMessage> msg( mRegistrationDialog.makeInitialRegister(NameAddr(uri),NameAddr(uri)) );
   
   mNextTimeToRegister = Timer::getRandomFutureTimeMs( 10*60*1000 /*10 minutes*/ );
   
   mStack->send( *msg );
}


int 
TuIM::getNumBudies() const
{
   return int(mBuddy.size());
}

const Uri 
TuIM::getBuddyUri(const int index)
{
   assert( index >= 0 );
   assert( index < getNumBudies() );

   return mBuddy[index].uri;
}

const Data 
TuIM::getBuddyGroup(const int index)
{
   assert( index >= 0 );
   assert( index < getNumBudies() );

   return mBuddy[index].group;
}

void 
TuIM::addBuddy( const Uri& uri, const Data& group )
{
   Buddy b;
   b.uri = uri;
   b.group = group;
   b.presDialog = new Dialog( NameAddr(mContact) );
   assert( b.presDialog );
   
   mBuddy.push_back( b );

   // subscribe to this budy 
   auto_ptr<SipMessage> msg( mRegistrationDialog.makeInitialRegister(NameAddr(b.uri),NameAddr(b.uri)) );
   b.mNextTimeToSubscribe = Timer::getRandomFutureTimeMs( 10*60*1000 /*5 minutes*/ );
   
   mStack->send( *msg );
}

void 
TuIM::removeBudy( const Uri& name)
{
   assert(0);
}

void 
TuIM::setMyPresense( const bool open, const Data& status )
{
   assert(0);
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
