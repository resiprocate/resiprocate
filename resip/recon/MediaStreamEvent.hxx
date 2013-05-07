#if !defined(MediaStreamEvent_hxx)
#define MediaStreamEvent_hxx

#include "ConversationManager.hxx"

#include <resip/dum/DumCommand.hxx>
#include <reTurn/StunTuple.hxx>

namespace recon
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
      EncodeStream& encode(EncodeStream& strm) const;
      EncodeStream& encodeBrief(EncodeStream& strm) const;

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
      EncodeStream& encode(EncodeStream& strm) const;
      EncodeStream& encodeBrief(EncodeStream& strm) const;

   private:
      RemoteParticipantDialogSet& mRemoteParticipantDialogSet;
      unsigned int mErrorCode;
};
}

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
