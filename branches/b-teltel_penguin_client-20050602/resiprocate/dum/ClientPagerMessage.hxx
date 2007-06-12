#if !defined(RESIP_CLIENTPAGERMESSAGE_HXX)
#define RESIP_CLIENTPAGERMESSAGE_HXX

#include "resiprocate/dum/NonDialogUsage.hxx"
#include "resiprocate/CSeqCategory.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

#include <deque>
#include <map>
#include <memory>

namespace resip
{
class SipMessage;

class DUM_API ClientPagerMessage : public NonDialogUsage
{
  public:
      friend class InternalClientPagerMessage_Page;
      friend class InternalClientPagerMessage_End;
      friend class InternalClientPagerMessage_DispatchSipMsg;
      friend class InternalClientPagerMessage_DispatchTimeoutMsg;

      ClientPagerMessage(DialogUsageManager& dum, DialogSet& dialogSet);
      ClientPagerMessageHandle getHandle();

      typedef std::auto_ptr<std::map<resip::Data, resip::Data> > HeaderNameValueType;

      virtual void pageAsync(std::auto_ptr<Contents> contents, 
         HeaderNameValueType extraHeaders = HeaderNameValueType());
      virtual void endAsync();
      virtual void dispatchAsync(const SipMessage& msg);
      virtual void dispatchAsync(const DumTimeout& timer);

      //allow the user to adorn the MESSAGE message if desired
      //!kh!
      //I don't know how this would interact with the queuing mechanism.
      //Will come back to re-visit this in the future.
      SipMessage& getMessageRequest();
      size_t      msgQueued() const;

   protected:
      //!kh!
      //queues the message if there is one sent but not yet received a response
      //for it.
      //asserts if contents->get() is NULL.
      virtual void page(std::auto_ptr<Contents> contents, 
         std::auto_ptr< std::map<resip::Data, resip::Data> > extraHeaders = std::auto_ptr< std::map<resip::Data, resip::Data> >());
      virtual void end();
      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

   protected:
      virtual ~ClientPagerMessage();

   private:
      friend class DialogSet;

      //uses memory from creator
      SipMessage& mRequest;
      
      class MessageContents
      {
      public:
         MessageContents(Contents* contents = 0, std::map<resip::Data, resip::Data>* extraHeadersMap = 0)
            :mContents(contents)
            ,mExtraHeadersMap(extraHeadersMap)
         {
         }

         void reset()
         {
            delete mContents;
            mContents = NULL;
            delete mExtraHeadersMap;
            mExtraHeadersMap = NULL;
         }

         Contents* mContents;
         std::map<resip::Data, resip::Data>* mExtraHeadersMap;
      };


      typedef std::deque<MessageContents> MsgQueue;
      MsgQueue mMsgQueue;

      // disabled
      ClientPagerMessage(const ClientPagerMessage&);
      ClientPagerMessage& operator=(const ClientPagerMessage&);

      void pageFirstMsgQueued ();
      void clearMsgQueued ();
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
