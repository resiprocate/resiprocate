#include "ConversationManager.hxx"
#include "UserAgentSubsystem.hxx"
#include "UserAgentDialogSetFactory.hxx"
#include "RemoteParticipant.hxx"
#include "DefaultDialogSet.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/dum/AppDialogSet.hxx>
#include <rutil/WinLeakCheck.hxx>

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

using namespace useragent;
using namespace resip;
using namespace std;


UserAgentDialogSetFactory::UserAgentDialogSetFactory(ConversationManager& conversationManager) :
    mConversationManager(conversationManager)
{
}

AppDialogSet* 
UserAgentDialogSetFactory::createAppDialogSet(DialogUsageManager& dum,
                                              const SipMessage& msg)
{
   switch(msg.method())
   {
   case INVITE:
      return new RemoteParticipantDialogSet(mConversationManager);
      break;
   default:
      return new DefaultDialogSet(mConversationManager);
      break;
   }
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
