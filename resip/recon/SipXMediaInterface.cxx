#include "SipXMediaInterface.hxx"
#include "SipXMediaStackAdapter.hxx"
#include "ReconSubsystem.hxx"
#include "DtmfEvent.hxx"
#include "FlowManagerSipXSocket.hxx"

#include <CpTopologyGraphInterface.h>
#include <mp/MpResourceTopology.h>
#include <mi/MiNotification.h>
#include <mi/MiDtmfNotf.h>
#include <mi/MiRtpStreamActivityNotf.h>
#include <mi/MiIntNotf.h>

#include <rutil/Lock.hxx>
#include <rutil/Logger.hxx>

using namespace recon;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

SipXMediaInterface::SipXMediaInterface(ConversationManager& conversationManager, CpTopologyGraphInterface* mediaInterface) :
   mConversationManager(conversationManager),
   mMediaInterface(mediaInterface),
   mAllowLoggingDTMFDigits(true)
{
   InfoLog(<< "SipXMediaInterface: CpTopologyGraphInterface created: 0x" << std::hex << mMediaInterface);

   // Add the available sipX resources.  Note:  This list must match what is provided by the sipX CpTopologyGraphImpl
   // default initial resources, plus any customizations done in SipXMediaStackAdapter::addExtraPlayAndRecordResourcesToTopology
   mMediaResourceAllocations[MediaResourceParticipant::Tone].push_back(MediaResourceAllocationInfo(DEFAULT_TONE_GEN_RESOURCE_NAME));
   mMediaResourceAllocations[MediaResourceParticipant::File].push_back(MediaResourceAllocationInfo(DEFAULT_FROM_FILE_RESOURCE_NAME));  // Note:  File resource also tracks Cache and Buffer types
   mMediaResourceAllocations[MediaResourceParticipant::Record].push_back(MediaResourceAllocationInfo(DEFAULT_RECORDER_RESOURCE_NAME));
   SipXMediaStackAdapter& mediaStackAdapter = static_cast<SipXMediaStackAdapter&>(conversationManager.getMediaStackAdapter());
   if (mediaStackAdapter.extraPlayAndRecordResourcesEnabled())
   {
      mMediaResourceAllocations[MediaResourceParticipant::File].push_back(MediaResourceAllocationInfo(SipXMediaStackAdapter::DEFAULT_FROM_FILE_2_RESOURCE_NAME));
      mMediaResourceAllocations[MediaResourceParticipant::Record].push_back(MediaResourceAllocationInfo(SipXMediaStackAdapter::DEFAULT_RECORDER_2_RESOURCE_NAME));
   }
}

SipXMediaInterface::~SipXMediaInterface() 
{ 
   InfoLog(<< "~SipXMediaInterface: releasing CpTopologyGraphInterface: 0x" << std::hex << mMediaInterface);
   mMediaInterface->release(); 
}

OsStatus 
SipXMediaInterface::createConnection(int& connectionId, ParticipantHandle partHandle, FlowManagerSipXSocket* rtpSocket, FlowManagerSipXSocket* rtcpSocket, bool isMulticast)
{
   resip_assert(mMediaInterface);
   OsStatus ret = mMediaInterface->createConnection(connectionId, rtpSocket, rtcpSocket, isMulticast);
   updateConnectionIdToPartipantHandleMapping(connectionId, partHandle);
   return ret;
}

OsStatus 
SipXMediaInterface::createConnection(int& connectionId, ParticipantHandle partHandle, const char* localAddress, int localPort)
{
   resip_assert(mMediaInterface);
   OsStatus ret = mMediaInterface->createConnection(connectionId, localAddress, localPort);
   updateConnectionIdToPartipantHandleMapping(connectionId, partHandle);
   return ret;
}

void 
SipXMediaInterface::updateConnectionIdToPartipantHandleMapping(int connectionId, ParticipantHandle partHandle)
{
   resip::Lock lock(mConnectionIdToParticipantHandleMapMutex);
   mConnectionIdToPartipantHandleMap[connectionId] = partHandle;
}

OsStatus 
SipXMediaInterface::deleteConnection(int connectionId)
{
   resip_assert(mMediaInterface);
   OsStatus ret = mMediaInterface->deleteConnection(connectionId);
   {
      resip::Lock lock(mConnectionIdToParticipantHandleMapMutex);
      mConnectionIdToPartipantHandleMap.erase(connectionId);
   }
   return ret;
}

ParticipantHandle 
SipXMediaInterface::getParticipantHandleForConnectionId(int connectionId)
{
   ParticipantHandle partHandle = 0;
   resip::Lock lock(mConnectionIdToParticipantHandleMapMutex);
   auto it = mConnectionIdToPartipantHandleMap.find(connectionId);
   if (it != mConnectionIdToPartipantHandleMap.end())
   {
      partHandle = it->second;
   }
   return partHandle;
}

ParticipantHandle 
SipXMediaInterface::getParticipantHandleForMediaResource(MediaResourceParticipant::ResourceType resourceType, const resip::Data& sipXResourceName)
{
   ParticipantHandle partHandle = 0;
   resip::Lock lock(mMediaResourceAllocationsMutex);
   std::list<MediaResourceAllocationInfo>& resourceInfos = getAllocationResourceInfos(resourceType);
   // Walk the list and find the resource name
   for (auto it = resourceInfos.begin(); it != resourceInfos.end(); it++)
   {
      if (it->mSipXResourceName == sipXResourceName)
      {
         resip_assert(it->mAllocatedParticipantHandle != 0);
         partHandle = it->mAllocatedParticipantHandle;
         break;
      }
   }
   return partHandle;
}

// ensure mutex is grabbed before calling
std::list<SipXMediaInterface::MediaResourceAllocationInfo>& SipXMediaInterface::getAllocationResourceInfos(MediaResourceParticipant::ResourceType resourceType)
{
   if (resourceType == MediaResourceParticipant::Cache ||
       resourceType == MediaResourceParticipant::Buffer)
   {
      // Cache and File resources are the same sipX resource, track both under the File type
      resourceType = MediaResourceParticipant::File;
   }
   resip_assert(mMediaResourceAllocations.find(resourceType) != mMediaResourceAllocations.end());
   return mMediaResourceAllocations[resourceType];
}

bool 
SipXMediaInterface::allocateAvailableResourceForMediaOperation(MediaResourceParticipant::ResourceType resourceType, ParticipantHandle partHandle, Data& allocatedResourceName)
{
   resip::Lock lock(mMediaResourceAllocationsMutex);
   std::list<MediaResourceAllocationInfo>& resourceInfos = getAllocationResourceInfos(resourceType);
   // Walk the list and find an unallocated resource
   for (auto it = resourceInfos.begin(); it != resourceInfos.end(); it++)
   {
      if (it->mAllocatedParticipantHandle == 0)
      {
         allocatedResourceName = it->mSipXResourceName;
         it->mAllocatedParticipantHandle = partHandle;
         return true;
      }
   }
   return false;
}

void 
SipXMediaInterface::unallocateResourceForMediaOperation(MediaResourceParticipant::ResourceType resourceType, ParticipantHandle partHandle)
{
   resip::Lock lock(mMediaResourceAllocationsMutex);
   std::list<MediaResourceAllocationInfo>& resourceInfos = getAllocationResourceInfos(resourceType);
   // Walk the list and find the allocated resource, then disassociate the participant handle to signify it is unallocated
   for (auto it = resourceInfos.begin(); it != resourceInfos.end(); it++)
   {
      if (it->mAllocatedParticipantHandle == partHandle)
      {
         it->mAllocatedParticipantHandle = 0;
         break;
      }
   }
}

OsStatus 
SipXMediaInterface::post(const OsMsg& msg)
{
   if((OsMsg::MsgTypes)msg.getMsgType() == OsMsg::MI_NOTF_MSG)
   {
      MiNotification* pNotfMsg = (MiNotification*)&msg;
      switch((MiNotification::NotfType)pNotfMsg->getType())
      {
      case MiNotification::MI_NOTF_PLAY_STARTED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_PLAY_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_PAUSED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_PLAY_PAUSED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_RESUMED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_PLAY_RESUMED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_STOPPED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_PLAY_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_PLAY_FINISHED:
         {
            // Queue event to conversation manager thread
            ParticipantHandle partHandle = getParticipantHandleForMediaResource(MediaResourceParticipant::File, pNotfMsg->getSourceId().data());
            MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::RESOURCE_DONE, MediaEvent::DIRECTION_OUT);
            mConversationManager.post(mevent);
            InfoLog(<< "SipXMediaInterface: received MI_NOTF_PLAY_FINISHED, sourceId=" << pNotfMsg->getSourceId().data() <<
               ", connectionId=" << pNotfMsg->getConnectionId() << ", participantHandle=" << partHandle);
         }
         break;
      case MiNotification::MI_NOTF_PLAY_ERROR:
         {
            // Queue event to conversation manager thread
            ParticipantHandle partHandle = getParticipantHandleForMediaResource(MediaResourceParticipant::File, pNotfMsg->getSourceId().data());
            MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::RESOURCE_FAILED, MediaEvent::DIRECTION_OUT);
            mConversationManager.post(mevent);
            InfoLog(<< "SipXMediaInterface: received MI_NOTF_PLAY_ERROR, sourceId=" << pNotfMsg->getSourceId().data() <<
               ", connectionId=" << pNotfMsg->getConnectionId() <<  ", participantHandle=" << partHandle);
         }
         break;
      case MiNotification::MI_NOTF_PROGRESS:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_PROGRESS, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_STARTED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_RECORD_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
#ifndef SIPX_NO_RECORD
      case MiNotification::MI_NOTF_RECORD_PAUSED:
         InfoLog(<< "SipXMediaInterface: received MI_NOTF_RECORD_PAUSED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_RESUMED:
         InfoLog(<< "SipXMediaInterface: received MI_NOTF_RECORD_RESUMED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
#endif
      case MiNotification::MI_NOTF_RECORD_STOPPED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_RECORD_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORD_FINISHED:
         {
            // Queue event to conversation manager thread
            ParticipantHandle partHandle = getParticipantHandleForMediaResource(MediaResourceParticipant::Record, pNotfMsg->getSourceId().data());
            MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::RESOURCE_DONE, MediaEvent::DIRECTION_IN);
            mConversationManager.post(mevent);
            InfoLog(<< "SipXMediaInterface: received MI_NOTF_RECORD_FINISHED, sourceId=" << pNotfMsg->getSourceId().data() <<
               ", connectionId=" << pNotfMsg->getConnectionId() << ", participantHandle=" << partHandle);
         }
         break;
      case MiNotification::MI_NOTF_RECORD_ERROR:
      {
         // Queue event to conversation manager thread
         ParticipantHandle partHandle = getParticipantHandleForMediaResource(MediaResourceParticipant::Record, pNotfMsg->getSourceId().data());
         MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::RESOURCE_FAILED, MediaEvent::DIRECTION_IN);
         mConversationManager.post(mevent);
         InfoLog(<< "SipXMediaInterface: received MI_NOTF_RECORD_ERROR, sourceId=" << pNotfMsg->getSourceId().data() <<
            ", connectionId=" << pNotfMsg->getConnectionId() << ", participantHandle=" << partHandle);
      }
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

            ParticipantHandle partHandle = getParticipantHandleForConnectionId(pNotfMsg->getConnectionId());

            if (mAllowLoggingDTMFDigits)
            {
               InfoLog(<< "SipXMediaInterface: received MI_NOTF_DTMF_RECEIVED, sourceId=" << pNotfMsg->getSourceId().data() <<
                  ", connectionId=" << pNotfMsg->getConnectionId() <<
                  ", participantHandle=" << partHandle <<
                  ", keyCode=" << pDtmfNotfMsg->getKeyCode() <<
                  ", state=" << pDtmfNotfMsg->getKeyPressState() <<
                  ", duration=" << pDtmfNotfMsg->getDuration());
            }
            else
            {
               InfoLog(<< "SipXMediaInterface: received MI_NOTF_DTMF_RECEIVED, sourceId=" << pNotfMsg->getSourceId().data() <<
                  ", connectionId=" << pNotfMsg->getConnectionId() <<
                  ", participantHandle=" << partHandle <<
                  ", keyCode=<HIDDEN>" <<
                  ", state=" << pDtmfNotfMsg->getKeyPressState() <<
                  ", duration=" << pDtmfNotfMsg->getDuration());
            }

            // Get event into dum queue, so that callback is on dum thread
            DtmfEvent* devent = new DtmfEvent(mConversationManager, partHandle, pDtmfNotfMsg->getKeyCode(), durationMS, pDtmfNotfMsg->getKeyPressState()==MiDtmfNotf::KEY_UP);
            mConversationManager.post(devent);
         }
         break;
      case MiNotification::MI_NOTF_DELAY_SPEECH_STARTED:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_DELAY_SPEECH_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_DELAY_NO_DELAY:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_DELAY_NO_DELAY, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_DELAY_QUIESCENCE:
         InfoLog( << "SipXMediaInterface: received MI_NOTF_DELAY_QUIESCENCE, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RX_STREAM_ACTIVITY: ///< Value for MiRtpStreamActivityNotf notifications.
         {
            MiRtpStreamActivityNotf* pRtpStreamActivityNotfMsg = (MiRtpStreamActivityNotf*)&msg;
         
            DebugLog( << "SipXMediaInterface: received MI_NOTF_RX_STREAM_ACTIVITY, sourceId=" << pNotfMsg->getSourceId().data() <<
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
         //{
         //   MiIntNotf* pIntNotfMsg = (MiIntNotf*)&msg;
         //   DebugLog( << "MediaInterface: received MI_NOTF_ENERGY_LEVEL, sourceId=" << pNotfMsg->getSourceId().data() << 
         //      ", connectionId=" << pNotfMsg->getConnectionId() <<
         //      ", value=" << pIntNotfMsg->getValue());
         //}
         break;
      case MiNotification::MI_NOTF_VOICE_STARTED:
         {
#ifndef SIPX_NO_RECORD
            MediaEvent::MediaDirection direction = (pNotfMsg->getSourceId().contains(DIRECTION_IN_SUFFIX) || pNotfMsg->getSourceId().contains(MIC_NAME_SUFFIX)) ? MediaEvent::DIRECTION_IN : MediaEvent::DIRECTION_OUT;
            DebugLog(<< "MediaInterface: received MI_NOTF_VOICE_STARTED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId() << ", direction=" << (direction == MediaEvent::DIRECTION_IN ? "IN" : "OUT"));
            ParticipantHandle partHandle = getParticipantHandleForConnectionId(pNotfMsg->getConnectionId());
   
            // Get event into dum queue, so that callback is on dum thread
            MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::VOICE_STARTED, direction);
            mConversationManager.post(mevent);
#endif
         }
         break;
      case MiNotification::MI_NOTF_VOICE_STOPPED:
         {
#ifndef SIPX_NO_RECORD
            MediaEvent::MediaDirection direction = (pNotfMsg->getSourceId().contains(DIRECTION_IN_SUFFIX) || pNotfMsg->getSourceId().contains(MIC_NAME_SUFFIX)) ? MediaEvent::DIRECTION_IN : MediaEvent::DIRECTION_OUT;
            DebugLog(<< "MediaInterface: received MI_NOTF_VOICE_STOPPED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId() << ", direction=" << (direction == MediaEvent::DIRECTION_IN ? "IN" : "OUT"));
            ParticipantHandle partHandle = getParticipantHandleForConnectionId(pNotfMsg->getConnectionId());

            // Get event into dum queue, so that callback is on dum thread
            MediaEvent* mevent = new MediaEvent(mConversationManager, partHandle, MediaEvent::VOICE_STOPPED, direction);
            mConversationManager.post(mevent);
#endif
         }
         break;
#ifndef SIPX_NO_RECORD
      case MiNotification::MI_NOTF_TONE_DETECT_ON:
         InfoLog(<< "MediaInterface: received MI_NOTF_TONE_DETECT_ON, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
#endif
      case MiNotification::MI_NOTF_H264_SPS:
         InfoLog(<< "MediaInterface: received MI_NOTF_H264_SPS, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_H264_PPS:
         InfoLog( << "MediaInterface: received MI_NOTF_H264_PPS, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
#ifndef SIPX_NO_RECORD
      case MiNotification::MI_NOTF_TONE_DETECT_OFF:
         InfoLog( << "MediaInterface: received MI_NOTF_TONE_DETECT_OFF, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_RECORDER_CIRCULARBUFFER_WATERMARK_REACHED:
         WarningLog(<< "MediaInterface: received MI_NOTF_RECORDER_CIRCULARBUFFER_WATERMARK_REACHED, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_INPUT_DEVICE_NOT_PRESENT:
         InfoLog(<< "MediaInterface: received MI_NOTF_INPUT_DEVICE_NOT_PRESENT, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
      case MiNotification::MI_NOTF_OUTPUT_DEVICE_NOT_PRESENT:
         InfoLog(<< "MediaInterface: received MI_NOTF_OUTPUT_DEVICE_NOT_PRESENT, sourceId=" << pNotfMsg->getSourceId().data() << ", connectionId=" << pNotfMsg->getConnectionId());
         break;
#endif

      default:
         InfoLog(<< "SipXMediaInterface: unrecognized MiNotification type = " << pNotfMsg->getType());
      }
   }
   else
   {
      InfoLog(<< "SipXMediaInterface: unrecognized message type = " << msg.getMsgType());
   }
   return OS_SUCCESS;
}


/* ====================================================================

 Copyright (c) 2010-2021, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
