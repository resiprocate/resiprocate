#if !defined(ConversationManagerCmds_hxx)
#define ConversationManagerCmds_hxx

#include <resip/dum/DumCommand.hxx>
#include <rutil/Logger.hxx>

#include "ReconSubsystem.hxx"
#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "RemoteParticipant.hxx"
#include "LocalParticipant.hxx"
#include "MediaResourceParticipant.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

namespace recon
{

/**
  The classes defined here are used to pass commands from the
  application thread to the UserAgent thread (process loop).  
  This ensures thread safety of the Conversation Manager methods that are
  available to an application.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

// The purpose of this class is to stub out some methods which most commands
// Do not bother to implement.
class DumCommandStub : public resip::DumCommand
{
public:
   DumCommandStub() : CommandName( "Unnamed Command" ) {};
   DumCommandStub( const char *commandName ) : CommandName( commandName ) {};
   virtual ~DumCommandStub() {};
   virtual resip::Message* clone() const { assert(0); return 0; }
   virtual EncodeStream& encode(EncodeStream& strm) const { strm << " " << CommandName << ": "; return strm; }
   virtual EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
private:
   const char *CommandName;
};

class AddConversationProfileCmd  : public DumCommandStub
{
   public:  
      AddConversationProfileCmd(ConversationManager *conversationManager,
                                ConversationProfileHandle handle,
                                resip::SharedPtr<ConversationProfile> conversationProfile,
                                bool defaultOutgoing)
         : DumCommandStub( "AddConversationProfileCmd" ),
           mConversationManager(conversationManager),
           mHandle(handle),
           mConversationProfile(conversationProfile),
           mDefaultOutgoing(defaultOutgoing) {}
      virtual void executeCommand()
      {
         mConversationManager->addConversationProfileImpl(mHandle, mConversationProfile, mDefaultOutgoing);
      }
   private:
      ConversationManager* mConversationManager;
      ConversationProfileHandle mHandle;
      resip::SharedPtr<ConversationProfile> mConversationProfile;
      bool mDefaultOutgoing;
};

class SetDefaultOutgoingConversationProfileCmd  : public DumCommandStub
{
   public:  
      SetDefaultOutgoingConversationProfileCmd(ConversationManager *conversationManager,
                                               ConversationProfileHandle handle)
         : DumCommandStub( "SetDefaultOutgoingConversationProfileCmd" ),
           mConversationManager(conversationManager),
           mHandle(handle) {}
      virtual void executeCommand()
      {
         mConversationManager->setDefaultOutgoingConversationProfileImpl(mHandle);
      }
   private:
      ConversationManager* mConversationManager;
      ConversationProfileHandle mHandle;
};

class DestroyConversationProfileCmd : public DumCommandStub
{
   public:  
      DestroyConversationProfileCmd(ConversationManager *conversationManager,
                                    ConversationProfileHandle handle)
         : DumCommandStub( "DestroyConversationProfileCmd" ),
           mConversationManager(conversationManager),
           mHandle(handle) {}
      virtual void executeCommand()
      {
         mConversationManager->destroyConversationProfileImpl(mHandle);
      }
   private:
      ConversationManager* mConversationManager;
      ConversationProfileHandle mHandle;
};

class CreateConversationCmd : public DumCommandStub
{
   public:  
      CreateConversationCmd(ConversationManager* conversationManager, 
                            ConversationHandle convHandle,
                            ConversationProfileHandle cpHandle ) 
         : DumCommandStub( "CreateConversationCmd" ),
           mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mcpHandle(cpHandle) {}
      virtual void executeCommand()
      {
         resip::SharedPtr<ConversationProfile> cProfile;

         // If the handle is 0 (invalid), then just use the default
         // outgoing profile. Otherwise, try to fetch the real profile
         // from the user agent directly.
         if ( mcpHandle == 0 )
            cProfile = mConversationManager->getDefaultOutgoingConversationProfile();
         else
            cProfile = mConversationManager->getConversationProfile( mcpHandle );

         // the list of selected codecs (among other things) may have changed
         resip::Data ipaddr = resip::Data::Empty;
         mConversationManager->buildSessionCapabilities(cProfile->audioSupported(), cProfile->videoSupported(), ipaddr, cProfile->sessionCaps(), cProfile->sessionName());

         Conversation* conversation(new Conversation(mConvHandle, cProfile, *mConversationManager));
         assert(conversation);
			((void*)conversation);

         //mConversationManager->registerConversation( conversation );
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ConversationProfileHandle mcpHandle;
};

class DestroyConversationCmd : public DumCommandStub
{
   public:  
      DestroyConversationCmd(ConversationManager* conversationManager, 
                             ConversationHandle convHandle) 
         : DumCommandStub("DestroyConversationCmd"),
           mConversationManager(conversationManager),
           mConvHandle(convHandle) {}
      virtual void executeCommand()
      {
         Conversation* conv = mConversationManager->getConversation(mConvHandle);
         if(conv != NULL)
         {
            conv->destroy(); // cleanup
            //mConversationManager->unregisterConversation( conv );
         }
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
};

class JoinConversationCmd : public DumCommandStub
{
   public:  
      JoinConversationCmd(ConversationManager* conversationManager, 
                          ConversationHandle sourceConvHandle,
                          ConversationHandle destConvHandle) 
         : DumCommandStub("JoinConversationCmd"),
           mConversationManager(conversationManager),
           mSourceConvHandle(sourceConvHandle),
           mDestConvHandle(destConvHandle) {}
      virtual void executeCommand()
      {
         Conversation* sourceConversation = mConversationManager->getConversation(mSourceConvHandle);
         Conversation* destConversation = mConversationManager->getConversation(mDestConvHandle);
         if(sourceConversation && destConversation)
         {
            sourceConversation->join(destConversation);  // Join source Conversation into dest Conversation and destroy source conversation
         }
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
};

class CreateRemoteParticipantCmd : public DumCommandStub
{
   public:  
      CreateRemoteParticipantCmd(ConversationManager* conversationManager, 
                                 ParticipantHandle partHandle,
                                 ConversationHandle convHandle,
                                 const resip::NameAddr& destination,
                                 const ConversationManager::MediaAttributes& mediaAttributes,
                                 const ConversationManager::CallAttributes& callAttributes) 
         : DumCommandStub("CreateRemoteParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mDestination(destination),
           mMediaAttribs(mediaAttributes),
           mCallAttribs(callAttributes)
      {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation != NULL)
         {
            RemoteParticipantDialogSet* participantDialogSet = new RemoteParticipantDialogSet(*mConversationManager, mCallAttribs.forkSelectMode);
            RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mPartHandle); 
            if(participant)
            {
               conversation->addParticipant(participant);
               participant->initiateRemoteCall(conversation->getProfile(), mDestination, conversation, mMediaAttribs, mCallAttribs);
            }
            else
            {
               WarningLog(<< "CreateRemoteParticipantCmd: error creating UACOriginalRemoteParticipant.");
               mConversationManager->onParticipantDestroyed(mPartHandle);
            }
         }
         else
         {
            WarningLog(<< "CreateRemoteParticipantCmd: invalid conversation handle.");
            mConversationManager->onParticipantDestroyed(mPartHandle);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::NameAddr mDestination;
      ConversationManager::MediaAttributes mMediaAttribs;
      ConversationManager::CallAttributes mCallAttribs;
};

class CreateMediaResourceParticipantCmd : public DumCommandStub
{
   public:  
      CreateMediaResourceParticipantCmd(ConversationManager* conversationManager, 
                                        ParticipantHandle partHandle,
                                        ConversationHandle convHandle,
                                        resip::Uri& mediaUrl) 
         : DumCommandStub("CreateMediaResourceParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mMediaUrl(mediaUrl) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation != NULL)
         {
            MediaResourceParticipant* mediaResourceParticipant = new MediaResourceParticipant(mPartHandle, *mConversationManager, mMediaUrl);
            if(mediaResourceParticipant)
            {
               conversation->addParticipant(mediaResourceParticipant);
               mediaResourceParticipant->startPlay();
            }
            else
            {
               WarningLog(<< "CreateMediaResourceParticipantCmd: error creating MediaResourceParticipant.");
               mConversationManager->onParticipantDestroyed(mPartHandle);
            }
         }
         else
         {
            WarningLog(<< "CreateMediaResourceParticipantCmd: invalid conversation handle.");
            mConversationManager->onParticipantDestroyed(mPartHandle);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::Uri mMediaUrl;
};

class CreateLocalParticipantCmd : public DumCommandStub
{
   public:  
      CreateLocalParticipantCmd(ConversationManager* conversationManager, 
                                ParticipantHandle partHandle) 
         : DumCommandStub("CreateLocalParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         new LocalParticipant(mPartHandle, *mConversationManager);
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class DestroyParticipantCmd : public DumCommandStub
{
   public:  
      DestroyParticipantCmd(ConversationManager* conversationManager, 
                            ParticipantHandle partHandle,
                            const resip::Data& appDefinedReason = resip::Data::Empty) 
         : DumCommandStub("DestroyParticipantCmd"), mConversationManager(conversationManager), mPartHandle(partHandle), mReason(appDefinedReason) {}
      DestroyParticipantCmd(const DestroyParticipantCmd& rhs) 
         : DumCommandStub("DestroyParticipantCmd"), mConversationManager(rhs.mConversationManager), mPartHandle(rhs.mPartHandle), mReason(rhs.mReason) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         if(participant)
         {
            participant->destroyParticipant(mReason);
         }
      }
      resip::Message* clone() const { return new DestroyParticipantCmd(*this); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      resip::Data mReason;
};

class AddParticipantCmd : public DumCommandStub
{
   public:  
      AddParticipantCmd(ConversationManager* conversationManager, 
                        ConversationHandle convHandle,
                        ParticipantHandle partHandle) 
         : DumCommandStub("AddParticipantCmd"),
           mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && ( conversation != NULL ))
         {
            conversation->addParticipant(participant);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class RemoveParticipantCmd : public DumCommandStub
{
   public:  
      RemoveParticipantCmd(ConversationManager* conversationManager, 
                           ConversationHandle convHandle,
                           ParticipantHandle partHandle) 
         : DumCommandStub("RemoveParticipantCmd"),
           mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && ( conversation != NULL ))
         {
            conversation->removeParticipant(participant);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class MoveParticipantCmd : public DumCommandStub
{
   public:  
      MoveParticipantCmd(ConversationManager* conversationManager, 
                         ParticipantHandle partHandle,
                         ConversationHandle sourceConvHandle,
                         ConversationHandle destConvHandle,
                         bool bTriggerHold = false ) 
         : DumCommandStub("RemoveParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mSourceConvHandle(sourceConvHandle),
           mDestConvHandle(destConvHandle),
           mbTriggerHold(bTriggerHold) {}

      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* sourceConversation = mConversationManager->getConversation(mSourceConvHandle);
         Conversation* destConversation   = mConversationManager->getConversation(mDestConvHandle);
         if(participant && sourceConversation && destConversation)
         {
            // remove has to happen before add, since there could only be one mixer (optionally)
            sourceConversation->removeParticipant(participant, mbTriggerHold);
            destConversation->addParticipant(participant);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
      bool mbTriggerHold;
};

class ModifyParticipantContributionCmd : public DumCommandStub
{
   public:  
      ModifyParticipantContributionCmd(ConversationManager* conversationManager, 
                                       ConversationHandle convHandle,
                                       ParticipantHandle partHandle,
                                       unsigned int inputGain,
                                       unsigned int outputGain) 
         : DumCommandStub("ModifyParticipantContributionCmd"),
           mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle),
           mInputGain(inputGain),
           mOutputGain(outputGain) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && conversation)
         {
            conversation->modifyParticipantContribution(participant, mInputGain, mOutputGain);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
      unsigned int mInputGain;
      unsigned int mOutputGain;
};

class OutputBridgeMixWeightsCmd  : public DumCommandStub
{
   public:  
      OutputBridgeMixWeightsCmd(ConversationManager* conversationManager) 
         : DumCommandStub("OutputBridgeMixWeightsCmd"), mConversationManager(conversationManager) {}
      virtual void executeCommand()
      {
         mConversationManager->getBridgeMixer().outputBridgeMixWeights();
      }
   private:
      ConversationManager* mConversationManager;
};

class AlertParticipantCmd : public DumCommandStub
{
   public:  
      AlertParticipantCmd(ConversationManager* conversationManager, 
                          ParticipantHandle partHandle,
                          bool earlyFlag) 
         : DumCommandStub("AlertParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mEarlyFlag(earlyFlag) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->alert(mEarlyFlag);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      bool mEarlyFlag;
};

class AnswerParticipantCmd : public DumCommandStub
{
   public:  
      AnswerParticipantCmd(ConversationManager* conversationManager, 
                          ParticipantHandle partHandle,
                          ConversationManager::MediaAttributes mediaAttributes) 
         : DumCommandStub("AnswerParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mMediaAttributes(mediaAttributes) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->accept(mMediaAttributes);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationManager::MediaAttributes mMediaAttributes;
};

class RejectParticipantCmd : public DumCommandStub
{
   public:  
      RejectParticipantCmd(ConversationManager* conversationManager, 
                           ParticipantHandle partHandle,
                           unsigned int rejectCode) 
         : DumCommandStub("RejectParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mRejectCode(rejectCode) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->reject(mRejectCode);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      unsigned int mRejectCode;
};

class RedirectParticipantCmd : public DumCommandStub
{
   public:  
      RedirectParticipantCmd(ConversationManager* conversationManager, 
                             ParticipantHandle partHandle,
                             resip::NameAddr& destination) 
         : DumCommandStub("RedirectParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mDestination(destination) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->redirect(mDestination);
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      resip::NameAddr mDestination;
};

class RedirectToParticipantCmd : public DumCommandStub
{
   public:  
      RedirectToParticipantCmd(ConversationManager* conversationManager, 
                               ParticipantHandle partHandle,
                               ParticipantHandle destPartHandle) 
         : DumCommandStub("RedirectToParticipantCmd"),
           mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mDestPartHandle(destPartHandle) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         RemoteParticipant* destRemoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mDestPartHandle));
         if(remoteParticipant && destRemoteParticipant)
         {
            remoteParticipant->redirectToParticipant(destRemoteParticipant->getInviteSessionHandle());
         }
      }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ParticipantHandle mDestPartHandle;
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
