#if !defined(RESIP_TUIM_HXX)
#define RESIP_TUIM_HXX

#include <vector>

#include "resiprocate/Dialog.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/os/Timer.hxx"

namespace resip
{

class Pidf;
class Dialog;

class TuIM
{
   private:
      class Buddy;
      
public:
      class Callback
      {
         public:
            virtual void receivedPage(const Data& msg, 
                                      const Uri& from, 
                                      const Data& signedBy,  
                                      Security::SignatureStatus sigStatus,
                                      bool wasEncryped  ) = 0; 
            virtual void sendPageFailed(const Uri& dest, int respNumber ) =0;
            virtual void receivePageFailed(const Uri& sender) =0;
            virtual void registrationFailed(const Uri& dest, int respNumber ) =0;
            virtual void registrationWorked(const Uri& dest ) =0;
            virtual void presenseUpdate(const Uri& dest, bool open, const Data& status ) =0;
            virtual ~Callback();
      };
      
      TuIM(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           Callback* callback);

      ///
      void setOutboundProxy( const Uri& uri );
      void setDefaultProtocol( TransportType protocol );
      void setUAName( const Data& name );
      
      bool haveCerts( bool sign, const Data& encryptFor );
      void sendPage(const Data& text, const Uri& dest, bool sign, const Data& encryptFor );

      void process();

      // Registration management 
      void registerAor( const Uri& uri, 
                        const Data& password = Data::Empty, 
                        int registrationTimeSeconds = 1800 );
      
      // Buddy List management
      int getNumBuddies() const;
      const Uri getBuddyUri(const int index);
      const Data getBuddyGroup(const int index);
      bool getBuddyStatus(const int index, Data* status=NULL);
      void addBuddy( const Uri& uri, const Data& group );
      void removeBuddy( const Uri& name);

      // Presense management
      void setMyPresense( const bool open, const Data& status = Data::Empty );

   private:
      void processRequest(SipMessage* msg);
      void processMessageRequest(SipMessage* msg);
      void processSubscribeRequest(SipMessage* msg);
      void processRegisterRequest(SipMessage* msg);
      void processNotifyRequest(SipMessage* msg);

      void processResponse(SipMessage* msg);
      void processRegisterResponse(SipMessage* msg);
      void processSubscribeResponse(SipMessage* msg, Buddy& buddy );

      void sendNotify(Dialog* dialog);
      
      void setOutbound( SipMessage& msg );
      
      Callback* mCallback;
      SipStack* mStack;
      Uri mAor;
      Uri mContact;
      //Data mPassword;

      // people I subscibe too 
      class Buddy
      {
         public:
            Uri uri;
            Data group;
            Dialog* presDialog; 
            UInt64 mNextTimeToSubscribe;
            bool online;
            Data status;
      };
      std::vector<Buddy> mBuddy;
	  typedef std::vector<Buddy>::iterator BuddyIterator;

      // people who subscribe to me 
      std::vector<Dialog*> mSubscribers;
      Pidf* mPidf;
      
      // registration information
      Dialog mRegistrationDialog;
      UInt64 mNextTimeToRegister;
      Data   mRegistrationPassword;
      int    mLastAuthCSeq; // This is the CSeq of the last registration message
                            // sent that included digest authorization information 
      int    mRegistrationTimeSeconds; // this is the default time to request in
                                       // a registration
            
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
