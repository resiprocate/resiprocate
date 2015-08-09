#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/DialogUsage.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/ResipAssert.h"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DialogUsage::Exception::Exception(const Data& msg,const Data& file,int line)
   : BaseException(msg, file, line)
{
}

const char*
DialogUsage::Exception::name() const
{
   return "DialogUsage::Exception";
}

DialogUsage::DialogUsage(DialogUsageManager& dum, Dialog& dialog) :
   BaseUsage(dum),
   mDialog(dialog)
{
}

DialogUsage::~DialogUsage()
{
   mDialog.possiblyDie();
}

AppDialogSetHandle 
DialogUsage::getAppDialogSet()
{
   if (mDialog.mDialogSet.mAppDialogSet == 0)
   {
      ErrLog(<< "mDialog.mDialogSet.mAppDialogSet is NULL!!!");
      return AppDialogSetHandle();
   }
   return mDialog.mDialogSet.mAppDialogSet->getHandle();
}

AppDialogHandle 
DialogUsage::getAppDialog()
{
   return mDialog.mAppDialog->getHandle();
}

const NameAddr&
DialogUsage::myAddr() const
{
   return mDialog.mLocalNameAddr;
}

const NameAddr&
DialogUsage::peerAddr() const
{
   return mDialog.mRemoteNameAddr;
}

const NameAddr&
DialogUsage::remoteTarget() const
{
   return mDialog.mRemoteTarget;
}

const DialogId& 
DialogUsage::getDialogId() const
{
   return mDialog.getId();
}

const Data& 
DialogUsage::getCallId() const
{
   return mDialog.getId().getCallId();
}

SharedPtr<UserProfile> 
DialogUsage::getUserProfile() 
{
   return mDialog.mDialogSet.getUserProfile();
}

void 
DialogUsage::send(SharedPtr<SipMessage> msg)
{
   // give app an chance to adorn the message.
   onReadyToSend(*msg);
   mDialog.send(msg);
}

void
DialogUsage::sendCommand(SharedPtr<SipMessage> message)
{   
   mDum.post(new DialogUsageSendCommand(*this, message));
}


/*
void 
DialogUsage::send(SipMessage& msg)
{
   resip_assert(msg.isResponse() || msg.header(h_RequestLine).method() == ACK);
   mDialog.send(msg);
}
*/

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
