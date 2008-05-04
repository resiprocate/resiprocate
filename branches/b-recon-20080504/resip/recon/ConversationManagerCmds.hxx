#if !defined(ConversationManagerCmds_hxx)
#define ConversationManagerCmds_hxx

#include <resip/dum/DumCommand.hxx>

#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "RemoteParticipant.hxx"
#include "LocalParticipant.hxx"
#include "MediaResourceParticipant.hxx"

#define RESIPROCATE_SUBSYSTEM UserAgentSubsystem::USERAGENT

namespace useragent
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
                            ConversationManager::ConversationHandle convHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle) {}
      virtual void executeCommand()
      {
            Conversation* conversation = new Conversation(mConvHandle, *mConversationManager);
            assert(conversation);
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " CreateConversationCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mConvHandle;
};

class DestroyConversationCmd  : public resip::DumCommand
{
   public:  
      DestroyConversationCmd(ConversationManager* conversationManager, 
                             ConversationManager::ConversationHandle convHandle) 
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
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " DestroyConversationCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mConvHandle;
};

class JoinConversationCmd  : public resip::DumCommand
{
   public:  
      JoinConversationCmd(ConversationManager* conversationManager, 
                          ConversationManager::ConversationHandle sourceConvHandle,
                          ConversationManager::ConversationHandle destConvHandle) 
         : mConversationManager(conversationManager),
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
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " JoinConversationCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mSourceConvHandle;
      ConversationManager::ConversationHandle mDestConvHandle;
};

class CreateRemoteParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateRemoteParticipantCmd(ConversationManager* conversationManager, 
                                 ConversationManager::ParticipantHandle partHandle,
                                 ConversationManager::ConversationHandle convHandle,
                                 resip::NameAddr& destination,
                                 ConversationManager::ParticipantForkSelectMode forkSelectMode) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mDestination(destination),
           mForkSelectMode(forkSelectMode) {}
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
               participant->initiateRemoteCall(mDestination);
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
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " CreateRemoteParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      ConversationManager::ConversationHandle mConvHandle;
      resip::NameAddr mDestination;
      ConversationManager::ParticipantForkSelectMode mForkSelectMode;
};

class CreateMediaResourceParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateMediaResourceParticipantCmd(ConversationManager* conversationManager, 
                                        ConversationManager::ParticipantHandle partHandle,
                                        ConversationManager::ConversationHandle convHandle,
                                        resip::Uri& mediaUrl) 
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
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " CreateMediaResourceParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      ConversationManager::ConversationHandle mConvHandle;
      resip::Uri mMediaUrl;
};

class CreateLocalParticipantCmd  : public resip::DumCommand
{
   public:  
      CreateLocalParticipantCmd(ConversationManager* conversationManager, 
                                ConversationManager::ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         new LocalParticipant(mPartHandle, *mConversationManager);
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " CreateLocalParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
};

class DestroyParticipantCmd  : public resip::DumCommand
{
   public:  
      DestroyParticipantCmd(ConversationManager* conversationManager, 
                            ConversationManager::ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         if(participant)
         {
            participant->destroyParticipant();
         }
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " DestroyParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
};

class AddParticipantCmd  : public resip::DumCommand
{
   public:  
      AddParticipantCmd(ConversationManager* conversationManager, 
                        ConversationManager::ConversationHandle convHandle,
                        ConversationManager::ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && conversation)
         {
            conversation->addParticipant(participant);
         }
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " AddParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mConvHandle;
      ConversationManager::ParticipantHandle mPartHandle;
};

class RemoveParticipantCmd  : public resip::DumCommand
{
   public:  
      RemoveParticipantCmd(ConversationManager* conversationManager, 
                           ConversationManager::ConversationHandle convHandle,
                           ConversationManager::ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(participant && conversation)
         {
            conversation->removeParticipant(participant);
         }
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mConvHandle;
      ConversationManager::ParticipantHandle mPartHandle;
};

class MoveParticipantCmd  : public resip::DumCommand
{
   public:  
      MoveParticipantCmd(ConversationManager* conversationManager, 
                         ConversationManager::ParticipantHandle partHandle,
                         ConversationManager::ConversationHandle sourceConvHandle,
                         ConversationManager::ConversationHandle destConvHandle) 
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
            // Add to new conversation and remove from old (add before remove, so that hold/unhold won't happen)
            destConversation->addParticipant(participant);
            sourceConversation->removeParticipant(participant);
         }
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      ConversationManager::ConversationHandle mSourceConvHandle;
      ConversationManager::ConversationHandle mDestConvHandle;
};

class ModifyParticipantContributionCmd  : public resip::DumCommand
{
   public:  
      ModifyParticipantContributionCmd(ConversationManager* conversationManager, 
                                       ConversationManager::ConversationHandle convHandle,
                                       ConversationManager::ParticipantHandle partHandle,
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
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " ModifyParticipantContributionCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ConversationHandle mConvHandle;
      ConversationManager::ParticipantHandle mPartHandle;
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
         mConversationManager->getBridgeMixer().outputBridgeMixWeights();
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " OutputBridgeMixWeightsCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
};

class AlertParticipantCmd  : public resip::DumCommand
{
   public:  
      AlertParticipantCmd(ConversationManager* conversationManager, 
                          ConversationManager::ParticipantHandle partHandle,
                          bool earlyFlag) 
         : mConversationManager(conversationManager),
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
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " AlertParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      bool mEarlyFlag;
};

class AnswerParticipantCmd  : public resip::DumCommand
{
   public:  
      AnswerParticipantCmd(ConversationManager* conversationManager, 
                          ConversationManager::ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->accept();
         }
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " AnswerParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
};

class RejectParticipantCmd  : public resip::DumCommand
{
   public:  
      RejectParticipantCmd(ConversationManager* conversationManager, 
                           ConversationManager::ParticipantHandle partHandle,
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
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " RejectParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      unsigned int mRejectCode;
};

class RedirectParticipantCmd  : public resip::DumCommand
{
   public:  
      RedirectParticipantCmd(ConversationManager* conversationManager, 
                             ConversationManager::ParticipantHandle partHandle,
                             resip::NameAddr& destination) 
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
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " RedirectParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      resip::NameAddr mDestination;
};

class RedirectToParticipantCmd  : public resip::DumCommand
{
   public:  
      RedirectToParticipantCmd(ConversationManager* conversationManager, 
                               ConversationManager::ParticipantHandle partHandle,
                               ConversationManager::ParticipantHandle destPartHandle) 
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
      }
      resip::Message* clone() const { assert(0); return 0; }
      std::ostream& encode(std::ostream& strm) const { strm << " RedirectToParticipantCmd: "; return strm; }
      std::ostream& encodeBrief(std::ostream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationManager::ParticipantHandle mPartHandle;
      ConversationManager::ParticipantHandle mDestPartHandle;
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
