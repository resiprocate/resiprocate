#if !defined(MediaStreamEvent_hxx)
#define MediaStreamEvent_hxx

#include "ConversationManager.hxx"

#include <resip/dum/DumCommand.hxx>
#include <StunTuple.hxx>

namespace useragent
{

class RemoteParticipantDialogSet;
class Message;

/**
  This class represents events from the Media stream.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class MediaStreamReadyEvent : public resip::DumCommand
{
   public:
      MediaStreamReadyEvent(RemoteParticipantDialogSet& remoteParticipantDialogSet, const reTurn::StunTuple& rtpTuple, const reTurn::StunTuple& rtcpTuple);
      virtual void executeCommand();

      Message* clone() const;
      std::ostream& encode(std::ostream& strm) const;
      std::ostream& encodeBrief(std::ostream& strm) const;

   private:
      RemoteParticipantDialogSet& mRemoteParticipantDialogSet;
      reTurn::StunTuple mRtpTuple;
      reTurn::StunTuple mRtcpTuple;
};

class MediaStreamErrorEvent : public resip::DumCommand
{
   public:
      MediaStreamErrorEvent(RemoteParticipantDialogSet& remoteParticipantDialogSet, unsigned int errorCode);
      virtual void executeCommand();

      Message* clone() const;
      std::ostream& encode(std::ostream& strm) const;
      std::ostream& encodeBrief(std::ostream& strm) const;

   private:
      RemoteParticipantDialogSet& mRemoteParticipantDialogSet;
      unsigned int mErrorCode;
};
}

#endif


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
