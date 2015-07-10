#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

/* TODO list for this file ....
   handle 302 in message
   handle 302 in subscribe

   sort out how contact is formed 
   keep track of ourstanding message dialogs 
   add a publish sending dialogs 
   send options to register, see if do publish, if so use 

   suppport loading destination certificates from server 
*/

#include "rutil/ResipAssert.h"
#include <memory>

#include "resip/stack/SipStack.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TuIM.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/CpimContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/OctetContents.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/SipFrag.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Socket.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
using namespace std;

static int IMMethodList[] = { (int) REGISTER, 
                              (int) MESSAGE, 
	                      (int) SUBSCRIBE, 
                              (int) NOTIFY };
const int IMMethodListSize = sizeof(IMMethodList) / sizeof(*IMMethodList);


TuIM::Callback::~Callback()
{
}


TuIM::TuIM(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           Callback* callback,
           const int registrationTimeSeconds,
           const int subscriptionTimeSeconds)
   : mCallback(callback),
     mStack(stack),
     mAor(aor),
     mContact(contact),
     mPidf( new Pidf ),
     mRegistrationDialog(NameAddr(contact)),
     mNextTimeToRegister(0),
     mRegistrationPassword( Data::Empty ),
     mLastAuthCSeq(0),
     mRegistrationTimeSeconds(registrationTimeSeconds),
     mSubscriptionTimeSeconds(subscriptionTimeSeconds),
     mDefaultProtocol( UNKNOWN_TRANSPORT )
{
   resip_assert( mStack );
   resip_assert(mCallback);
   resip_assert(mPidf);
   
   mPidf->setSimpleId( Random::getRandomHex(4) );  
   mPidf->setEntity(mAor);  
   mPidf->setSimpleStatus( true, Data::Empty, mContact.getAor() );
}


bool
TuIM::haveCerts( bool sign, const Data& encryptFor )
{
/*   assert(0); */

#if defined( USE_SSL )
   Security* sec = mStack->getSecurity();
   resip_assert(sec);
   
   if ( sign )
   {    
      if ( !sec->hasUserPrivateKey(mAor.getAor()) )
      {
         return false;
      }
   } 
   if ( !encryptFor.empty() )
   {
      if ( !sec->hasUserCert( encryptFor ) )
      {
         return false;
      }
   }
#else
   if ( (!encryptFor.empty()) || (sign) )
   {
      return false;
   }
#endif
   return true;
}


void 
TuIM::sendPage(const Data& text, const Uri& dest, 
               const bool sign, const Data& encryptFor)
{
   if ( text.empty() )
   {
      DebugLog( << "tried to send blank message - dropped " );
      return;
   }
   DebugLog( << "send to <" << dest << ">" << "\n" << text );

   NameAddr target;
   target.uri() = dest;
   
   NameAddr from;
   from.uri() = mAor;

   NameAddr contact;
   contact.uri() = mContact;

   DeprecatedDialog* dialog = new DeprecatedDialog( NameAddr(mContact) );
 
   auto_ptr<SipMessage> msg( dialog->makeInitialMessage(NameAddr(target),NameAddr(from)) );
 
   Page page;
   page.text = text;
   page.uri = dest;
   page.sign = sign;
   page.encryptFor = encryptFor;
   page.dialog = dialog;
   
   mPages.push_back(page);
   
   Contents* body = ( new PlainContents(text) );
#if 1
   msg->header(h_ContentTransferEncoding) = StringCategory(Data("binary"));
#endif

#if defined( USE_SSL )
   if ( !encryptFor.empty() )
   {
      Security* sec = mStack->getSecurity();
      resip_assert(sec);
      
      Contents* old = body;
      old->header(h_ContentTransferEncoding) = msg->header(h_ContentTransferEncoding);
      body = sec->encrypt( old, encryptFor );
      delete old;

      if ( !body )
      {
         mCallback->sendPageFailed( dest, -2 );
         return;
      }

#if 0      
      msg->header(h_ContentTransferEncoding) = StringCategory(Data("binary"));
#endif
   }

   if ( sign )
   {
      Security* sec = mStack->getSecurity();
      resip_assert(sec);
    
      Contents* old = body;
      old->header(h_ContentTransferEncoding) = msg->header(h_ContentTransferEncoding);
      body = sec->sign( mAor.getAor(), old );
      delete old;

      if ( !body )
      {
         mCallback->sendPageFailed( dest, -1 );
         return;
      }

#if 0
      msg->header(h_ContentTransferEncoding) = StringCategory(Data("binary"));
#endif
   }
#endif

   msg->setContents(body);

   if (1)       // Compute the identity header.
   {
      //Security* sec = mStack->getSecurity();
      //assert(sec);
      
      DateCategory now;
      msg->header(h_Date) = now;
      
      //Data token = msg->getCanonicalIdentityString();
      //Data res = sec->computeIdentity( token );

      msg->header(h_Identity).value() = Data::Empty;
   }
   
   setOutbound( *msg );
   //ErrLog( "About to send " << *msg );
   
   mStack->send( *msg );

   delete body;
}


void
TuIM::processRequest(SipMessage* msg)
{
   if ( msg->header(h_RequestLine).getMethod() == MESSAGE )
   {
      processMessageRequest(msg);
      return;
   }
   if ( msg->header(h_RequestLine).getMethod() == SUBSCRIBE )
   {
      processSubscribeRequest(msg);
      return;
   }
   if ( msg->header(h_RequestLine).getMethod() == REGISTER )
   {
      processRegisterRequest(msg);
      return;
   }
   if ( msg->header(h_RequestLine).getMethod() == NOTIFY )
   {
      processNotifyRequest(msg);
      return;
   }

   InfoLog(<< "Don't support this METHOD, send 405" );
   
   SipMessage * resp = Helper::make405( *msg, IMMethodList, IMMethodListSize ); 
   mStack->send(*resp); 
   delete resp;
}


void 
TuIM::processSipFrag(SipMessage* msg)
{
   Contents* contents = msg->getContents();
   if ( !contents )
   {
      InfoLog(<< "Received message with no contents" );
      return;
   }

   InfoLog(<< "Received message with body contents" );

   resip_assert( contents );
   Mime mime = contents->getType();
   DebugLog ( << "got body of type  " << mime.type() << "/" << mime.subType() );

   Data signedBy;

#if defined( USE_SSL )
   SignatureStatus sigStat = SignatureNone;
   MultipartSignedContents* mBody = dynamic_cast<MultipartSignedContents*>(contents);
   if ( mBody )
   {
      Security* sec = mStack->getSecurity();
      resip_assert(sec);
      
      contents = sec->checkSignature( mBody, &signedBy, &sigStat );
      
      if ( !contents )
      { 
         InfoLog( << "Some problem decoding multipart/signed message");
         return;
      } 

      InfoLog( << "Signed by " << signedBy << " stat = " << sigStat );
   }
#endif

   if ( contents )
   {
      MultipartMixedContents* mixed = dynamic_cast<MultipartMixedContents*>(contents);
      if ( mixed )
      {
         InfoLog( << "Got a multipart mixed" );

         contents = NULL;
         
         MultipartMixedContents::Parts& parts = mixed->parts();
         for( MultipartMixedContents::Parts::const_iterator i = parts.begin(); 
              i != parts.end();  
              ++i)
         {
            Contents* c = *i;  
            resip_assert( c );
            InfoLog ( << "mixed has a " << c->getType() );

            if ( c->getType() == Mime("application","sipfrag") )
            {
               InfoLog ( << "mixed has sipfrag " << c->getType() );
           
               SipFrag* frag = dynamic_cast<SipFrag*>(c);
               if ( frag )
               {
                  InfoLog( << "Got a sipFrag inside mixed" );
                  
                  SipMessage& m = frag->message();
                  
                  InfoLog( <<"Frag is " << m );
               }
             }
         }
      }
   }

   if ( contents )
   {
      SipFrag* frag = dynamic_cast<SipFrag*>(contents);
      if ( frag )
      {
         InfoLog( << "Got a sipFrag" );

         SipMessage& m = frag->message();
         
         InfoLog( <<"Frag is " << m );
      }
      else
      {
         InfoLog ( << "Can not handle type " << contents->getType() );

         return;
      }
   }
}


void 
TuIM::processSubscribeRequest(SipMessage* msg)
{
   resip_assert( msg->header(h_RequestLine).getMethod() == SUBSCRIBE );
   CallId id = msg->header(h_CallId);
   
   processSipFrag( msg );
   
   int expires=mSubscriptionTimeSeconds;
   if ( msg->exists(h_Expires) )
   {
      expires = msg->header(h_Expires).value();
   }
   if (expires > mSubscriptionTimeSeconds )
   {
      expires = mSubscriptionTimeSeconds;
   }
   
   DeprecatedDialog* dialog = NULL;

   // see if we already have this subscription
   for ( SubscriberIterator i=mSubscribers.begin(); i != mSubscribers.end(); i++)
   {
      DeprecatedDialog* d = i->dialog;
      resip_assert( d );
      
      if ( d->getCallId() == id )
      {
         // found the subscrition in list of current subscrbtions 
         dialog = d;
         break;
      }
   }
   
   if ( !dialog )
   {
      // create a new subscriber 
      DebugLog ( << "Creating new subscrition dialog ");

      Subscriber s;
      
      s.dialog = new DeprecatedDialog( NameAddr(mContact) );
      dialog = s.dialog;
      
      Uri from = msg->header(h_From).uri();
      s.aor = from.getAorNoPort();

      resip_assert( mCallback );
      s.authorized = mCallback->authorizeSubscription( from );
      
      mSubscribers.push_back( s );
   }

   resip_assert( dialog );
   dialog->setExpirySeconds( expires );
   
   auto_ptr<SipMessage> response( dialog->makeResponse( *msg, 200 ));
 
   response->header(h_Expires).value() = expires;
   response->header(h_Event).value() = Data("presence");
   
   mStack->send( *response );

   sendNotify( dialog );

#if 1
   // do symetric subscriptions 
   // See if this person is our buddy list and if we are not subscribed to them

    UInt64 now = Timer::getTimeMs();
    Uri from = msg->header(h_From).uri();

    for ( BuddyIterator i=mBuddies.begin(); i != mBuddies.end(); i++)
    {
       Data buddyAor = i->uri.getAor();
       
       if ( ! (i->presDialog->isCreated()) )
       {
          if (  from.getAor() == i->uri.getAor() )
          {
	    if ( from.getAor() != mAor.getAor() )
	    {
              i->mNextTimeToSubscribe = now;
	    }
          }
       }
    }
#endif
}


void 
TuIM::processRegisterRequest(SipMessage* msg)
{
   resip_assert( msg->header(h_RequestLine).getMethod() == REGISTER );
   CallId id = msg->header(h_CallId);

   int expires = msg->header(h_Expires).value();
   if ( expires == 0 ) 
   { 
      expires = 3600;
   } 

   SipMessage* response = Helper::makeResponse( *msg, 200 );

   // the Contacts from the default Helper are wrong for a Registration
   response->remove(h_Contacts);
   
   if ( msg->exists(h_Contacts) )
   {
      ParserContainer<NameAddr> &providedContacts(msg->header(h_Contacts));

      int multipleContacts = (int)providedContacts.size();

      DebugLog ( << multipleContacts << " contacts were in received message." );   

      ParserContainer<NameAddr>::iterator i(providedContacts.begin());
      for ( ; i != providedContacts.end() ; ++ i) {
         if ( i->isAllContacts() && multipleContacts )  // oops, multiple Contacts and one is "*"
         {
            delete response;  // do I need to do this?
            response = Helper::makeResponse( *msg, 400 );
            mStack->send( *response );
            delete response;
            return;
         }
         if ( !i->exists(p_expires) )  // add expires params where they don't exist
         {
            i->param(p_expires) = expires;
         }
         response->header(h_Contacts).push_back(*i);   // copy each Contact into response
      }
   }
   // else the REGISTER is a query, just send the message with no Contacts

   mStack->send( *response );

   delete response;
}


void 
TuIM::processNotifyRequest(SipMessage* msg)
{
   resip_assert( mCallback );
   resip_assert( msg->header(h_RequestLine).getMethod() == NOTIFY ); 

   processSipFrag( msg );

   auto_ptr<SipMessage> response( Helper::makeResponse( *msg, 200 ));
   mStack->send( *response );

   Uri from = msg->header(h_From).uri();
   DebugLog ( << "got notify from " << from );

   Contents* contents = msg->getContents();
   if ( !contents )
   {
      InfoLog(<< "Received NOTIFY message event with no contents" );
      mCallback->presenceUpdate( from, true, Data::Empty );
      return;
   }

   Mime mime = contents->getType();
   DebugLog ( << "got  NOTIFY event with body of type  " << mime.type() << "/" << mime.subType() );
  
   Pidf* body = dynamic_cast<Pidf*>(contents);
   if ( !body )
   {
      InfoLog(<< "Received NOTIFY message event with no PIDF contents" );
      mCallback->presenceUpdate( from, true, Data::Empty );
      return;
   }
 
   Data note;
   bool open = body->getSimpleStatus( &note );

   bool changed = true;


   // update if found in budy list 
   for ( BuddyIterator i=mBuddies.begin(); i != mBuddies.end(); i++)
   {
      Uri u = i->uri; // getBuddyUri(i);
      
      if ( u.getAor() == from.getAor() )
      {

         if ( (i->status == note) &&
              (i->online == open) )
         {
            changed = false;
         }
         
         i->status = note;
         i->online = open;
      }
   }
   
   InfoLog(<< "Processed NOTIFY message : Presence changed: " << changed );
   // notify callback 
   if (changed)
   {
      resip_assert(mCallback);
      mCallback->presenceUpdate( from, open, note );
   }
}


void 
TuIM::processMessageRequest(SipMessage* msg)
{
   resip_assert( msg );
   resip_assert( msg->header(h_RequestLine).getMethod() == MESSAGE );
   
   NameAddr contact; 
   contact.uri() = mContact;
            
   SipMessage* response = Helper::makeResponse(*msg, 200, contact, "OK");
   mStack->send( *response );
   delete response; response=0;
               
   Contents* contents = msg->getContents();
   if ( !contents )
   {
      InfoLog(<< "Received Message message with no contents" );
      // .kw. NO: delete msg; msg=0; // our caller owns msg
      return;
   }

   Mime mime = contents->getType();
   DebugLog ( << "got body of type  " << mime.type() << "/" << mime.subType() );

   Data signedBy;
   SignatureStatus sigStat = SignatureNone;
   bool encrypted=false;

#if defined( USE_SSL )
   Uri from = msg->header(h_From).uri();
   signedBy = from.getAorNoPort();
   
   InfoLog ( << "assuming signedBy is " << signedBy );
   
   MultipartSignedContents* mBody = dynamic_cast<MultipartSignedContents*>(contents);
   if ( mBody )
   {
      Security* sec = mStack->getSecurity();
      resip_assert(sec);
      
      contents = sec->checkSignature( mBody, &signedBy, &sigStat );
      
      //ErrLog("Signed by " << signedBy << " stat = " << sigStat );
      
      if ( !contents )
      { 
         Uri from = msg->header(h_From).uri();
         InfoLog( << "Some problem decoding multipart/signed message");
         
         mCallback->receivePageFailed( from );
         return;
      } 
   }
  
   Pkcs7SignedContents* sBody = dynamic_cast<Pkcs7SignedContents*>(contents);
   if ( sBody )
   {
      resip_assert( sBody );
      Security* sec = mStack->getSecurity();
      resip_assert(sec);

      contents = sec->decrypt( mAor.getAor(), sBody );

      if ( !contents )
      { 
         Uri from = msg->header(h_From).uri();
         InfoLog( << "Some problem decoding signed SMIME message");
        
         mCallback->receivePageFailed( from );
         return;
      }

      encrypted=true;
   }

   Pkcs7Contents* eBody = dynamic_cast<Pkcs7Contents*>(contents);
   if ( eBody )
   {
      resip_assert( eBody );
      Security* sec = mStack->getSecurity();
      resip_assert(sec);

      contents = sec->decrypt( mAor.getAor(), eBody );

      if ( !contents )
      { 
         Uri from = msg->header(h_From).uri();
         InfoLog( << "Some problem decoding SMIME message");
        
         mCallback->receivePageFailed( from );
         return;
      }

      encrypted=true;
   }
 
#endif

   if ( contents )
   {
      PlainContents* plain = dynamic_cast<PlainContents*>(contents);
      if ( plain )
      {
         resip_assert( plain );
         const Data& text = plain->text();
         DebugLog ( << "got message from with text of <" << text << ">" );
                 
         Uri from = msg->header(h_From).uri();
         DebugLog ( << "got message from " << from );
                  
         resip_assert( mCallback );
         mCallback->receivedPage( text, from, signedBy, sigStat, encrypted );
         return;
      }
 
      CpimContents* cpim = dynamic_cast<CpimContents*>(contents);
      if ( cpim )
      {
         resip_assert( cpim );
         const Data& text = cpim->text();
         DebugLog ( << "got CPIM message from with text of <" << text << ">" );
                 
         // !cj! TODO - should get from out of CPIM message 
         Uri from = msg->header(h_From).uri();
         DebugLog ( << "got message from " << from );
                  
         resip_assert( mCallback );
         mCallback->receivedPage( text, from, signedBy, sigStat, encrypted );
         return;
      }
 
      MultipartMixedContents* mixed = dynamic_cast<MultipartMixedContents*>(contents);
      if ( mixed )
      {
         InfoLog( << "Got a multipart mixed" );
       
         contents = NULL;
         
         MultipartMixedContents::Parts& parts = mixed->parts();
         for( MultipartMixedContents::Parts::const_iterator i = parts.begin(); 
              i != parts.end();  
              ++i)
         {
            Contents* c = *i;  
            resip_assert( c );
            InfoLog ( << "mixed has a " << c->getType() );

            if ( c->getType() == Mime("text","plain") )
            {
               InfoLog ( << "mixed has sipfrag " << c->getType() );
      
               PlainContents* plainBody = dynamic_cast<PlainContents*>(c);
               if ( plainBody )
               {
                  resip_assert( plainBody );
                  const Data& text = plainBody->text();
                  DebugLog ( << "got message from with text of <" << text << ">" );
                  
                  Uri from = msg->header(h_From).uri();
                  DebugLog ( << "got message from " << from );
                  
                  resip_assert( mCallback );
                  mCallback->receivedPage( text, from, signedBy, sigStat, encrypted );
                  return;
               }

               // !cj! TODO - should deal with CPIM too 
            }
            
         }
         
         return;
      }
   
#if 1   // !cj! TODO remove 
      OctetContents* octets = dynamic_cast<OctetContents*>(contents);
      if (octets)
      {
         resip_assert( contents );
         const Data& text = octets->getBodyData();
         DebugLog ( << "got message from with text of <" << text << ">" );
         
         Uri from = msg->header(h_From).uri();
         DebugLog ( << "got message from " << from );
                  
         resip_assert( mCallback );
         mCallback->receivedPage( text, from, signedBy, sigStat, encrypted );
         return;
      }
#endif

      // deal with it if no one else has 
      {
         InfoLog ( << "Can not handle type " << contents->getType() );
         Uri from = msg->header(h_From).uri();
         mCallback->receivePageFailed( from );
         return;
      }
   }
}


void
TuIM::processResponse(SipMessage* msg)
{  
   resip_assert( msg->exists(h_CallId));
   CallId id = msg->header(h_CallId);
   resip_assert( id.value() != Data::Empty );

   processSipFrag( msg );
   
   // see if it is a registraition response 
   CallId regId = mRegistrationDialog.getCallId(); 

   Data v1 = id.value();
   Data v2 = regId.value();

   InfoLog( << "want id =" << id );

   if ( id == regId )
   {
      InfoLog ( << "matched the reg dialog" 
                 <<  mRegistrationDialog.getCallId() << " = " << id  );
      processRegisterResponse( msg );
      return;
   }
   
   // see if it is a subscribe response 
   for ( BuddyIterator i=mBuddies.begin(); i != mBuddies.end(); i++)
   {
      Buddy& buddy = *i;
      resip_assert(  buddy.presDialog );
      InfoLog( << "check buddy id =" <<  buddy.presDialog->getCallId() );
      if ( buddy.presDialog->getCallId() == id  )
      {
         DebugLog ( << "matched the subscribe dialog" );
         processSubscribeResponse( msg, buddy );
         return;
      }
   }
   
   // see if it is a publish response
   for ( StateAgentIterator i=mStateAgents.begin(); i != mStateAgents.end(); i++)
   {
      resip_assert( i->dialog );
      InfoLog( << "check publish id =" <<  i->dialog->getCallId() );
      if ( i->dialog->getCallId() == id  )
      {
         DebugLog ( << "matched the publish dialog" );
         processPublishResponse( msg, *i );
         return;
      }
   }
   
   // see if it is a notify response
   for ( SubscriberIterator i=mSubscribers.begin(); i != mSubscribers.end(); i++)
   {
      DeprecatedDialog* dialog = i->dialog;
      resip_assert( dialog );
      InfoLog( << "check subscriber id =" <<  dialog->getCallId() );
      if ( dialog->getCallId() == id  )
      {
         DebugLog ( << "matched the notify dialog" );
         processNotifyResponse( msg, *dialog );
         return;
      }
   }
   
   // see if it is a page response
   for ( PageIterator i=mPages.begin(); i != mPages.end(); i++)
   {
      resip_assert( i->dialog ); 
      InfoLog( << "check page id =" <<  i->dialog->getCallId() );
      if ( i->dialog->getCallId() == id  )
      {
         DebugLog ( << "matched the MESSAGE dialog" );
         processPageResponse( msg, *i );
         return;
      }
   }
  
   int number = msg->header(h_StatusLine).responseCode();
   InfoLog( << "got response that DID NOT MATCH of type " << number );
}


void 
TuIM::processRegisterResponse(SipMessage* msg)
{
   int number = msg->header(h_StatusLine).responseCode();
   Uri to = msg->header(h_To).uri();
   InfoLog ( << "register of " << to << " got response " << number );   
   unsigned int cSeq = msg->header(h_CSeq).sequence();

   if ( number<200 )
   {
      return;
   }

   if ( number >= 200 )
   { 
      mRegistrationDialog.createDialogAsUAC( *msg );
   }
   
   if ( ((number == 401) || (number == 407)) && (cSeq != mLastAuthCSeq) )
   {
      SipMessage* reg = mRegistrationDialog.makeRegister();
      
      const Data cnonce = Data::Empty;
      unsigned int nonceCount=0;
       
      Helper::addAuthorization(*reg,*msg,mAor.user(),mRegistrationPassword,cnonce,nonceCount);

      mLastAuthCSeq = reg->header(h_CSeq).sequence();

      reg->header(h_Expires).value() = mRegistrationTimeSeconds;
      reg->header(h_Contacts).front().param(p_expires) = mRegistrationTimeSeconds;
   
      mNextTimeToRegister = Timer::getRandomFutureTimeMs( mRegistrationTimeSeconds*1000 );

      InfoLog( << *reg );
      
      setOutbound( *reg );

      mStack->send( *reg );

      delete reg; reg=NULL;
      
      return;
   }
   
   if ( number >= 300 )
   {
      resip_assert( mCallback );
      mCallback->registrationFailed( to, number );
      return;
   }
   
   if ( (number>=200) && (number<300) )
   {
      int expires = mRegistrationTimeSeconds;

      if ( msg->exists(h_Expires) )
      {
         expires = msg->header(h_Expires).value();
      }
      
      // loop throught the contacts, find me, and extract expire time
      resip::ParserContainer<resip::NameAddr>::iterator i =  msg->header(h_Contacts).begin();
      while ( i != msg->header(h_Contacts).end() )
      {
         try
         { 
            Uri uri = i->uri();

            if ( uri.getAor() == mContact.getAor() )
            {
               int e = i->param(p_expires);
               DebugLog(<< "match " << uri.getAor() << " e=" << e );

               expires = e;
            }
         }
         catch ( exception* )
         {
            InfoLog(<< "Bad contact in 2xx to register - skipped" );
         }
         
         i++;
      }

      if ( expires < 15 )
      {
         InfoLog(<< "Got very small expiers of " << expires );
         expires = 15;
      }

      mNextTimeToRegister = Timer::getRandomFutureTimeMs( expires*1000 );
      
      mCallback->registrationWorked( to );

      return;
   }
}


void 
TuIM::processNotifyResponse(SipMessage* msg, DeprecatedDialog& d )
{ 
   int number = msg->header(h_StatusLine).responseCode();
   DebugLog( << "got NOTIFY response of type " << number );
   
   if ( number >= 300 )
   {
      // TODO 
   }
}


void 
TuIM::processPublishResponse(SipMessage* msg, StateAgent& sa )
{ 
   int number = msg->header(h_StatusLine).responseCode();
   DebugLog( << "got PUBLISH response of type " << number );
   
   if ( number >= 300 )
   {
      // TODO
   }
}


void 
TuIM::processPageResponse(SipMessage* msg, Page& page )
{
   int number = msg->header(h_StatusLine).responseCode();
   DebugLog( << "got MESSAGE response of type " << number );
   
   if ( number >= 400 )
   {
      Uri dest = msg->header(h_To).uri();
      resip_assert( mCallback );
      mCallback->sendPageFailed( dest,number );
   }

   if ( (number>=300) && (number<400) )
   {
      ParserContainer<NameAddr>::iterator dest =  msg->header(h_Contacts).begin();
      while ( dest != msg->header(h_Contacts).end() )
      {
         DebugLog(<< "Got a 3xx to" << *dest  );

         // send a message to redirected location
         Uri uri = dest->uri();
         sendPage( page.text, uri, page.sign, page.encryptFor );
         
         dest++;
      }
   }

   if (  (number>=200) && (number<300) )
   { 
      // got a final response for notify - can remove this dialog information
      CallId id = msg->header(h_CallId);

      for( PageIterator i=mPages.begin(); i != mPages.end(); i++ )
      {
         if ( i->dialog->getCallId() == id )
         {
            i = mPages.erase( i );
         }
      }
   }
}


void 
TuIM::processSubscribeResponse(SipMessage* msg, Buddy& buddy)
{
   int number = msg->header(h_StatusLine).responseCode();
   Uri to = msg->header(h_To).uri();
   InfoLog ( << "subscribe got response " << number << " from " << to );   

   if ( (number>=200) && (number<300) )
   {
      int expires = mSubscriptionTimeSeconds;
      if ( msg->exists(h_Expires) )
      {
         expires = msg->header(h_Expires).value();
      }
      if ( expires < 15 )
      {
         InfoLog( << "Got very small expiers of " << expires );
         expires = 15;
      } 
      
      resip_assert( buddy.presDialog );
      buddy.presDialog->createDialogAsUAC( *msg );
      
      buddy.mNextTimeToSubscribe = Timer::getRandomFutureTimeMs( expires*1000 );
   }

   if (  (number>=300) && (number<400) )
   {
      ParserContainer<NameAddr>::iterator dest =  msg->header(h_Contacts).begin();
      while ( dest != msg->header(h_Contacts).end() )
      {
         DebugLog(<< "Got a 3xx to" << *dest  );
         Uri uri = dest->uri();

         addBuddy( uri , buddy.group );

         buddy.mNextTimeToSubscribe = Timer::getForever();
 
         dest++;
      }
   }
   
   if (  (number>=400) )
   {
      DebugLog( << "Got an error to some subscription" );     

      // take this buddy off line 
      Uri to = msg->header(h_To).uri();
      resip_assert( mCallback );
      
      bool changed = true;
      
      for ( BuddyIterator i=mBuddies.begin(); i != mBuddies.end(); i++)
      {
         Uri u = i->uri; // getBuddyUri(i);
         
         if ( u.getAor() == to.getAor() )
         {
            if (  i->online == false )
            {  
               changed = false;
            }
            
            i->online = false;
         }
      }

      if ( changed )
      {
         mCallback->presenceUpdate( to, false, Data::Empty );
      }
      
      // try to contact this buddy again later
      buddy.mNextTimeToSubscribe = Timer::getRandomFutureTimeMs( mSubscriptionTimeSeconds*1000 );
   }
}


void 
TuIM::process()
{
   resip_assert( mStack );

   UInt64 now = Timer::getTimeMs();
   
   // check if register needs refresh
   if ( now > mNextTimeToRegister )
   {
      if ( mRegistrationDialog.isCreated() )
      {
         auto_ptr<SipMessage> msg( mRegistrationDialog.makeRegister() );
         msg->header(h_Expires).value() = mRegistrationTimeSeconds;
         setOutbound( *msg );
         mStack->send( *msg );
      }
      mNextTimeToRegister = Timer::getRandomFutureTimeMs( mRegistrationTimeSeconds*1000 );
   }
   
   // check if any subscribes need refresh
   for ( BuddyIterator i=mBuddies.begin(); i != mBuddies.end(); i++)
   {
      if (  now > i->mNextTimeToSubscribe )
      {
         Buddy& buddy = *i;
         
         buddy.mNextTimeToSubscribe 
                    = Timer::getRandomFutureTimeMs( mSubscriptionTimeSeconds*1000 );
         
         resip_assert(  buddy.presDialog );
         if ( buddy.presDialog->isCreated() )
         {
            auto_ptr<SipMessage> msg( buddy.presDialog->makeSubscribe() );
                        
            msg->header(h_Event).value() = Data("presence");;
            msg->header(h_Accepts).push_back( Mime( "application","pidf+xml") );
            msg->header(h_Expires).value() = mSubscriptionTimeSeconds;
            
            setOutbound( *msg );

            mStack->send( *msg );
         }
         else
         {
            // person was not available last time - try to subscribe now
            subscribeBuddy( buddy );
         }
      }
   }

   // TODO - go and clean out any subscrption to us that have expired

   // check for any messages from the sip stack 
   SipMessage* msg( mStack->receive() );
   if ( msg )
   {
      DebugLog ( << "got message: " << *msg);
   
      if ( msg->isResponse() )
      { 
         processResponse( msg );
      }
      
      if ( msg->isRequest() )
      {
         processRequest( msg );
      }

      delete msg; msg=0;
   }
}


void 
TuIM::registerAor( const Uri& uri, const Data& password  )
{  
   mRegistrationPassword = password;
   
   //const NameAddr aorName;
   //const NameAddr contactName;
   //aorName.uri() = uri;
   //contactName.uri() = mContact;
   //SipMessage* msg = Helper::makeRegister(aorName,aorName,contactName);

   auto_ptr<SipMessage> msg( mRegistrationDialog.makeInitialRegister(NameAddr(uri),NameAddr(uri)) );

   msg->header(h_Expires).value() = mRegistrationTimeSeconds;
   msg->header(h_Contacts).front().param(p_expires) = mRegistrationTimeSeconds;
   
   Token t;
   t = Token(Data("presence"));
   msg->header(h_AllowEvents).push_back( t );
   
   mNextTimeToRegister = Timer::getRandomFutureTimeMs( mRegistrationTimeSeconds*1000 );
   
   setOutbound( *msg );

   mStack->send( *msg );
}


int 
TuIM::getNumBuddies() const
{
   return int(mBuddies.size());
}


const Uri 
TuIM::getBuddyUri(const int index)
{
   resip_assert( index >= 0 );
   resip_assert( index < getNumBuddies() );

   return mBuddies[index].uri;
}


const Data 
TuIM::getBuddyGroup(const int index)
{
   resip_assert( index >= 0 );
   resip_assert( index < getNumBuddies() );

   return mBuddies[index].group;
}


bool 
TuIM::getBuddyStatus(const int index, Data* status)
{ 
   resip_assert( index >= 0 );
   resip_assert( index < getNumBuddies() );

   if (status)
   {
      *status =  mBuddies[index].status;
   }
   
   bool online = mBuddies[index].online;

   return online;
}


void 
TuIM::subscribeBuddy( Buddy& buddy )
{
   // subscribe to this budy 
   auto_ptr<SipMessage> msg( buddy.presDialog->makeInitialSubscribe(NameAddr(buddy.uri),NameAddr(mAor)) );

   msg->header(h_Event).value() = Data("presence");;
   msg->header(h_Accepts).push_back( Mime( "application","pidf+xml") );
   msg->header(h_Expires).value() = mSubscriptionTimeSeconds;
   
   buddy.mNextTimeToSubscribe = Timer::getRandomFutureTimeMs( mSubscriptionTimeSeconds*1000 );
   
   setOutbound( *msg );

   mStack->send( *msg );
}


void 
TuIM::addBuddy( const Uri& uri, const Data& group )
{
   Buddy buddy;
   buddy.uri = uri;
   buddy.online = false;
   buddy.status = Data::Empty;
   buddy.group = group;
   buddy.presDialog = new DeprecatedDialog( NameAddr(mContact) );
   resip_assert( buddy.presDialog );

   subscribeBuddy( buddy );

   mBuddies.push_back( buddy );
}


void 
TuIM::removeBuddy( const Uri& name)
{
   TuIM::BuddyIterator i;
	
   i = mBuddies.begin();	
   while ( i != mBuddies.end() )
   {
      Uri u = i->uri;

      if ( u.getAor() == name.getAor() )
      {
         // remove this buddy 
         // !cj! - should unsubscribe 
         i = mBuddies.erase(i);
      }
      else
      {
         i++;
      }
   }
}


void 
TuIM::sendNotify(DeprecatedDialog* dialog)
{ 
   resip_assert( dialog );
   
   auto_ptr<SipMessage> msg( dialog->makeNotify() );

   Pidf* pidf = new Pidf( *mPidf );

   msg->header(h_Event).value() = "presence";

   Token state;
   state.value() = Data("active");
   state.param(p_expires) = dialog->getExpirySeconds(); 
   msg->header(h_SubscriptionState) = state;

   msg->setContents( pidf );
   
   setOutbound( *msg );

   mStack->send( *msg );
}


void 
TuIM::sendPublish(StateAgent& sa)
{ 
   resip_assert( sa.dialog );
   
   auto_ptr<SipMessage> msg( sa.dialog->makeInitialPublish(NameAddr(sa.uri),NameAddr(mAor)) );

   Pidf* pidf = new Pidf( *mPidf );

   msg->header(h_Event).value() = "presence";

   msg->setContents( pidf );
   
   setOutbound( *msg );

   mStack->send( *msg );
}


void 
TuIM::authorizeSubscription( const Data& user )
{
   // TODO implement this 
}


void 
TuIM::setMyPresence( const bool open, const Data& status, const Data& user  )
{
   // TODO implement the pser user status (when user is not empty)
   resip_assert( mPidf );
   mPidf->setSimpleStatus( open, status, mContact.getAor() );
   
   for ( SubscriberIterator i=mSubscribers.begin(); i != mSubscribers.end(); i++)
   {
      DeprecatedDialog* dialog = i->dialog;
      resip_assert( dialog );
      
      sendNotify(dialog);
   } 

   for ( StateAgentIterator i=mStateAgents.begin(); i != mStateAgents.end(); i++)
   {
      sendPublish( *i );
   }
}


void 
TuIM::setOutboundProxy( const Uri& uri )
{
   InfoLog( << "Set outbound proxy to " << uri );
   mOutboundProxy = uri;
}

  
void 
TuIM::setUAName( const Data& name )
{
   DebugLog( << "Set User Agent Name to " << name );
   mUAName = name;
}


void 
TuIM::setOutbound( SipMessage& msg )
{
   if ( msg.isResponse() )
   {
      return;
   }

   if ( !mOutboundProxy.host().empty() )
   {
      NameAddr proxy( mOutboundProxy );
      msg.header(h_Routes).push_front( proxy );
   }
   
   if ( !mUAName.empty() )
   {
      DebugLog( << "UserAgent name=" << mUAName  );
      msg.header(h_UserAgent).value() = mUAName;
   }

   if ( mDefaultProtocol != UNKNOWN_TRANSPORT )
   {
      if ( ! msg.header(h_RequestLine).uri().exists(p_transport) )
      {
         msg.header(h_RequestLine).uri().param(p_transport) = Tuple::toDataLower(mDefaultProtocol);
      }
   }
}


void 
TuIM::setDefaultProtocol( TransportType protocol )
{
   mDefaultProtocol = protocol;
}


void 
TuIM::addStateAgent( const Uri& uri )
{
   StateAgent sa;
   
   sa.dialog = new  DeprecatedDialog( NameAddr(mContact) );
   sa.uri = uri;
   
   mStateAgents.push_back( sa );
   
   sendPublish( sa );
}


bool 
TuIM::Callback::authorizeSubscription( const Uri& user )
{
   return true;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provi
ded that the following conditions
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
