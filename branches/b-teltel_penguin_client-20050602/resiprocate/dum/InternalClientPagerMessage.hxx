// ==========================================================================================================
// InternalClientPagerMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ClientPagerMessage.
// ==========================================================================================================
#ifndef RESIP_InternalClientPagerMessage_hxx
#define RESIP_InternalClientPagerMessage_hxx

#include <memory>

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ClientPagerMessage.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalClientPagerMessage_Page : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPagerMessage_Page);
      InternalClientPagerMessage_Page(ClientPagerMessageHandle&                           h, 
                                      std::auto_ptr<Contents>                             contents, 
                                      std::auto_ptr< std::map<resip::Data, resip::Data> > extraHeaders)
         : mClientPagerMessageHandle(h)
          ,mContents(contents)
          ,mExtraHeaders(extraHeaders) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPagerMessage_Page"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPagerMessageHandle.isValid())
         {
            mClientPagerMessageHandle->page(mContents, mExtraHeaders);
         }
      }
   private: // members.
      ClientPagerMessageHandle                            mClientPagerMessageHandle;
      std::auto_ptr<Contents>                             mContents;
      std::auto_ptr< std::map<resip::Data, resip::Data> > mExtraHeaders;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPagerMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPagerMessage_End);
      InternalClientPagerMessage_End(ClientPagerMessageHandle& h) : mClientPagerMessageHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPagerMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPagerMessageHandle.isValid())
         {
            mClientPagerMessageHandle->end();
         }
      }
   private: // members.
      ClientPagerMessageHandle  mClientPagerMessageHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPagerMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPagerMessage_DispatchSipMsg);
      InternalClientPagerMessage_DispatchSipMsg(ClientPagerMessageHandle& h, const SipMessage& sipMsg)
         : mClientPagerMessageHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPagerMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPagerMessageHandle.isValid())
         {
            mClientPagerMessageHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ClientPagerMessageHandle   mClientPagerMessageHandle;
      SipMessage                 mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPagerMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPagerMessage_DispatchTimeoutMsg);
      InternalClientPagerMessage_DispatchTimeoutMsg(ClientPagerMessageHandle& h, const DumTimeout& timeoutMsg)
         : mClientPagerMessageHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPagerMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPagerMessageHandle.isValid())
         {
            mClientPagerMessageHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ClientPagerMessageHandle   mClientPagerMessageHandle;
      DumTimeout                 mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalClientPagerMessage_hxx

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
