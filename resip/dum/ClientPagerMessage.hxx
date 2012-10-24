#if !defined(RESIP_CLIENTPAGERMESSAGE_HXX)
#define RESIP_CLIENTPAGERMESSAGE_HXX

#include "resip/dum/NonDialogUsage.hxx"
#include "resip/stack/CSeqCategory.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include <deque>
#include <memory>

namespace resip
{
class SipMessage;

class ClientPagerMessage : public NonDialogUsage
{
  public:
      ClientPagerMessage(DialogUsageManager& dum, DialogSet& dialogSet);
      ClientPagerMessageHandle getHandle();

      //allow the user to adorn the MESSAGE message if desired
      //!kh!
      //I don't know how this would interact with the queuing mechanism.
      //Will come back to re-visit this in the future.
      SipMessage& getMessageRequest();
      SharedPtr<SipMessage> getMessageRequestSharedPtr() { return mRequest; }

      //!kh!
      //queues the message if there is one sent but not yet received a response
      //for it.
      //asserts if contents->get() is NULL.
      virtual void page(std::auto_ptr<Contents> contents, DialogUsageManager::EncryptionLevel level=DialogUsageManager::None);
      virtual void end();

      /**
       * Provide asynchronous method access by using command
       */
      virtual void endCommand();
      virtual void pageCommand(std::auto_ptr<Contents> contents, DialogUsageManager::EncryptionLevel level=DialogUsageManager::None);

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      size_t       msgQueued () const;

      virtual EncodeStream& dump(EncodeStream& strm) const;

   protected:
      virtual ~ClientPagerMessage();

   private:
      friend class DialogSet;

      //uses memory from creator
      //SipMessage& mRequest;
      SharedPtr<SipMessage> mRequest;

      typedef struct
      {
            DialogUsageManager::EncryptionLevel encryptionLevel;
            Contents* contents;
      } Item;

      typedef std::deque<Item> MsgQueue;
      MsgQueue mMsgQueue;
      bool mEnded;

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
