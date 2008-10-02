#if !defined(RESIP_DIALOG_HXX)
#define RESIP_DIALOG_HXX

#include <iosfwd>
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/CallId.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Timer.hxx"

namespace resip
{

class SipMessage;
class NameAddr;
class CallID;

class DeprecatedDialog
{
   public:
      class Exception : public BaseException 
      {
         public:
            Exception( const resip::Data& msg,
                       const resip::Data& file,
                       const int line): BaseException(msg,file,line){}
            const char* name() const { return "Dialog::Exception"; }
      };
      
      // pass in a contact for this location e.g. "sip:local@domain:5060"
      DeprecatedDialog(const NameAddr& localContact);

      // If a dialog is not yet created, make a dialog or early dialog as
      // follows. Otherwise, just make response from the dialog state
      // Used by the UAS to create a dialog or early dialog, will return a 1xx(code)
      // UAS should call this upon receiving the invite/subscribe. 
      // This should not be called twice if the UAS sends back a 180 and a 200,
      // it should call it for the 180 and then use Dialog::makeResponse to make
      // the 200
      SipMessage* makeResponse(const SipMessage& request, int code=200);
      
      // This happens when a dialog gets created on a UAC when 
      // a UAC receives a response that creates a dialog. Also works for NOTIFY
      // requests which create a dialog
      void createDialogAsUAC(const SipMessage& response);

      // Called when a 2xx response is received in an existing dialog
      // Replace the _remoteTarget with uri from Contact header in response
      void targetRefreshResponse(const SipMessage& response);

      // Called when a request is received in an existing dialog
      // return status code of response to generate - 0 if ok
      int targetRefreshRequest(const SipMessage& request);

      // given a template of a request, update the relevant fields based on this
      void updateRequest(SipMessage& msg);

      // For UAS, make a response and create the dialog if necessary
      void makeResponse(const SipMessage& request, SipMessage& response, int code=200);

      bool isCreated() const { return mCreated; }
      bool isEarly() const { return mEarly; }

      static Data dialogId(const SipMessage& msg);
      const Data dialogId() const;
      const CallID& getCallId() const { return mCallId; }
      const NameAddr& getRemoteTarget() const { return mRemoteTarget; }
      //const Data& getLocalTag() const { return mLocalTag; }
      const Data& getRemoteTag() const { return mRemoteTag; }
      
      // For creating request which do not form a dialog but whose response
      // might create a dialog 
      SipMessage* makeInitialRegister(const NameAddr& registrar, const NameAddr& from);
      SipMessage* makeInitialSubscribe(const NameAddr& target, const NameAddr& from);
      SipMessage* makeInitialPublish(const NameAddr& target, const NameAddr& from);
      SipMessage* makeInitialInvite(const NameAddr& target, const NameAddr& from);
      SipMessage* makeInitialMessage(const NameAddr& target, const NameAddr& from);
      
      // For creating requests within a dialog
      SipMessage* makeInvite();
      SipMessage* makeUpdate();
      SipMessage* makeRegister();
      SipMessage* makeSubscribe();
      SipMessage* makeBye();
      SipMessage* makeRefer(const NameAddr& referTo);
      SipMessage* makeNotify();
      SipMessage* makeOptions();
      SipMessage* makePublish();
      SipMessage* makeAck();
      SipMessage* makeAck(const SipMessage& request);
      SipMessage* makeCancel(const SipMessage& request);
      SipMessage* makeRequest(MethodTypes method);
      CallID makeReplaces();
      
      // resets to an empty dialog with no state
      void clear();
      
      // set how many seconds in the futre this dialog will expire
      void setExpirySeconds( int secondsInFuture );
      int  getExpirySeconds(); // get number Seconds till this expires, it can
                               // be negative 
      
      
   private:
      SipMessage* makeRequestInternal(MethodTypes method);
      void incrementCSeq(SipMessage& request);
      void copyCSeq(SipMessage& request);

      NameAddr mContact;  // for this UA

      // Dialog State
      bool mCreated;
      bool mEarly;
      
      Uri mRequestUri;
      
      NameAddrs mRouteSet;
      NameAddr mRemoteTarget;

      unsigned long mRemoteSequence;
      bool mRemoteEmpty;
      unsigned long mLocalSequence;
      bool mLocalEmpty;

      CallID mCallId;
      Data mLocalTag;
      Data mRemoteTag;
      CallID mDialogId;
      
      NameAddr mRemoteUri;
      NameAddr mLocalUri;
      
      bool secure; // indicates the messages in this Dialog must use TLS

      UInt64 expireyTimeAbsoluteMs;
      
      friend EncodeStream& operator<<(EncodeStream&, const DeprecatedDialog&);
};

EncodeStream&
operator<<(EncodeStream& strm, const DeprecatedDialog& d);
 
} // namespace Cathay

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
