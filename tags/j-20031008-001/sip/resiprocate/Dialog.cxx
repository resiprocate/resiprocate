#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <iostream>
#include "resiprocate/Dialog.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


Dialog::Dialog(const NameAddr& localContact) 
   : mContact(localContact),
     mCreated(false),
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
}

SipMessage*
Dialog::makeResponse(const SipMessage& request, int code)
{
   assert( code >= 100 );
   
   if ( (!mCreated) && (code < 300) && (code > 100) )
   {
      assert (code > 100);
      assert (code < 300);      
      assert(request.isRequest());
      assert(request.header(h_RequestLine).getMethod() == INVITE ||
             request.header(h_RequestLine).getMethod() == SUBSCRIBE);
      
      assert (request.header(h_Contacts).size() == 1);

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
      assert (response->header(h_To).exists(p_tag));
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
Dialog::createDialogAsUAC(const SipMessage& response)
{
   if (!mCreated)
   {
      assert(response.isResponse());

      if ( !response.exists(h_Contacts) && response.header(h_Contacts).size() != 1)
      {
         InfoLog (<< "Response doesn't have a contact header or more than one contact, so can't create dialog");
         DebugLog (<< response);
         throw Exception("Invalid or missing contact header in message", __FILE__,__LINE__);
      }

      // reverse order from response
      if (response.exists(h_RecordRoutes))
      {
         mRouteSet = response.header(h_RecordRoutes).reverse();
      }
      mRemoteTarget = response.header(h_Contacts).front();
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
}



void 
Dialog::createRegistrationAsUAC(const SipMessage& response)
{
   if (!mCreated)
   {
      assert(response.isResponse());

      // reverse order from response
      if (response.exists(h_RecordRoutes))
      {
         mRouteSet = response.header(h_RecordRoutes).reverse();
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
}



void 
Dialog::targetRefreshResponse(const SipMessage& response)
{
   if (response.exists(h_Contacts) && response.header(h_Contacts).size() == 1)
   {
      mRemoteTarget = response.header(h_Contacts).front();
   }
   else
   {
      InfoLog (<< "Response doesn't have a contact header or more than one contact, so can't create dialog");
      DebugLog (<< response);
      throw Exception("Invalid or missing contact header in message", __FILE__,__LINE__);
   }
}

int 
Dialog::targetRefreshRequest(const SipMessage& request)
{
   assert (request.header(h_RequestLine).getMethod() != CANCEL);
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


SipMessage* 
Dialog::makeInitialRegister(const NameAddr& registrar,
                            const NameAddr& aor)
{
   SipMessage* msg = Helper::makeRegister( registrar, aor, mContact );
   assert( msg );

   mLocalEmpty = false;
   mLocalSequence = msg->header(h_CSeq).sequence();
   mCallId = msg->header(h_CallId);
   assert(msg->header(h_From).exists(p_tag));
   mLocalTag = msg->header(h_From).param(p_tag);  
   mRemoteUri = msg->header(h_To);
   mLocalUri = msg->header(h_From);
   
   mRemoteTarget = mRemoteUri;
   
   return msg;
}


SipMessage* 
Dialog::makeInitialSubscribe(const NameAddr& target, 
                             const NameAddr& from)
{
   SipMessage* msg = Helper::makeSubscribe( target, from, mContact );
   assert( msg );

   mLocalEmpty = false;
   mLocalSequence = msg->header(h_CSeq).sequence();
   mCallId = msg->header(h_CallId);
   assert(msg->header(h_From).exists(p_tag));
   mLocalTag = msg->header(h_From).param(p_tag);  
   mRemoteUri = msg->header(h_To);
   mLocalUri = msg->header(h_From);
   
   return msg;
}


SipMessage* 
Dialog::makeInitialInvite(const NameAddr& target,
                          const NameAddr& from)
{
   SipMessage* msg = Helper::makeInvite( target, from, mContact );
   assert( msg );

   mLocalEmpty = false;
   mLocalSequence = msg->header(h_CSeq).sequence();
   mCallId = msg->header(h_CallId);
   assert(msg->header(h_From).exists(p_tag));
   mLocalTag = msg->header(h_From).param(p_tag);  
   mRemoteUri = msg->header(h_To);
   mLocalUri = msg->header(h_From);
   
   return msg;
}


SipMessage*
Dialog::makeInvite()
{
   SipMessage* request = makeRequestInternal(INVITE);
   incrementCSeq(*request);
   DebugLog(<< "Dialog::makeInvite: " << *request);
   return request;
}


SipMessage*
Dialog::makeRegister()
{
   SipMessage* request = makeRequestInternal(REGISTER);
   incrementCSeq(*request);
   DebugLog(<< "Dialog::makeRegister: " << *request);
   return request;
}


SipMessage*
Dialog::makeSubscribe()
{
   SipMessage* request = makeRequestInternal(SUBSCRIBE);
   incrementCSeq(*request);
   DebugLog(<< "Dialog::makeSubscribe: " << *request);
   return request;
}

SipMessage*
Dialog::makeBye()
{
   SipMessage* request = makeRequestInternal(BYE);
   incrementCSeq(*request);

   return request;
}


SipMessage*
Dialog::makeRefer(const NameAddr& referTo)
{
   SipMessage* request = makeRequestInternal(REFER);
   request->header(h_ReferTo) = referTo;
   request->header(h_ReferredBy) = mLocalUri;
   incrementCSeq(*request);
   return request;
}

SipMessage*
Dialog::makeNotify()
{
   SipMessage* request = makeRequestInternal(NOTIFY);
   incrementCSeq(*request);
   return request;
}


SipMessage*
Dialog::makeOptions()
{
   SipMessage* request = makeRequestInternal(OPTIONS);
   incrementCSeq(*request);
   return request;
}

SipMessage*
Dialog::makeRequest(resip::MethodTypes method)
{
   assert(method != ACK);
   assert(method != CANCEL);
   
   SipMessage* request = makeRequestInternal(method);
   incrementCSeq(*request);
   return request;
}

SipMessage*
Dialog::makeAck(const SipMessage& original)
{
   SipMessage* request = makeRequestInternal(ACK);
   copyCSeq(*request);

   // !dcm! should we copy the authorizations? 
   // !jf! will this do the right thing if these headers weren't in original 
   // we should be able to store this stuff in the Dialog and not need to pass
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
Dialog::makeCancel(const SipMessage& request)
{
   assert (request.header(h_Vias).size() >= 1);
   assert (request.header(h_RequestLine).getMethod() == INVITE);
   
   SipMessage* cancel = new SipMessage;
   
   RequestLine rLine(CANCEL, request.header(h_RequestLine).getSipVersion());
   rLine.uri() = request.header(h_RequestLine).uri();
   cancel->header(h_RequestLine) = rLine;

   cancel->header(h_CallId) = request.header(h_CallId);
   cancel->header(h_To) = request.header(h_To); 
   cancel->header(h_From) = request.header(h_From);
   cancel->header(h_CSeq) = request.header(h_CSeq);
   cancel->header(h_CSeq).method() = CANCEL;
   cancel->header(h_Vias).push_back(request.header(h_Vias).front());
   if (request.exists(h_Routes))
   {
      cancel->header(h_Routes) = request.header(h_Routes);
   }
   
   return cancel;
}


CallId 
Dialog::makeReplaces()
{
   return mDialogId;
}

void
Dialog::clear()
{
//   mContact.clear(); // !cj! - likely need this 
   mCreated = false;
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
Dialog::makeRequestInternal(MethodTypes method)
{
   assert(mCreated);
   SipMessage* request = new SipMessage;
   RequestLine rLine(method);
   
   if ( method == REGISTER )
   {
       rLine.uri().scheme() = mRemoteTarget.uri().scheme();
       rLine.uri().host() = mRemoteTarget.uri().host();
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
   request->header(h_Contacts).push_front(mContact);
   request->header(h_CSeq).method() = method;
   copyCSeq(*request);
   request->header(h_MaxForwards).value() = 70;

   Via via;
   via.param(p_branch); // will create the branch
   request->header(h_Vias).push_front(via);

   //DebugLog(<<"Created a request within dialog: " << this << "  " << mContact);
   //DebugLog(<<"contact after copy: " <<     request->header(h_Contacts).front());
   return request;
}

void
Dialog::copyCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   request.header(h_CSeq).sequence() = mLocalSequence;
}

void
Dialog::incrementCSeq(SipMessage& request)
{
   if (mLocalEmpty)
   {
      mLocalSequence = 1;
      mLocalEmpty = false;
   }
   //DebugLog ( << "mLocalSequence: " << mLocalSequence);
   request.header(h_CSeq).sequence() = ++mLocalSequence;
}

std::ostream&
resip::operator<<(std::ostream& strm, Dialog& d)
{
   strm << "Dialog: [" << d.dialogId() 
        << " created=" << d.mCreated 
        << ",remoteTarget=" << d.mRemoteTarget 
      //<< "routeset=" << Inserter(d.mRouteSet) << std::endl
        << ",remoteSeq=" << d.mRemoteSequence 
        << ",remote=" << d.mRemoteUri 
        << ",remoteTag=" << d.mRemoteTag 
        << ",localSeq=" << d.mLocalSequence
        << ",local=" << d.mLocalUri 
        << ",localTag=" << d.mLocalTag 
        << "]";
   return strm;
}
  
 
const Data 
Dialog::dialogId() const
{
   return Data::from(mDialogId);
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
