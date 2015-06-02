#if !defined(RESIP_DIALOGUSAGE_HXX)
#define RESIP_DIALOGUSAGE_HXX

#include "rutil/BaseException.hxx"
#include "rutil/SharedPtr.hxx"
#include "resip/dum/DumCommand.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/dum/Handles.hxx"

namespace resip
{

class DialogUsageManager;
class Dialog;
class DialogId;
class DumTimeout;
class SipMessage;
class NameAddr;

class DialogUsage : public BaseUsage
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,const Data& file,int line);
            virtual const char* name() const;
      };

      AppDialogSetHandle getAppDialogSet();
      AppDialogHandle getAppDialog();     

      // Convenience methods for accessing attributes of a dialog. 
      const NameAddr& myAddr() const;
      const NameAddr& peerAddr() const;
      const NameAddr& remoteTarget() const;
      const DialogId& getDialogId() const;
      const Data& getCallId() const;
      SharedPtr<UserProfile> getUserProfile();
      
   protected:
      friend class DialogSet;
      friend class DialogUsageManager;
      friend class DialogUsageSendCommand;

      class DialogUsageSendCommand : public DumCommandAdapter
      {
      public:
         DialogUsageSendCommand(DialogUsage& usage, SharedPtr<SipMessage> msg)
            : mDialogUsage(usage),
              mMessage(msg)
         {
         }
      
         virtual void executeCommand()
         {
            mDialogUsage.send(mMessage);
         }
      
         virtual EncodeStream& encodeBrief(EncodeStream& strm) const
         {
            return strm << "DialogUsageSendCommand";
         }
      private:
         DialogUsage& mDialogUsage;
         SharedPtr<SipMessage> mMessage;
      };
      

      //virtual void send(SipMessage& msg);
      virtual void send(SharedPtr<SipMessage> msg);      
      virtual void sendCommand(SharedPtr<SipMessage> msg);

      // any usage that wants to give app a chance to adorn the message
      // should override this method.
      virtual void onReadyToSend(SipMessage&) {}

      DialogUsage(DialogUsageManager& dum, Dialog& dialog);
      virtual ~DialogUsage();

      Dialog& mDialog;
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
