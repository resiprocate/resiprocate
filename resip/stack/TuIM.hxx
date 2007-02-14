#if !defined(RESIP_TUIM_HXX)
#define RESIP_TUIM_HXX

#include <list>

#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Timer.hxx"

namespace resip
{

class Pidf;

class TuIM
{
   private:
      class Buddy;
      class StateAgent;
      class Page;
      
public:
      class Callback
      {
         public:
            virtual void receivedPage(const Data& msg, 
                                      const Uri& from, 
                                      const Data& signedBy,  
                                      SignatureStatus sigStatus,
                                      bool wasEncryped  ) = 0; 
            virtual void sendPageFailed(const Uri& dest, int respNumber ) =0;
            virtual void receivePageFailed(const Uri& sender) =0;
            virtual void registrationFailed(const Uri& dest, int respNumber ) =0;
            virtual void registrationWorked(const Uri& dest ) =0;
            virtual void presenceUpdate(const Uri& user, bool open, const Data& status ) =0;
            virtual bool authorizeSubscription( const Uri& user ); // return
                                                                   // true if
                                                                   // sub is ok
            
            virtual ~Callback();
      };
      
      TuIM(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           Callback* callback,
           const int registrationTimeSeconds = 1*60*60,
           const int subscriptionTimeSeconds =   10*60 );

      ///
      void setOutboundProxy( const Uri& uri );
      void setDefaultProtocol( TransportType protocol );
      void setUAName( const Data& name );
      
      bool haveCerts( bool sign, const Data& encryptFor );
      void sendPage(const Data& text, const Uri& dest, 
                    const bool sign=false, const Data& encryptFor = Data::Empty );

      void process();

      // Registration management 
      void registerAor( const Uri& uri, 
                        const Data& password = Data::Empty );
      
      // Buddy List management
      int getNumBuddies() const;
      const Uri getBuddyUri(const int index);
      const Data getBuddyGroup(const int index);
      bool getBuddyStatus(const int index, Data* status=NULL);
      void addBuddy( const Uri& uri, const Data& group );
      void removeBuddy( const Uri& name);

      // Presence management
      void setMyPresence( const bool open, 
                          const Data& status = Data::Empty,
                          const Data& user = Data::Empty );
      void addStateAgent( const Uri& uri );
      void authorizeSubscription( const Data& user );
      
   private:
      void processSipFrag(SipMessage* msg);
      
      void processRequest(SipMessage* msg);
      void processMessageRequest(SipMessage* msg);
      void processSubscribeRequest(SipMessage* msg);
      void processRegisterRequest(SipMessage* msg);
      void processNotifyRequest(SipMessage* msg);

      void processResponse(SipMessage* msg);
      void processRegisterResponse(SipMessage* msg);
      void processSubscribeResponse(SipMessage* msg, Buddy& buddy );
      void processNotifyResponse(SipMessage* msg, DeprecatedDialog& d );
      void processPublishResponse(SipMessage* msg, StateAgent& sa );
      void processPageResponse(SipMessage* msg, Page& page );

      void sendNotify(DeprecatedDialog* dialog);
      void sendPublish(StateAgent& dialog);
      
      void setOutbound( SipMessage& msg );
      
      void subscribeBuddy( Buddy& buddy );
      
      Callback* mCallback;
      SipStack* mStack;
      Uri mAor;
      Uri mContact;

      class Buddy
      {
         public:
            Uri uri;
            Data group;
            DeprecatedDialog* presDialog; 
            UInt64 mNextTimeToSubscribe;
            bool online;
            Data status;
            
            Buddy() {};
            Buddy(const Buddy& rhs)
            {
                uri = rhs.uri;
                group = rhs.group;
                presDialog = rhs.presDialog;
                mNextTimeToSubscribe = rhs.mNextTimeToSubscribe;
                online = rhs.online;
                status = rhs.status;
            };
      };

      // people I subscribe to
      std::vector<Buddy> mBuddies;
      typedef std::vector<Buddy>::iterator BuddyIterator;

      class StateAgent
      {
         public:
            Uri uri;
            DeprecatedDialog* dialog;
      };
      // people I publish to
      std::list<StateAgent> mStateAgents;
      typedef std::list<StateAgent>::iterator StateAgentIterator;

      // people who subscribe to me 
      class Subscriber
      {
         public:
            Data aor;
            bool authorized;
            DeprecatedDialog* dialog;
      };
      std::list<Subscriber> mSubscribers;
      typedef std::list<Subscriber>::iterator SubscriberIterator;

      class Page
      {
         public:
            Data text;
            Uri uri;
            bool sign;
            Data encryptFor;
            DeprecatedDialog* dialog;
      };
      // outstanding messages
      std::list<Page> mPages;
      typedef std::list<Page>::iterator PageIterator;

      // Current pres info
      Pidf* mPidf;

      // registration information
      DeprecatedDialog mRegistrationDialog;
      UInt64 mNextTimeToRegister;
      Data   mRegistrationPassword;
      unsigned int mLastAuthCSeq; // This is the CSeq of the last registration message
                            // sent that included digest authorization information 

      const int    mRegistrationTimeSeconds; // this is the default time to request in
                                             // a registration
            
      const int    mSubscriptionTimeSeconds; // this is the default time to request in
                                             // a subscription
            
      Uri mOutboundProxy;
      Data mUAName;
      TransportType mDefaultProtocol;
};

}

#endif

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
