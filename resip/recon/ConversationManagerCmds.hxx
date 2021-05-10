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
                            ConversationManager::AutoHoldMode autoHoldMode,
                            ConversationHandle sharedFlowConvHandle) 
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mAutoHoldMode(autoHoldMode),
           mSharedFlowConvHandle(sharedFlowConvHandle){}
      virtual void executeCommand()
      {
            Conversation* conversation = new Conversation(mConvHandle, *mConversationManager, 0, mSharedFlowConvHandle, mAutoHoldMode);
            resip_assert(conversation);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " CreateConversationCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ConversationManager::AutoHoldMode mAutoHoldMode;
      ConversationHandle mSharedFlowConvHandle;
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
         Conversation* sourceConversation = mConversationManager->getConversation(mSourceConvHandle);
         Conversation* destConversation = mConversationManager->getConversation(mDestConvHandle);
         if (sourceConversation && destConversation)
         {
            if (sourceConversation == destConversation)
            {
               // NoOp
               return;
            }

            // Safety check when running in sipXConversationMediaInterfaceMode - ensure both conversation are using the same
            // media interface
            if (!mConversationManager->supportsJoin(mSourceConvHandle, mDestConvHandle))
            {
               WarningLog(<< "JoinConversationCmd: not supported");
               return;
            }

            // Join source Conversation into dest Conversation and destroy source conversation
            sourceConversation->join(destConversation);
         }
         else
         {
            if (!sourceConversation)
            {
               WarningLog(<< "JoinConversationCmd: invalid source conversation handle.");
            }
            if (!destConversation)
            {
               WarningLog(<< "JoinConversationCmd: invalid destination conversation handle.");
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
                                 std::shared_ptr<resip::UserProfile> callerProfile = nullptr,
                                 const std::multimap<resip::Data,resip::Data>& extraHeaders = (std::multimap<resip::Data,resip::Data>()))
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mDestination(destination),
           mForkSelectMode(forkSelectMode),
           mCallerProfile(std::move(callerProfile)),
           mExtraHeaders(extraHeaders) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation)
         {
            const auto _callerProfile = std::dynamic_pointer_cast<ConversationProfile>(mCallerProfile);
            RemoteParticipantDialogSet* participantDialogSet = mConversationManager->createRemoteParticipantDialogSetInstance(mForkSelectMode, _callerProfile);
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
      std::shared_ptr<resip::UserProfile> mCallerProfile;
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
            MediaResourceParticipant* mediaResourceParticipant = mConversationManager->createMediaResourceParticipantInstance(mPartHandle, mMediaUrl);
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
         mConversationManager->createLocalParticipantInstance(mPartHandle);
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
            if(mConversationManager->supportsMultipleConversations())
            {
               // Participants can only belong to multiple conversations if they have the same media interface
               if(participant->getConversations().size() > 0)
               {
                  // All conversations they are currently in will have the same media interface, just check that first conversation's media interface
                  // matches the new conversation we are trying to add to.
                  ConversationHandle existingConversationHandle = participant->getConversations().begin()->second->getHandle();
                  if (!mConversationManager->supportsJoin(existingConversationHandle, mConvHandle))
                  {
                     WarningLog(<< "AddParticipantCmd: participants cannot belong to multiple conversations that don't share a media interface in sipXConversationMediaInterfaceMode.");
                     return;
                  }
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

            // Safety check when running in sipXConversationMediaInterfaceMode - ensure both conversation are using the same
            // media interface
            if (!mConversationManager->supportsJoin(mSourceConvHandle, mDestConvHandle))
            {
               WarningLog(<< "MoveParticipantCmd: failed, both conversations must be using the same media interface.");
               return;
            }

            // Add to new conversation and remove from old (add before remove, so that hold/unhold won't happen)
            destConversation->addParticipant(participant);
            sourceConversation->removeParticipant(participant);
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
      OutputBridgeMixWeightsCmd(ConversationManager* conversationManager, ConversationHandle convHandle) 
         : mConversationManager(conversationManager), mConversationHandle(convHandle) {}
      virtual void executeCommand()
      {
         mConversationManager->outputBridgeMatrixImpl(mConversationHandle);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " OutputBridgeMixWeightsCmd: conversationHandle=" << mConversationHandle; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConversationHandle;
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
            if(mConversationManager->supportsMultipleConversations() && mEarlyFlag)
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
            if(mConversationManager->supportsMultipleConversations())
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

class HoldParticipantCmd  : public resip::DumCommand
{
   public:
      HoldParticipantCmd(ConversationManager* conversationManager,
                          ParticipantHandle partHandle,
                          bool hold)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mHold(hold) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            if(mConversationManager->supportsMultipleConversations() && mHold)
            {
               // Need to ensure, that the remote paticipant is added to a conversation before doing an opertation that requires
               // media (ie. hold set to true).
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "HoldParticipantCmd: remote participants must to added to a conversation before hold can be used when in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->setLocalHold(mHold);
         }
         else
         {
            WarningLog(<< "HoldParticipantCmd: invalid remote participant handle.");
         }
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " HoldParticipantCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      bool mHold;
};

class ApplicationTimerCmd : public resip::DumCommand
{
public:
   ApplicationTimerCmd(ConversationManager* conversationManager,
      unsigned int timerId, unsigned int timerData1, unsigned int timerData2)
      : mConversationManager(conversationManager),
      mTimerId(timerId),
      mTimerData1(timerData1),
      mTimerData2(timerData2) {}
   virtual void executeCommand()
   {
      mConversationManager->onApplicationTimer(mTimerId, mTimerData1, mTimerData2);
   }
   resip::Message* clone() const { return new ApplicationTimerCmd(mConversationManager, mTimerId, mTimerData1, mTimerData2); }
   EncodeStream& encode(EncodeStream& strm) const { strm << " ApplicationTimerCmd: timerId=" << mTimerId << ", data1=" << mTimerData1 << ", data2=" << mTimerData2; return strm; }
   EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
private:
   ConversationManager* mConversationManager;
   unsigned int mTimerId;
   unsigned int mTimerData1;
   unsigned int mTimerData2;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc.
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
