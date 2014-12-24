#if !defined(ConversationManagerCmds_hxx)
#define ConversationManagerCmds_hxx

#include <resip/dum/DumCommand.hxx>
#include <rutil/Logger.hxx>

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
class CreateConversationCmd  : public resip::DumCommand
{
   public:  
      CreateConversationCmd(ConversationManager* conversationManager, 
                            ConversationHandle convHandle,
                            bool broadcastOnly) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mBroadcastOnly(broadcastOnly) {}
      virtual void executeCommand()
      {
            Conversation* conversation = new Conversation(mConvHandle, *mConversationManager, 0, mBroadcastOnly);
            resip_assert(conversation);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " CreateConversationCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      bool mBroadcastOnly;
};

class DestroyConversationCmd  : public resip::DumCommand
{
   public:  
      DestroyConversationCmd(ConversationManager* conversationManager, 
                             ConversationHandle convHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation)
         {
            conversation->destroy();
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " DestroyConversationCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
};

class JoinConversationCmd  : public resip::DumCommand
{
   public:  
      JoinConversationCmd(ConversationManager* conversationManager, 
                          ConversationHandle sourceConvHandle,
                          ConversationHandle destConvHandle) 
         : mConversationManager(conversationManager),
           mSourceConvHandle(sourceConvHandle),
           mDestConvHandle(destConvHandle) {}
      virtual void executeCommand()
      {
         if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode)
         {
            WarningLog(<< "JoinConversationCmd: command not allowed in sipXConversationMediaInterfaceMode.");
         }
         else
         {
            Conversation* sourceConversation = mConversationManager->getConversation(mSourceConvHandle);
            Conversation* destConversation = mConversationManager->getConversation(mDestConvHandle);
            if(sourceConversation && destConversation)
            {
               if(sourceConversation == destConversation)
               {
                  // NoOp
                  return;
               }
               sourceConversation->join(destConversation);  // Join source Conversation into dest Conversation and destroy source conversation
            }
            else
            {
               if(!sourceConversation)
               {
                  WarningLog(<< "JoinConversationCmd: invalid source conversation handle.");
               }
               if(!destConversation)
               {
                  WarningLog(<< "JoinConversationCmd: invalid destination conversation handle.");
               }
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " JoinConversationCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
};

class CreateRemoteParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateRemoteParticipantCmd(ConversationManager* conversationManager, 
                                 ParticipantHandle partHandle,
                                 ConversationHandle convHandle,
                                 const resip::NameAddr& destination,
                                 ConversationManager::ParticipantForkSelectMode forkSelectMode,
                                 resip::SharedPtr<resip::UserProfile> callerProfile = resip::SharedPtr<resip::UserProfile>(),
                                 const std::multimap<resip::Data,resip::Data>& extraHeaders = (std::multimap<resip::Data,resip::Data>()))
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mDestination(destination),
           mForkSelectMode(forkSelectMode),
           mCallerProfile(callerProfile),
           mExtraHeaders(extraHeaders) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation)
         {
            RemoteParticipantDialogSet* participantDialogSet = new RemoteParticipantDialogSet(*mConversationManager, mForkSelectMode);
            RemoteParticipant *participant = participantDialogSet->createUACOriginalRemoteParticipant(mPartHandle); 
            if(participant)
            {
               conversation->addParticipant(participant);
               participant->initiateRemoteCall(mDestination, mCallerProfile, mExtraHeaders);
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
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " CreateRemoteParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::NameAddr mDestination;
      ConversationManager::ParticipantForkSelectMode mForkSelectMode;
      resip::SharedPtr<resip::UserProfile> mCallerProfile;
      std::multimap<resip::Data,resip::Data> mExtraHeaders;
};

class CreateMediaResourceParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateMediaResourceParticipantCmd(ConversationManager* conversationManager, 
                                        ParticipantHandle partHandle,
                                        ConversationHandle convHandle,
                                        const resip::Uri& mediaUrl) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mMediaUrl(mediaUrl) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation)
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
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " CreateMediaResourceParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::Uri mMediaUrl;
};

class CreateLocalParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateLocalParticipantCmd(ConversationManager* conversationManager, 
                                ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         new LocalParticipant(mPartHandle, *mConversationManager);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " CreateLocalParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class DestroyParticipantCmd  : public resip::DumCommand
{
   public:  
      DestroyParticipantCmd(ConversationManager* conversationManager, 
                            ParticipantHandle partHandle) 
         : mConversationManager(conversationManager), mPartHandle(partHandle) {}
      DestroyParticipantCmd(const DestroyParticipantCmd& rhs) 
         : mConversationManager(rhs.mConversationManager), mPartHandle(rhs.mPartHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         if(participant)
         {
            participant->destroyParticipant();
         }
      }
      resip::Message* clone() const { return new DestroyParticipantCmd(*this); }
      EncodeStream& encode(EncodeStream& strm) const { strm << " DestroyParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class AddParticipantCmd  : public resip::DumCommand
{
   public:  
      AddParticipantCmd(ConversationManager* conversationManager, 
                        ConversationHandle convHandle,
                        ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);

         if(participant && conversation)
         {
            if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode)
            {
               // Need to ensure, that we are not adding the participant to more than one conversation.
               if(participant->getConversations().size() > 0)
               {
                  WarningLog(<< "AddParticipantCmd: participants cannot belong to multiple conversations in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            conversation->addParticipant(participant);
         }
         else
         {
            if(!participant)
            {
               WarningLog(<< "AddParticipantCmd: invalid participant handle.");
            }
            if(!conversation)
            {
               WarningLog(<< "AddParticipantCmd: invalid conversation handle.");
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " AddParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class RemoveParticipantCmd  : public resip::DumCommand
{
   public:  
      RemoveParticipantCmd(ConversationManager* conversationManager, 
                           ConversationHandle convHandle,
                           ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && conversation)
         {
            if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode)
            {
               // Need to ensure, that only local participants can be removed from conversations
               if(!dynamic_cast<LocalParticipant*>(participant))
               {
                  WarningLog(<< "RemoveParticipantCmd: only local participants can be removed from conversations in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            conversation->removeParticipant(participant);
         }
         else
         {
            if(!participant)
            {
               WarningLog(<< "RemoveParticipantCmd: invalid participant handle.");
            }
            if(!conversation)
            {
               WarningLog(<< "RemoveParticipantCmd: invalid conversation handle.");
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class MoveParticipantCmd  : public resip::DumCommand
{
   public:  
      MoveParticipantCmd(ConversationManager* conversationManager, 
                         ParticipantHandle partHandle,
                         ConversationHandle sourceConvHandle,
                         ConversationHandle destConvHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mSourceConvHandle(sourceConvHandle),
           mDestConvHandle(destConvHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* sourceConversation = mConversationManager->getConversation(mSourceConvHandle);
         Conversation* destConversation   = mConversationManager->getConversation(mDestConvHandle);
         if(participant && sourceConversation && destConversation)
         {
            if(sourceConversation == destConversation)
            {
               // No-Op
               return;
            }
            if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode)
            {
               // Need to ensure, that only local participants can be moved between conversations
               if(!dynamic_cast<LocalParticipant*>(participant))
               {
                  WarningLog(<< "MoveParticipantCmd: only local participants can be moved between conversations in sipXConversationMediaInterfaceMode.");
                  return;
               }
               // Remove from old before adding to new conversation (since participants can't belong to multiple conversations
               // and only local participants can be moved in sipXConversationMediaInterfaceMode - no need to worry about the
               // hold/unhold issue that is mentioned in the 2nd half of the else statement for sipXGlobalMediaInterfaceMode)
               sourceConversation->removeParticipant(participant);
               destConversation->addParticipant(participant);
            }
            else
            {
               // Add to new conversation and remove from old (add before remove, so that hold/unhold won't happen)
               destConversation->addParticipant(participant);
               sourceConversation->removeParticipant(participant);
            }
         }
         else
         {
            if(!participant)
            {
               WarningLog(<< "MoveParticipantCmd: invalid participant handle.");
            }
            if(!sourceConversation)
            {
               WarningLog(<< "MoveParticipantCmd: invalid source conversation handle.");
            }
            if(!destConversation)
            {
               WarningLog(<< "MoveParticipantCmd: invalid destination conversation handle.");
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
};

class ModifyParticipantContributionCmd  : public resip::DumCommand
{
   public:  
      ModifyParticipantContributionCmd(ConversationManager* conversationManager, 
                                       ConversationHandle convHandle,
                                       ParticipantHandle partHandle,
                                       unsigned int inputGain,
                                       unsigned int outputGain) 
         : mConversationManager(conversationManager),
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
         else
         {
            if(!participant)
            {
               WarningLog(<< "ModifyParticipantContributionCmd: invalid participant handle.");
            }
            if(!conversation)
            {
               WarningLog(<< "ModifyParticipantContributionCmd: invalid conversation handle.");
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " ModifyParticipantContributionCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
      unsigned int mInputGain;
      unsigned int mOutputGain;
};

class OutputBridgeMixWeightsCmd  : public resip::DumCommand
{
   public:  
      OutputBridgeMixWeightsCmd(ConversationManager* conversationManager) 
         : mConversationManager(conversationManager) {}
      virtual void executeCommand()
      {
         resip_assert(mConversationManager->getBridgeMixer()!=0);
         mConversationManager->getBridgeMixer()->outputBridgeMixWeights();
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " OutputBridgeMixWeightsCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
};

class AlertParticipantCmd  : public resip::DumCommand
{
   public:  
      AlertParticipantCmd(ConversationManager* conversationManager, 
                          ParticipantHandle partHandle,
                          bool earlyFlag) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mEarlyFlag(earlyFlag) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode && mEarlyFlag)
            {
               // Need to ensure, that the remote paticipant is added to a conversation before doing an opertation that requires
               // media (ie. EarlyMediaFlag set to true).
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "AlertParticipantCmd: remote participants must to added to a conversation before alert with early flag can be used when in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->alert(mEarlyFlag);
         }
         else
         {
            WarningLog(<< "AlertParticipantCmd: invalid remote participant handle.");
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " AlertParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      bool mEarlyFlag;
};

class AnswerParticipantCmd  : public resip::DumCommand
{
   public:  
      AnswerParticipantCmd(ConversationManager* conversationManager, 
                          ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            if(mConversationManager->getMediaInterfaceMode() == ConversationManager::sipXConversationMediaInterfaceMode)
            {
               // Need to ensure, that the remote paticipant is added to a conversation before accepting the call
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "AnswerParticipantCmd: remote participant must to added to a conversation before calling accept in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->accept();
         }
         else
         {
            WarningLog(<< "AnswerParticipantCmd: invalid remote participant handle.");
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " AnswerParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class RejectParticipantCmd  : public resip::DumCommand
{
   public:  
      RejectParticipantCmd(ConversationManager* conversationManager, 
                           ParticipantHandle partHandle,
                           unsigned int rejectCode) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mRejectCode(rejectCode) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->reject(mRejectCode);
         }
         else
         {
            WarningLog(<< "RejectParticipantCmd: invalid remote participant handle.");
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " RejectParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      unsigned int mRejectCode;
};

class RedirectParticipantCmd  : public resip::DumCommand
{
   public:  
      RedirectParticipantCmd(ConversationManager* conversationManager, 
                             ParticipantHandle partHandle,
                             const resip::NameAddr& destination) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mDestination(destination) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->redirect(mDestination);
         }
         else
         {
            WarningLog(<< "RedirectParticipantCmd: invalid remote participant handle.");
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " RedirectParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      resip::NameAddr mDestination;
};

class RedirectToParticipantCmd  : public resip::DumCommand
{
   public:  
      RedirectToParticipantCmd(ConversationManager* conversationManager, 
                               ParticipantHandle partHandle,
                               ParticipantHandle destPartHandle) 
         : mConversationManager(conversationManager),
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
         else
         {
            if(!remoteParticipant)
            {
               WarningLog(<< "RedirectToParticipantCmd: invalid remote participant handle.");
            }
            if(!destRemoteParticipant)
            {
               WarningLog(<< "RedirectToParticipantCmd: invalid destination remote participant handle.");
            }
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " RedirectToParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
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
