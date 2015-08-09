#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


DeprecatedDialog::DeprecatedDialog(const NameAddr& localContact) 
   : mContact(localContact),
     mCreated(false),
     mEarly(false),
     mRouteSet(),
     mRemoteTarget(),
     mRemoteSequence(0),
     mRemoteEmpty(true),
     mLocalSequence(0),
     mLocalEmpty(true),
     mCallId(),
     mLocalTag(),
     mRemoteTag(),
     mRemoteUri(),
     mLocalUri()
{
    // .kw. members "secure" and "expireyTimeAbsoluteMs" are not initialized!
}

SipMessage*
DeprecatedDialog::makeResponse(const SipMessage& request, int code)
{
   resip_assert( code >= 100 );
   
   if ( (!mCreated) && (code < 300) && (code > 100) )
   {
      resip_assert (code > 100);
      resip_assert (code < 300);      
      resip_assert(request.isRequest());
      resip_assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE ||
             request.header(h_RequestLine).getMethod() == PUBLISH);
      
      resip_assert (request.header(h_Contacts).size() == 1);

      SipMessage* response = Helper::makeResponse(request, code, mContact);
      if (request.exists(h_RecordRoutes))
      {
         mRouteSet = request.header(h_RecordRoutes);
      }

      if (!request.exists(h_Contacts) && request.header(h_Contacts).size() != 1)
      {
         InfoLog (<< "Request doesn't have a contact header or more than one contact, so can't create dialog");
         DebugLog (<< request);
         throw Exception("Invalid or missing contact header in request", __FILE__,__LINE__);
      }

      mRemoteTarget = request.header(h_Contacts).front();
      mRemoteSequence = request.header(h_CSeq).sequence();
      mRemoteEmpty = false;
      mLocalSequence = 0;
      mLocalEmpty = true;
      mCallId = request.header(h_CallId);
      response->header(h_To).param(p_tag) = Helper::computeTag(Helper::tagSize);
      resip_assert (response->header(h_To).exists(p_tag));
      mLocalTag = response->header(h_To).param(p_tag); // from response 
      if (request.header(h_From).exists(p_tag))  // 2543 compat
      {
         mRemoteTag = request.header(h_From).param(p_tag); 
      }
      
      mRemoteUri = request.header(h_From);
      mLocalUri = request.header(h_To);

      mDialogId = mCallId;
      mDialogId.param(p_toTag) = mLocalTag;
      mDialogId.param(p_fromTag) = mRemoteTag;

      mEarly = (code < 200);
      mCreated = true;

      return response;
   }
   else
   {
      SipMessage* response = Helper::makeResponse(request, code, mContact);
      if (mCreated)
      {
         response->header(h_To).param(p_tag) = mLocalTag;
      }
      //DebugLog(<< "Created response within dialog: " << *response);
      return response;
   }
}


void 
DeprecatedDialog::createDialogAsUAC(const SipMessage& msg)
{
   if (!mCreated)
   {
      if(msg.isResponse())
      {
         const SipMessage& response = msg;

         int code = response.header(h_StatusLine).statusCode();
         mEarly = (code > 100 && code < 200);
         
         if (code >= 200 && code < 300)
         {
            if (!response.exists(h_Contacts) || response.header(h_Contacts).size() != 1)
            {
               InfoLog (<< "Response doesn't have a contact header or more than one contact, so can't create dialog");
               DebugLog (<< response);
               throw Exception("Invalid or missing contact header in message", __FILE__,__LINE__);
            }
         }
         
         // reverse order from response
         if (response.exists(h_RecordRoutes))
         {
            mRouteSet = response.header(h_RecordRoutes).reverse();
         }
         
         if (response.exists(h_Contacts) && !response.header(h_Contacts).empty())
         {
            mRemoteTarget = response.header(h_Contacts).front();
         }
         
         mRemoteSequence = 0;
         mRemoteEmpty = true;
         mLocalSequence = response.header(h_CSeq).sequence();
         mLocalEmpty = false;
         mCallId = response.header(h_CallId);
         if ( response.header(h_From).exists(p_tag) ) // 2543 compat
         {
            mLocalTag = response.header(h_From).param(p_tag);  
         }
         if ( response.header(h_To).exists(p_tag) )  // 2543 compat
         {
            mRemoteTag = response.header(h_To).param(p_tag); 
         }
         mRemoteUri = response.header(h_To);
         mLocalUri = response.header(h_From);

         mDialogId = mCallId;
         mDialogId.param(p_toTag) = mLocalTag;
         mDialogId.param(p_fromTag) = mRemoteTag;

         mCreated = true;
      }
      else if (msg.isRequest() && msg.header(h_CSeq).method() == NOTIFY)
      {
         const SipMessage& notify = msg;
         if (notify.exists(h_RecordRoutes))
         {
            mRouteSet = notify.header(h_RecordRoutes);
         }
      
         if (!notify.exists(h_Contacts) && notify.header(h_Contacts).size() != 1)
         {
            InfoLog (<< "Notify doesn't have a contact header or more than one contact, so can't create dialog");
            DebugLog (<< notify);
            throw Exception("Invalid or missing contact header in notify", __FILE__,__LINE__);
         }

         mRemoteTarget = notify.header(h_Contacts).front();
         mRemoteSequence = notify.header(h_CSeq).sequence();
         mRemoteEmpty = false;
         mLocalSequence = 0;
         mLocalEmpty = true;
         mCallId = notify.header(h_CallId);
         if (notify.header(h_To).exists(p_tag))
         {
            mLocalTag = notify.header(h_To).param(p_tag); 
         }
         if (notify.header(h_From).exists(p_tag))  // 2543 compat
         {
            mRemoteTag = notify.header(h_From).param(p_tag); 
         }
      
         mRemoteUri = notify.header(h_From);
         mLocalUri = notify.header(h_To);

         mDialogId = mCallId;
         mDialogId.param(p_toTag) = mLocalTag;
         mDialogId.param(p_fromTag) = mRemoteTag;

         mCreated = true;
         mEarly = false;
      }
   }
   else if (msg.isResponse())
   {
      mEarly = (msg.header(h_StatusLine).statusCode() < 200 && 
                msg.header(h_StatusLine).statusCode() > 100);

      // don't update target for register since contact is not a target
      if ( msg.header(h_CSeq).method() != REGISTER )
      {
         targetRefreshResponse(msg);
      }
   }
}

void 
DeprecatedDialog::targetRefreshResponse(const SipMessage& response)
{
   if (response.exists(h_Contacts) && response.header(h_Contacts).size() == 1)
   {
      mRemoteTarget = response.header(h_Contacts).front();
   }
}

int 
DeprecatedDialog::targetRefreshRequest(const SipMessage& request)
{
   resip_assert (request.header(h_RequestLine).getMethod() != CANCEL);
   if (request.header(h_RequestLine).getMethod() != ACK)
   {
      unsigned long cseq = request.header(h_CSeq).sequence();
   
      if (mRemoteEmpty)
      {
         mRemoteSequence = cseq;
         mRemoteEmpty = false;
      }
      else if (cseq < mRemoteSequence)
      {
         InfoLog (<< "Got a cseq out of sequence: " << cseq << " < " << mRemoteSequence);
         throw Exception("out of order", __FILE__,__LINE__);
      }
      else
      {
         mRemoteSequence = cseq;
      }

      if (request.exists(h_Contacts) && request.header(h_Contacts).size() == 1)
      {
         mRemoteTarget = request.header(h_Contacts).front();
      }
      else
      {
         InfoLog (<< "Request doesn't have a contact header or more than one contact, so can't create dialog");
         DebugLog (<< request);
         throw Exception("Invalid or missing contact header in message", __FILE__,__LINE__);
      }
   }
   
   return 0;
}

void
DeprecatedDialog::updateRequest(SipMessage& request)
{
   resip_assert (request.isRequest());
   if (mCreated)
   {
      request.header(h_RequestLine).uri() = mRemoteTarget.uri();
      request.header(h_To) = mRemoteUri;
      if ( !mRemoteTag.empty() )
      {
         request.header(h_To).param(p_tag) = mRemoteTag;
      }
      request.header(h_From) = mLocalUri;
      if ( !mLocalTag.empty() )
      {
         request.header(h_From).param(p_tag) = mLocalTag; 
      }
      request.header(h_CallId) = mCallId;
      request.header(h_Routes) = mRouteSet;
      request.header(h_Contacts).clear();
      request.header(h_Contacts).push_back(mContact);
      copyCSeq(request);
      incrementCSeq(request);

      request.header(h_MaxForwards).value() = 70;

      Via via;
      via.param(p_branch); // will create the branch
      request.header(h_Vias).clear();
      request.header(h_Vias).push_back(via);

      request.clearForceTarget();
      Helper::processStrictRoute(request);
   }
   else
   {
      DebugLog (<< "Updating a request when not in a dialog yet");
   }
}

void
DeprecatedDialog::makeResponse(const SipMessage& request, SipMessage& response, int code)
{
   resip_assert(request.isRequest());
   if ( (!mCreated) && (code < 300) && (code > 100) )
   {
      resip_assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE);
      resip_assert (request.header(h_Contacts).size() == 1);

      Helper::makeResponse(response, request, code, mContact);
      response.header(h_To).param(p_tag) = Helper::computeTag(Helper::tagSize);

      if (request.exists(h_RecordRoutes))
      {
         mRouteSet = request.header(h_RecordRoutes);
      }

      if (!request.exists(h_Contacts) && request.header(h_Contacts).size() != 1)
      {
         InfoLog (<< "Request doesn't have a contact header or more than one contact, so can't create dialog");
         DebugLog (<< request);
         throw Exception("Invalid or missing contact header in request", __FILE__,__LINE__);
      }

      mRemoteTarget = request.header(h_Contacts).front();
      mRemoteSequence = request.header(h_CSeq).sequence();
      mRemoteEmpty = false;
      mLocalSequence = 0;
      mLocalEmpty = true;
      mCallId = request.header(h_CallId);
      resip_assert (response.const_header(h_To).exists(p_tag));
      mLocalTag = response.header(h_To).param(p_tag); // from response 
      if (request.header(h_From).exists(p_tag))  // 2543 compat
      {
         mRemoteTag = request.header(h_From).param(p_tag); 
      }
      
      mRemoteUri = request.header(h_From);
      mLocalUri = request.header(h_To);

      mDialogId = mCallId;
      mDialogId.param(p_toTag) = mLocalTag;
      mDialogId.param(p_fromTag) = mRemoteTag;

      mEarly = (code > 100 && code < 200);

      mCreated = true;
   }
   else
   {
      Helper::makeResponse(response, request, code, mContact);
      if (mCreated)
      {
         response.header(h_To).param(p_tag) = mLocalTag;
         mEarly = false;
      }
   }
}


SipMessage* 
DeprecatedDialog::makeInitialRegister(const NameAddr& registrar, const NameAddr& aor)
{
   SipMessage* msg = Helper::makeRegister( registrar, aor, mContact );
   resip_assert( msg );

   mRequestUri = msg->const_header(h_RequestLine).uri();
   mLocalEmpty = false;
   mLocalSequence = msg->const_header(h_CSeq).sequence();
   mCallId = msg->const_header(h_CallId);
   resip_assert(msg->const_header(h_From).exists(p_tag));
   mLocalTag = msg->const_header(h_From).param(p_tag);  
   mRemoteUri = msg->const_header(h_To);
   mLocalUri = msg->const_header(h_From);
   mCreated = true;
   
   mRemoteTarget = mRemoteUri;
   
   return msg;
}


SipMessage* 
DeprecatedDialog::makeInitialSubscribe(const NameAddr& target, const NameAddr& from)
{
   SipMessage* msg = Helper::makeSubscribe( target, from, mContact );
   resip_assert( msg );

   mRequestUri = msg->const_header(h_RequestLine).uri();
   mLocalEmpty = false;
   mLocalSequence = msg->const_header(h_CSeq).sequence();
   mCallId = msg->const_header(h_CallId);
   resip_assert(msg->const_header(h_From).exists(p_tag));
   mLocalTag = msg->const_header(h_From).param(p_tag);  
   mRemoteUri = msg->const_header(h_To);
   mLocalUri = msg->const_header(h_From);
   
   return msg;
}


SipMessage* 
DeprecatedDialog::makeInitialPublish(const NameAddr& target, const NameAddr& from)
{
   SipMessage* msg = Helper::makePublish( target, from, mContact );
   resip_assert( msg );

   mRequestUri = msg->const_header(h_RequestLine).uri();
   mLocalEmpty = false;
   mLocalSequence = msg->const_header(h_CSeq).sequence();
   mCallId = msg->const_header(h_CallId);
   resip_assert(msg->const_header(h_From).exists(p_tag));
   mLocalTag = msg->const_header(h_From).param(p_tag);  
   mRemoteUri = msg->const_header(h_To);
   mLocalUri = msg->const_header(h_From);
   
   return msg;
}


SipMessage* 
DeprecatedDialog::makeInitialMessage(const NameAddr& target, const NameAddr& from)
{
   SipMessage* msg = Helper::makeMessage( target, from, mContact );
   resip_assert( msg );

   mRequestUri = msg->const_header(h_RequestLine).uri();
   mLocalEmpty = false;
   mLocalSequence = msg->const_header(h_CSeq).sequence();
   mCallId = msg->const_header(h_CallId);
   resip_assert(msg->const_header(h_From).exists(p_tag));
   mLocalTag = msg->const_header(h_From).param(p_tag);  
   mRemoteUri = msg->const_header(h_To);
   mLocalUri = msg->const_header(h_From);
   
   return msg;
}


SipMessage* 
DeprecatedDialog::makeInitialInvite(const NameAddr& target, const NameAddr& from)
{
   SipMessage* msg = Helper::makeInvite( target, from, mContact );
   resip_assert( msg );

   mRequestUri = msg->const_header(h_RequestLine).uri();
   mLocalEmpty = false;
   mLocalSequence = msg->const_header(h_CSeq).sequence();
   mCallId = msg->const_header(h_CallId);
   resip_assert(msg->const_header(h_From).exists(p_tag));
   mLocalTag = msg->const_header(h_From).param(p_tag);  
   mRemoteUri = msg->const_header(h_To);
   mLocalUri = msg->const_header(h_From);
   
   return msg;
}


SipMessage*
DeprecatedDialog::makeInvite()
{
   SipMessage* request = makeRequestInternal(INVITE);
   incrementCSeq(*request);
   DebugLog(<< "DeprecatedDialog::makeInvite: " << *request);
   return request;
}

SipMessage*
DeprecatedDialog::makeUpdate()
{
   SipMessage* request = makeRequestInternal(UPDATE);
   incrementCSeq(*request);
   DebugLog(<< "DeprecatedDialog::makeUpdate: " << *request);
   return request;
}

SipMessage*
DeprecatedDialog::makeRegister()
{
   SipMessage* request = makeRequestInternal(REGISTER);
   incrementCSeq(*request);
   DebugLog(<< "DeprecatedDialog::makeRegister: " << *request);
   return request;
}

SipMessage*
DeprecatedDialog::makeSubscribe()
{
   SipMessage* request = makeRequestInternal(SUBSCRIBE);
   incrementCSeq(*request);
   DebugLog(<< "DeprecatedDialog::makeSubscribe: " << *request);
   return request;
}

SipMessage*
DeprecatedDialog::makeBye()
{
   SipMessage* request = makeRequestInternal(BYE);
   incrementCSeq(*request);

   return request;
}


SipMessage*
DeprecatedDialog::makeRefer(const NameAddr& referTo)
{
   SipMessage* request = makeRequestInternal(REFER);
   request->header(h_ReferTo) = referTo;
   request->header(h_ReferredBy) = mLocalUri;
   incrementCSeq(*request);
   return request;
}

SipMessage*
DeprecatedDialog::makeNotify()
{
   SipMessage* request = makeRequestInternal(NOTIFY);
   incrementCSeq(*request);
   return request;
}


SipMessage*
DeprecatedDialog::makeOptions()
{
   SipMessage* request = makeRequestInternal(OPTIONS);
   incrementCSeq(*request);
   return request;
}

SipMessage*
DeprecatedDialog::makePublish()
{
   SipMessage* request = makeRequestInternal(PUBLISH);
   incrementCSeq(*request);
   return request;
}

SipMessage*
DeprecatedDialog::makeRequest(resip::MethodTypes method)
{
   resip_assert(method != ACK);
   resip_assert(method != CANCEL);
   
   SipMessage* request = makeRequestInternal(method);
   incrementCSeq(*request);
   return request;
}

SipMessage*
DeprecatedDialog::makeAck(const SipMessage& original)
{
   SipMessage* request = makeRequestInternal(ACK);
   copyCSeq(*request);

   // !dcm! should we copy the authorizations? 
   // !jf! will this do the right thing if these headers weren't in original 
   // we should be able to store this stuff in the DeprecatedDialog and not need to pass
   // in the original
   if (original.exists(h_ProxyAuthorizations))
   {
      request->header(h_ProxyAuthorizations) = original.header(h_ProxyAuthorizations);
   }
   if (original.exists(h_Authorizations))
   {    
      request->header(h_Authorizations) = original.header(h_Authorizations);
   }
   request->header(h_CSeq).sequence() = original.header(h_CSeq).sequence();
   return request;
}

SipMessage*
DeprecatedDialog::makeAck()
{
   SipMessage* request = makeRequestInternal(ACK);
   copyCSeq(*request);
   return request;
}

SipMessage*
DeprecatedDialog::makeCancel(const SipMessage& request)
{
   resip_assert (request.header(h_Vias).size() >= 1);
   resip_assert (request.header(h_RequestLine).getMethod() == INVITE);
   
   SipMessage* cancel = new SipMessage;
   
   cancel->header(h_RequestLine) = request.header(h_RequestLine);
   cancel->header(h_RequestLine).method() = CANCEL;
   
   cancel->header(h_CallId) = request.header(h_CallId);
   cancel->header(h_To) = request.header(h_To); 
   cancel->header(h_From) = request.header(h_From);
   cancel->header(h_CSeq) = request.header(h_CSeq);
   cancel->header(h_CSeq).method() = CANCEL;
   cancel->header(h_Vias).push_back(request.header(h_Vias).front());
   
   return cancel;
}


CallId 
DeprecatedDialog::makeReplaces()
{
   return mDialogId;
}

void
DeprecatedDialog::clear()
{
   mCreated = false;
   mEarly = false;
   
   mRouteSet.clear();
   mRemoteTarget = NameAddr();
   mRemoteSequence = 0;
   mRemoteEmpty = true;
   mLocalSequence = 0;
   mLocalEmpty = true;
   mCallId.value() = Data::Empty;
   mLocalTag = Data::Empty;
   mRemoteTag = Data::Empty;
   mRemoteUri = NameAddr();
   mLocalUri = NameAddr();
}

SipMessage*
DeprecatedDialog::makeRequestInternal(MethodTypes method)
{
   SipMessage* request = new SipMessage;
   RequestLine rLine(method);

   if (!mCreated)
   {
      rLine.uri() = mRequestUri;
   }
   else
   {
      rLine.uri() = mRemoteTarget.uri();
   }
   
   request->header(h_RequestLine) = rLine;
   request->header(h_To) = mRemoteUri;
   if ( !mRemoteTag.empty() )
   {
       request->header(h_To).param(p_tag) = mRemoteTag;
   }
   request->header(h_From) = mLocalUri;
   if ( !mLocalTag.empty() )
   {
      request->header(h_From).param(p_tag) = mLocalTag; 
   }
   request->header(h_CallId) = mCallId;
   request->header(h_Routes) = mRouteSet;
   request->header(h_Contacts).push_back(mContact);
   request->header(h_CSeq).method() = method;
   copyCSeq(*request);
   request->header(h_MaxForwards).value() = 70;

   Via via;
   via.param(p_branch); // will create the branch
   request->header(h_Vias).push_front(via);

   Helper::processStrictRoute(*request);
   return request;
}

void
DeprecatedDialog::copyCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.header(h_CSeq).sequence() = mLocalSequence;
}

void
DeprecatedDialog::incrementCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   //DebugLog ( << "mLocalSequence: " << mLocalSequence);
   request.header(h_CSeq).sequence() = ++mLocalSequence;
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const DeprecatedDialog& d)
{
   strm << "DeprecatedDialog: [" << d.dialogId() 
        << " created=" << d.mCreated 
        << ",remoteTarget=" << d.mRemoteTarget 
        << ", routeset=" << Inserter(d.mRouteSet) 
        << ",remoteSeq=" << d.mRemoteSequence 
        << ",remote=" << d.mRemoteUri 
        << ",remoteTag=" << d.mRemoteTag 
        << ",localSeq=" << d.mLocalSequence
        << ",local=" << d.mLocalUri 
        << ",localTag=" << d.mLocalTag 
        << "]";
   return strm;
}
  
Data 
DeprecatedDialog::dialogId(const SipMessage& msg)
{
   CallID id(msg.header(h_CallId));
   if ((msg.isRequest() && msg.isExternal()) ||
       (msg.isResponse() && !msg.isExternal()))
   {
      if (msg.header(h_To).exists(p_tag))
      {
         id.param(p_toTag) = msg.header(h_To).param(p_tag);
      }
      if (msg.header(h_From).exists(p_tag))
      {
         id.param(p_fromTag) = msg.header(h_From).param(p_tag);
      }
   }
   else
   {
      if (msg.header(h_From).exists(p_tag))
      {
         id.param(p_toTag) = msg.header(h_From).param(p_tag);
      }
      if (msg.header(h_To).exists(p_tag))
      {
         id.param(p_fromTag) = msg.header(h_To).param(p_tag);
      }
   }
   return Data::from(id);
}

 
const Data 
DeprecatedDialog::dialogId() const
{
   return Data::from(mDialogId);
}


void 
DeprecatedDialog::setExpirySeconds( int secondsInFuture )
{ 
   expireyTimeAbsoluteMs = Timer::getTimeMs() + 1000*secondsInFuture;
}


int  
DeprecatedDialog::getExpirySeconds()
{
	// !cj! TODO - may be bugs here when result is negative 
   UInt64 delta = ( expireyTimeAbsoluteMs - Timer::getTimeMs() )/1000;

   int ret = (int)delta;
   return ret;
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
