#if !defined(RESIP_SUBSCRIPTIONHANDLER_HXX)
#define RESIP_SUBSCRIPTIONHANDLER_HXX

#include "resip/dum/Handles.hxx"
#include "resip/stack/Mime.hxx"
#include "resip/stack/Contents.hxx"

namespace resip
{
class SipMessage;
class SecurityAttributes;

class ClientSubscriptionHandler
{
  public:
      virtual ~ClientSubscriptionHandler() { }

      //Client must call acceptUpdate or rejectUpdate for any onUpdateFoo
      virtual void onUpdatePending(ClientSubscriptionHandle, const SipMessage& notify, bool outOfOrder)=0;
      virtual void onUpdateActive(ClientSubscriptionHandle, const SipMessage& notify, bool outOfOrder)=0;
      //unknown Subscription-State value
      virtual void onUpdateExtension(ClientSubscriptionHandle, const SipMessage& notify, bool outOfOrder)=0;

      virtual int onRequestRetry(ClientSubscriptionHandle, int retrySeconds, const SipMessage& notify)=0;
      
      //subscription can be ended through a notify or a failure response.
      virtual void onTerminated(ClientSubscriptionHandle, const SipMessage* msg)=0;   
      //not sure if this has any value - can be called for either a 200/SUBSCRIBE or a NOTIFY - whichever arrives first
      virtual void onNewSubscription(ClientSubscriptionHandle, const SipMessage& notify)=0;

      /// called to allow app to adorn a message.
      virtual void onReadyToSend(ClientSubscriptionHandle, SipMessage& msg);
      virtual void onNotifyNotReceived(ClientSubscriptionHandle);

      /// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
      /// errors, or missing CRLF keep alives pong responses from the server.
      //  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
      /// supports RFC5626 (outbound).
      /// Default implementation is to re-form the subscription using a new flow
      virtual void onFlowTerminated(ClientSubscriptionHandle);
};

class ServerSubscriptionHandler
{
  public:   
      virtual ~ServerSubscriptionHandler() {}

      virtual void onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub)=0;
      virtual void onNewSubscriptionFromRefer(ServerSubscriptionHandle, const SipMessage& sub);
      virtual void onRefresh(ServerSubscriptionHandle, const SipMessage& sub);
      //called when a new document is Published that matches this subscription.  Also
      //called when the publication is removed or expires, in which case contents 
      //and attrs are passed as null pointers.
      virtual void onPublished(ServerSubscriptionHandle associated, 
                               ServerPublicationHandle publication, 
                               const Contents* contents,
                               const SecurityAttributes* attrs);

      virtual void onNotifyAccepted(ServerSubscriptionHandle, const SipMessage& msg);      
      virtual void onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg);      

      //called when this usage is destroyed for any reason. One of the following
      //three methods will always be called before this, but this is the only
      //method that MUST be implemented by a handler
      virtual void onTerminated(ServerSubscriptionHandle)=0;

      virtual void onReadyToSend(ServerSubscriptionHandle, SipMessage& msg);

      //will be called when a NOTIFY is not delivered(with a usage terminating
      //statusCode), or the Dialog is destroyed
      virtual void onError(ServerSubscriptionHandle, const SipMessage& msg);      

      //app can synchronously decorate terminating NOTIFY messages. The only
      //graceful termination mechanism is expiration, but the client can
      //explicity end a subscription with an Expires header of 0.
      virtual void onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify);
      virtual void onExpired(ServerSubscriptionHandle, SipMessage& notify);

      /// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
      /// errors, or missing CRLF keep alives pong responses from the server.
      //  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
      /// supports RFC5626 (outbound).
      /// Default implementation is to tear down the subscription
      virtual void onFlowTerminated(ServerSubscriptionHandle);

      /** Default behavior is to use the expires value in the SipMessage, if it exists. Then verify the expires >= Min and <= Max (if set).  If an expires value does
       * not exists, use getDefaultExpires().  If hasDefaultExpires() is false, then reject the message with a 400.
       * Set errorReturnCode to an error code >= 400 to reject this subscription. 
       */  
      virtual void getExpires(const SipMessage &msg, UInt32 &expires, int &errorResponseCode);//ivr mod

      virtual bool hasDefaultExpires() const;
      virtual UInt32 getDefaultExpires() const;

      virtual bool hasMinExpires() const;
      virtual UInt32 getMinExpires() const;

      virtual bool hasMaxExpires() const;
      virtual UInt32 getMaxExpires() const;

      const Mimes& getSupportedMimeTypes() const;
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
