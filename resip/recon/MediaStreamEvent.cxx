#include "MediaStreamEvent.hxx"

#include "RemoteParticipantDialogSet.hxx"

#include <rutil/Logger.hxx>

using namespace useragent;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::USERAGENT

MediaStreamReadyEvent::MediaStreamReadyEvent(RemoteParticipantDialogSet& remoteParticipantDialogSet, 
                                             const reTurn::StunTuple& rtpTuple, 
                                             const reTurn::StunTuple& rtcpTuple) : 
   mRemoteParticipantDialogSet(remoteParticipantDialogSet),
   mRtpTuple(rtpTuple),
   mRtcpTuple(rtcpTuple)
{
}

void 
MediaStreamReadyEvent::executeCommand()
{
   mRemoteParticipantDialogSet.processMediaStreamReadyEvent(mRtpTuple, mRtcpTuple);
}

resip::Message* 
MediaStreamReadyEvent::clone() const
{
   assert(0);
   return 0;
}

std::ostream& 
MediaStreamReadyEvent::encode(std::ostream& strm) const
{
   strm << "MediaStreamReadyEvent: rtpTuple: " << mRtpTuple << " rtcpTuple=" << mRtcpTuple;
   return strm;
}

std::ostream& 
MediaStreamReadyEvent::encodeBrief(std::ostream& strm) const
{
   return encode(strm);
}


MediaStreamErrorEvent::MediaStreamErrorEvent(RemoteParticipantDialogSet& remoteParticipantDialogSet, 
                                             unsigned int errorCode) : 
   mRemoteParticipantDialogSet(remoteParticipantDialogSet),
   mErrorCode(errorCode)
{
}

void 
MediaStreamErrorEvent::executeCommand()
{
   mRemoteParticipantDialogSet.processMediaStreamErrorEvent(mErrorCode);
}

resip::Message* 
MediaStreamErrorEvent::clone() const
{
   assert(0);
   return 0;
}

std::ostream& 
MediaStreamErrorEvent::encode(std::ostream& strm) const
{
   strm << "MediaStreamErrorEvent: errorCode: " << mErrorCode;
   return strm;
}

std::ostream& 
MediaStreamErrorEvent::encodeBrief(std::ostream& strm) const
{
   return encode(strm);
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
