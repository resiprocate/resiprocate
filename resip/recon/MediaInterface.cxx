#include "MediaInterface.hxx"
#include "ConversationManager.hxx"
#include "ReconSubsystem.hxx"
#include "DtmfEvent.hxx"

#include <mi/MiNotification.h>
#include <mi/MiDtmfNotf.h>
#include <mi/MiRtpStreamActivityNotf.h>
#include <mi/MiIntNotf.h>

#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

MediaInterface::MediaInterface(ConversationManager& conversationManager, 
                               ConversationHandle ownerConversationHandle, 
                               CpMediaInterface* mediaInterface) :
   mConversationManager(conversationManager),
   mOwnerConversationHandle(ownerConversationHandle),
   mMediaInterface(mediaInterface)
{
}

OsStatus 
MediaInterface::post(const OsMsg& msg)
{
   if((OsMsg::MsgTypes)msg.getMsgType() == OsMsg::MI_NOTF_MSG)
   {
      MiNotification* pNotfMsg = (MiNotification*)&msg;
      switch((MiNotification::NotfType)pNotfMsg->getType())
      {
      case MiNotification::MI_NOTF_PLAY_STARTED:
         InfoLog( << "MediaInterface: received MI_NOTF_PLAY_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_PAUSED:
         InfoLog( << "MediaInterface: received MI_NOTF_PLAY_PAUSED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_RESUMED:
         InfoLog( << "MediaInterface: received MI_NOTF_PLAY_RESUMED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_STOPPED:
         InfoLog( << "MediaInterface: received MI_NOTF_PLAY_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_FINISHED:
         {
            // Queue event to conversation manager thread
            MediaEvent* mevent = new MediaEvent(mConversationManager, pNotfMsg->getConnectionId(), mOwnerConversationHandle, MediaEvent::PLAY_FINISHED);
            mConversationManager.post(mevent);
            InfoLog( << "MediaInterface: received MI_NOTF_PLAY_FINISHED, sourceId=" << pNotfMsg->getSourceId().data() << 
               ", connectionId=" << pNotfMsg->getConnectionId() << 
               ", conversationHandle=" << mOwnerConversationHandle);
         }
         break;
      case MiNotification::MI_NOTF_PROGRESS:
         InfoLog( << "MediaInterface: received MI_NOTF_PROGRESS, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_STARTED:
         InfoLog( << "MediaInterface: received MI_NOTF_RECORD_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_STOPPED:
         InfoLog( << "MediaInterface: received MI_NOTF_RECORD_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_FINISHED:
         InfoLog( << "MediaInterface: received MI_NOTF_RECORD_FINISHED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_ERROR:
         InfoLog( << "MediaInterface: received MI_NOTF_RECORD_ERROR, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_DTMF_RECEIVED:
         {
            MiDtmfNotf* pDtmfNotfMsg = (MiDtmfNotf*)&msg;

            int duration = pDtmfNotfMsg->getDuration();  // in RTP timestamp units
            int durationMS = duration;
            if(duration >= 0) // negative durations indicate start of tone in sipXtapi
            {
               int rtpClockRate = 8000;   // FIXME - should use actual RTP clock rate of the flow graph?
               durationMS = (duration * 1000) / rtpClockRate;   //  convert from RTP timestamp units to milliseconds
               StackLog(<< "RTP clock rate = " << rtpClockRate << "Hz, duration (timestamp units) = " << duration << " = " << durationMS << "ms");
            }

            // Get event into dum queue, so that callback is on dum thread
            DtmfEvent* devent = new DtmfEvent(mConversationManager, mOwnerConversationHandle, pNotfMsg->getConnectionId(), pDtmfNotfMsg->getKeyCode(), durationMS, pDtmfNotfMsg->getKeyPressState()==MiDtmfNotf::KEY_UP);
            mConversationManager.post(devent);

            InfoLog( << "MediaInterface: received MI_NOTF_DTMF_RECEIVED, sourceId=" << pNotfMsg->getSourceId().data() << 
               ", connectionId=" << pNotfMsg->getConnectionId() << 
               ", conversationHandle=" << mOwnerConversationHandle <<
               ", keyCode=" << pDtmfNotfMsg->getKeyCode() << 
               ", state=" << pDtmfNotfMsg->getKeyPressState() << 
               ", duration=" << pDtmfNotfMsg->getDuration());
         }
         break;
      case MiNotification::MI_NOTF_DELAY_SPEECH_STARTED:
         InfoLog( << "MediaInterface: received MI_NOTF_DELAY_SPEECH_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_DELAY_NO_DELAY:
         InfoLog( << "MediaInterface: received MI_NOTF_DELAY_NO_DELAY, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_DELAY_QUIESCENCE:
         InfoLog( << "MediaInterface: received MI_NOTF_DELAY_QUIESCENCE, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RX_STREAM_ACTIVITY: ///< Value for MiRtpStreamActivityNotf notifications.
         {
            MiRtpStreamActivityNotf* pRtpStreamActivityNotfMsg = (MiRtpStreamActivityNotf*)&msg;
         
            InfoLog( << "MediaInterface: received MI_NOTF_RX_STREAM_ACTIVITY, sourceId=" << pNotfMsg->getSourceId().data() << 
               ", connectionId=" << pNotfMsg->getConnectionId() <<
               ", state=" << (pRtpStreamActivityNotfMsg->getState() == MiRtpStreamActivityNotf::STREAM_START ? "STREAM_START" :
                              pRtpStreamActivityNotfMsg->getState() == MiRtpStreamActivityNotf::STREAM_STOP ? "STREAM_STOP" :
                              pRtpStreamActivityNotfMsg->getState() == MiRtpStreamActivityNotf::STREAM_CHANGE ? "STREAM_CHANGE" : 
                              Data(pRtpStreamActivityNotfMsg->getState()).c_str()) <<
               ", ssrc=" << pRtpStreamActivityNotfMsg->getSsrc() <<
               ", address=" << pRtpStreamActivityNotfMsg->getAddress() <<
               ", port=" << pRtpStreamActivityNotfMsg->getPort());
         }
         break;
      case MiNotification::MI_NOTF_ENERGY_LEVEL:       ///< Audio energy level (MiIntNotf)
         {
            //MiIntNotf* pIntNotfMsg = (MiIntNotf*)&msg;
            //InfoLog( << "MediaInterface: received MI_NOTF_ENERGY_LEVEL, sourceId=" << pNotfMsg->getSourceId().data() << 
            //   ", connectionId=" << pNotfMsg->getConnectionId() <<
            //   ", value=" << pIntNotfMsg->getValue());
         }
         break;
      case MiNotification::MI_NOTF_VOICE_STARTED:
         //InfoLog( << "MediaInterface: received MI_NOTF_VOICE_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_VOICE_STOPPED:
         //InfoLog( << "MediaInterface: received MI_NOTF_VOICE_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;

      default:
         InfoLog(<< "MediaInterface: unrecognized MiNotification type = " << pNotfMsg->getType());
      }
   }
   else
   {
      InfoLog(<< "MediaInterface: unrecognized message type = " << msg.getMsgType());
   }
   return OS_SUCCESS;
}


/* ====================================================================

 Copyright (c) 2010, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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
