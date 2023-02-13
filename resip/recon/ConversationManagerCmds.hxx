#if !defined(ConversationManagerCmds_hxx)
#define ConversationManagerCmds_hxx

#include <resip/dum/DumCommand.hxx>
#include <rutil/Logger.hxx>

#include "ConversationManager.hxx"
#include "Conversation.hxx"
#include "RemoteParticipant.hxx"
#include "RemoteIMPagerParticipant.hxx"
#include "LocalParticipant.hxx"
#include "MediaResourceParticipant.hxx"
#include "MediaStackAdapter.hxx"
#ifdef USE_KURENTO
#include "KurentoRemoteParticipant.hxx"
#endif
#ifdef USE_GSTREAMER
#include "GstRemoteParticipant.hxx"
#endif
#ifdef USE_LIBWEBRTC
#include "LibWebRTCRemoteParticipant.hxx"
#endif

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
class CreateConversationCmd  : public resip::DumCommandAdapter
{
   public:  
      CreateConversationCmd(ConversationManager* conversationManager, 
                            ConversationHandle convHandle,
                            ConversationManager::AutoHoldMode autoHoldMode,
                            ConversationHandle sharedMediaInterfaceConvHandle)
         : mConversationManager(conversationManager),
           mConvHandle(convHandle),
           mAutoHoldMode(autoHoldMode),
           mSharedMediaInterfaceConvHandle(sharedMediaInterfaceConvHandle){}
      virtual void executeCommand()
      {
            Conversation* conversation = mConversationManager->getMediaStackAdapter().createConversationInstance(mConvHandle, 0, mSharedMediaInterfaceConvHandle, mAutoHoldMode);
            resip_assert(conversation);
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateConversationCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ConversationManager::AutoHoldMode mAutoHoldMode;
      ConversationHandle mSharedMediaInterfaceConvHandle;
};

class DestroyConversationCmd  : public resip::DumCommandAdapter
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
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " DestroyConversationCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
};

class JoinConversationCmd  : public resip::DumCommandAdapter
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

            // If either conversation has a Remote or Media participant, then make sure both conversations are using the same
            // media interface
            if (sourceConversation->getNumRemoteParticipants() != 0 || sourceConversation->getNumMediaParticipants() != 0 ||
                destConversation->getNumRemoteParticipants() != 0 || destConversation->getNumMediaParticipants() != 0)
            {
               // Safety check when running in sipXConversationMediaInterfaceMode - ensure both conversation are using the same
               // media interface
               if (!mConversationManager->getMediaStackAdapter().canConversationsShareParticipants(sourceConversation, destConversation))
               {
                  WarningLog(<< "JoinConversationCmd: not supported for sourceConv=" << mSourceConvHandle << ", and destConv=" << mDestConvHandle);
                  return;
               }
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
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " JoinConversationCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
};

class CreateRemoteParticipantCmd  : public resip::DumCommandAdapter
{
   public:  
      CreateRemoteParticipantCmd(ConversationManager* conversationManager, 
                                 ParticipantHandle partHandle,
                                 ConversationHandle convHandle,
                                 const resip::NameAddr& destination,
                                 ConversationManager::ParticipantForkSelectMode forkSelectMode,
                                 std::shared_ptr<ConversationProfile> callerProfile = nullptr,
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
            RemoteParticipantDialogSet* participantDialogSet = mConversationManager->getMediaStackAdapter().createRemoteParticipantDialogSetInstance(mForkSelectMode, mCallerProfile);
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
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateRemoteParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::NameAddr mDestination;
      ConversationManager::ParticipantForkSelectMode mForkSelectMode;
      std::shared_ptr<ConversationProfile> mCallerProfile;
      std::multimap<resip::Data,resip::Data> mExtraHeaders;
};

class CreateRemoteIMPagerParticipantCmd : public resip::DumCommandAdapter
{
public:
   CreateRemoteIMPagerParticipantCmd(ConversationManager* conversationManager,
      ParticipantHandle partHandle,
      const resip::NameAddr& destination,
      std::shared_ptr<ConversationProfile> conversationProfile = nullptr)
      : mConversationManager(conversationManager),
      mPartHandle(partHandle),
      mDestination(destination),
      mConversationProfile(conversationProfile) {}
   virtual void executeCommand()
   {
      new RemoteIMPagerParticipant(mPartHandle, *mConversationManager, mDestination, mConversationProfile);
   }
   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateRemoteIMPagerParticipantCmd: "; return strm; }
private:
   ConversationManager* mConversationManager;
   ParticipantHandle mPartHandle;
   resip::NameAddr mDestination;
   std::shared_ptr<ConversationProfile> mConversationProfile;
};

class CreateRemoteIMSessionParticipantCmd : public resip::DumCommandAdapter
{
public:
   CreateRemoteIMSessionParticipantCmd(ConversationManager* conversationManager,
      ParticipantHandle partHandle,
      const resip::NameAddr& destination,
      ConversationManager::ParticipantForkSelectMode forkSelectMode,
      std::shared_ptr<ConversationProfile> callerProfile = nullptr,
      const std::multimap<resip::Data, resip::Data>& extraHeaders = (std::multimap<resip::Data, resip::Data>()))
      : mConversationManager(conversationManager),
      mPartHandle(partHandle),
      mDestination(destination),
      mForkSelectMode(forkSelectMode),
      mCallerProfile(callerProfile),
      mExtraHeaders(extraHeaders) {}
   virtual void executeCommand()
   {
      RemoteParticipantDialogSet* participantDialogSet = mConversationManager->createRemoteIMSessionParticipantDialogSetInstance(mForkSelectMode, mCallerProfile);
      RemoteParticipant* participant = participantDialogSet->createUACOriginalRemoteParticipant(mPartHandle);
      if (participant)
      {
         participant->initiateRemoteCall(mDestination, mCallerProfile, mExtraHeaders);
      }
      else
      {
         WarningLog(<< "CreateRemoteIMSessionParticipantCmd: error creating UACOriginalRemoteParticipant.");
         mConversationManager->onParticipantDestroyed(mPartHandle);
      }
   }
   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateRemoteIMSessionParticipantCmd: "; return strm; }
private:
   ConversationManager* mConversationManager;
   ParticipantHandle mPartHandle;
   resip::NameAddr mDestination;
   ConversationManager::ParticipantForkSelectMode mForkSelectMode;
   std::shared_ptr<ConversationProfile> mCallerProfile;
   std::multimap<resip::Data, resip::Data> mExtraHeaders;
};

class CreateMediaResourceParticipantCmd : public resip::DumCommandAdapter
{
   public:  
      CreateMediaResourceParticipantCmd(ConversationManager* conversationManager, 
                                        ParticipantHandle partHandle,
                                        ConversationHandle convHandle,
                                        const resip::Uri& mediaUrl,
                                        const std::shared_ptr<resip::Data>& audioBuffer,
                                        void* recordingCircularBuffer)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mConvHandle(convHandle),
           mMediaUrl(mediaUrl),
           mAudioBuffer(audioBuffer),
           mRecordingCircularBuffer(recordingCircularBuffer) {}
      virtual void executeCommand()
      {
         Conversation* conversation = mConversationManager->getConversation(mConvHandle);
         if(conversation)
         {
            MediaResourceParticipant* mediaResourceParticipant = mConversationManager->getMediaStackAdapter().createMediaResourceParticipantInstance(mPartHandle, mMediaUrl, mAudioBuffer, mRecordingCircularBuffer);
            if(mediaResourceParticipant)
            {
               conversation->addParticipant(mediaResourceParticipant);
               mediaResourceParticipant->startResource();  // Note: Can call delete this (MediaResourceParticipant) on failures
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
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateMediaResourceParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mConvHandle;
      resip::Uri mMediaUrl;
      std::shared_ptr<resip::Data> mAudioBuffer;
      void* mRecordingCircularBuffer;
};

class CreateLocalParticipantCmd : public resip::DumCommandAdapter
{
   public:  
      CreateLocalParticipantCmd(ConversationManager* conversationManager, 
                                ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         mConversationManager->getMediaStackAdapter().createLocalParticipantInstance(mPartHandle);
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " CreateLocalParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class DestroyParticipantCmd : public resip::DumCommandAdapter
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
      virtual resip::Message* clone() const { return new DestroyParticipantCmd(*this); }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " DestroyParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class AddParticipantCmd : public resip::DumCommandAdapter
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

         if (participant && conversation)
         {
            // Participants using a media interface (ie: those with a connection port) can only belong to multiple conversations if they 
            // have the same media interface
            if (participant->getConversations().size() > 0 && participant->getConnectionPortOnBridge() != -1)
            {
               // All conversations they are currently in will have the same media interface, just check that first conversation's media interface
               // matches the new conversation we are trying to add to.
               if (!mConversationManager->getMediaStackAdapter().canConversationsShareParticipants(participant->getConversations().begin()->second, conversation))
               {
                  WarningLog(<< "AddParticipantCmd: participants cannot belong to multiple conversations that don't share a media interface in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            conversation->addParticipant(participant);
         }
         else
         {
            if(!participant)
            {
               WarningLog(<< "AddParticipantCmd: invalid participant handle: " << mPartHandle);
            }
            if(!conversation)
            {
               WarningLog(<< "AddParticipantCmd: invalid conversation handle: " << mConvHandle);
            }
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " AddParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class RemoveParticipantCmd : public resip::DumCommandAdapter
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
            // When using multiple media interfaces, only local participants and remote IM participants can exist outside of any conversations.  We need to
            // prevent removal of other participant types (ie: those with a connection port on the bridge) from all conversations in this mode.
            if (mConversationManager->getMediaStackAdapter().supportsMultipleMediaInterfaces() && participant->getNumConversations() == 1)
            {
               LocalParticipant* localPart = dynamic_cast<LocalParticipant*>(participant);
               if (!localPart && participant->getConnectionPortOnBridge() != -1)  // Note:  connection port on bridge check eliminates RemoteIMParticipants from falling into if
               {
                  WarningLog(<< "RemoveParticipantCmd: you cannot remove non-local participants from all conversations when in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            conversation->removeParticipant(participant);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
};

class MoveParticipantCmd : public resip::DumCommandAdapter
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
         if (participant && sourceConversation && destConversation)
         {
            if (sourceConversation == destConversation)
            {
               // No-Op
               return;
            }

            // If participant uses a media interface, then ensure it's safe to move
            if (participant->getConnectionPortOnBridge() != -1)
            {
               // Safety check, mostly for when running in sipXConversationMediaInterfaceMode 
               // - ensure both conversation are using the same media interface
               if (!mConversationManager->getMediaStackAdapter().canConversationsShareParticipants(sourceConversation, destConversation))
               {
                  WarningLog(<< "MoveParticipantCmd: failed, both conversations must be using the same media interface.");
                  return;
               }
            }

            // Add to new conversation and remove from old (add before remove, so that hold/unhold won't happen)
            destConversation->addParticipant(participant);
            sourceConversation->removeParticipant(participant);
         }
         else
         {
            if(!participant)
            {
               WarningLog(<< "MoveParticipantCmd: invalid participant handle: " << mPartHandle);
            }
            if(!sourceConversation)
            {
               WarningLog(<< "MoveParticipantCmd: invalid source conversation handle: " << mSourceConvHandle);
            }
            if(!destConversation)
            {
               WarningLog(<< "MoveParticipantCmd: invalid destination conversation handle: " << mDestConvHandle);
            }
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " RemoveParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ConversationHandle mSourceConvHandle;
      ConversationHandle mDestConvHandle;
};

class ModifyParticipantContributionCmd : public resip::DumCommandAdapter
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
               WarningLog(<< "ModifyParticipantContributionCmd: invalid participant handle: " << mPartHandle);
            }
            if(!conversation)
            {
               WarningLog(<< "ModifyParticipantContributionCmd: invalid conversation handle: " << mConvHandle);
            }
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " ModifyParticipantContributionCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConvHandle;
      ParticipantHandle mPartHandle;
      unsigned int mInputGain;
      unsigned int mOutputGain;
};

class OutputBridgeMixWeightsCmd : public resip::DumCommandAdapter
{
   public:  
      OutputBridgeMixWeightsCmd(ConversationManager* conversationManager, ConversationHandle convHandle) 
         : mConversationManager(conversationManager), mConversationHandle(convHandle) {}
      virtual void executeCommand()
      {
         mConversationManager->getMediaStackAdapter().outputBridgeMatrixImpl(mConversationHandle);
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " OutputBridgeMixWeightsCmd: conversationHandle=" << mConversationHandle; return strm; }
   private:
      ConversationManager* mConversationManager;
      ConversationHandle mConversationHandle;
};

class AlertParticipantCmd : public resip::DumCommandAdapter
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
            if(mConversationManager->getMediaStackAdapter().supportsMultipleMediaInterfaces() && mEarlyFlag)
            {
               // Need to ensure, that the remote paticipant is added to a conversation before doing an operation that requires
               // media (ie. EarlyMediaFlag set to true).
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "AlertParticipantCmd: remote participants must be added to a conversation before alert with early flag can be used when in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->alert(mEarlyFlag);
         }
         else
         {
            WarningLog(<< "AlertParticipantCmd: invalid remote participant handle: " << mPartHandle);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " AlertParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      bool mEarlyFlag;
};

class AnswerParticipantCmd : public resip::DumCommandAdapter
{
   public:  
      AnswerParticipantCmd(ConversationManager* conversationManager, 
                          ParticipantHandle partHandle) 
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual void executeCommand()
      {
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(participant);
         if(remoteParticipant)
         {
            if(mConversationManager->getMediaStackAdapter().supportsMultipleMediaInterfaces())
            {
               // Need to ensure, that the remote paticipant is added to a conversation before accepting the call
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "AnswerParticipantCmd: remote participant must be added to a conversation before calling answer in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->accept();
            return;
         }

         RemoteIMPagerParticipant* remoteIMPagerParticipant = dynamic_cast<RemoteIMPagerParticipant*>(participant);
         if (remoteIMPagerParticipant)
         {
            remoteIMPagerParticipant->accept();
            return;
         }

         WarningLog(<< "AnswerParticipantCmd: invalid remote participant handle: " << mPartHandle);
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " AnswerParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class RejectParticipantCmd : public resip::DumCommandAdapter
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
         Participant* participant = mConversationManager->getParticipant(mPartHandle);
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(participant);
         if(remoteParticipant)
         {
            remoteParticipant->reject(mRejectCode);
            return;
         }

         RemoteIMPagerParticipant* remoteIMPagerParticipant = dynamic_cast<RemoteIMPagerParticipant*>(participant);
         if (remoteIMPagerParticipant)
         {
            remoteIMPagerParticipant->reject(mRejectCode);
            return;
         }

         WarningLog(<< "RejectParticipantCmd: invalid remote participant handle: " << mPartHandle);
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " RejectParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      unsigned int mRejectCode;
};

class RedirectParticipantCmd : public resip::DumCommandAdapter
{
   public:  
      RedirectParticipantCmd(ConversationManager* conversationManager, 
                             ParticipantHandle partHandle,
                             const resip::NameAddr& destination,
                             unsigned int redirectCode,
                             ConversationManager::RedirectSuccessCondition successCondition)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mDestination(destination),
           mRedirectCode(redirectCode),
           mSuccessCondition(successCondition) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->redirect(mDestination, mRedirectCode, mSuccessCondition);
         }
         else
         {
            WarningLog(<< "RedirectParticipantCmd: invalid remote participant handle: " << mPartHandle);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " RedirectParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      resip::NameAddr mDestination;
      unsigned int mRedirectCode;
      ConversationManager::RedirectSuccessCondition mSuccessCondition;
};

class RedirectToParticipantCmd : public resip::DumCommandAdapter
{
   public:  
      RedirectToParticipantCmd(ConversationManager* conversationManager, 
                               ParticipantHandle partHandle,
                               ParticipantHandle destPartHandle,
                               ConversationManager::RedirectSuccessCondition successCondition)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle),
           mDestPartHandle(destPartHandle),
           mSuccessCondition(successCondition) {}
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         RemoteParticipant* destRemoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mDestPartHandle));
         if(remoteParticipant && destRemoteParticipant)
         {
            remoteParticipant->redirectToParticipant(destRemoteParticipant->getInviteSessionHandle(), mSuccessCondition);
         }
         else
         {
            if(!remoteParticipant)
            {
               WarningLog(<< "RedirectToParticipantCmd: invalid remote participant handle: " << mPartHandle);
            }
            if(!destRemoteParticipant)
            {
               WarningLog(<< "RedirectToParticipantCmd: invalid destination remote participant handle: " << mDestPartHandle);
            }
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " RedirectToParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      ParticipantHandle mDestPartHandle;
      ConversationManager::RedirectSuccessCondition mSuccessCondition;
};

class HoldParticipantCmd : public resip::DumCommandAdapter
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
            if(mConversationManager->getMediaStackAdapter().supportsMultipleMediaInterfaces() && mHold)
            {
               // Need to ensure, that the remote paticipant is added to a conversation before doing an opertation that requires
               // media (ie. hold set to true).
               if(remoteParticipant->getConversations().size() == 0)
               {
                  WarningLog(<< "HoldParticipantCmd: remote participants must be added to a conversation before hold can be used when in sipXConversationMediaInterfaceMode.");
                  return;
               }
            }
            remoteParticipant->setLocalHold(mHold);
         }
         else
         {
            WarningLog(<< "HoldParticipantCmd: invalid remote participant handle: " << mPartHandle);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " HoldParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
      bool mHold;
};

class SendIMToParticipantCmd : public resip::DumCommandAdapter
{
public:
   SendIMToParticipantCmd(ConversationManager* conversationManager,
                          ParticipantHandle partHandle,
                          std::unique_ptr<resip::Contents> contents)
      : mConversationManager(conversationManager),
        mPartHandle(partHandle),
        mContents(std::move(contents)) {}
   virtual void executeCommand()
   {
      Participant* participant = mConversationManager->getParticipant(mPartHandle);
      IMParticipantBase* imParticipant = dynamic_cast<IMParticipantBase*>(participant);
      if (imParticipant)
      {
         imParticipant->sendInstantMessage(std::move(mContents));
         return;
      }

      WarningLog(<< "SendIMToParticipantCmd: invalid remote participant handle: " << mPartHandle);
   }
   EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " SendIMToParticipantCmd: "; return strm; }
private:
   ConversationManager* mConversationManager;
   ParticipantHandle mPartHandle;
   std::unique_ptr<resip::Contents> mContents;
};

class RequestKeyframeCmd : public resip::DumCommandAdapter
{
   public:
      RequestKeyframeCmd(ConversationManager* conversationManager,
                          ParticipantHandle partHandle)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual Message* clone() const { return new RequestKeyframeCmd(*this); }
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->requestKeyframe();
         }
         else
         {
            WarningLog(<< "RequestKeyframeCmd: invalid remote participant handle: " << mPartHandle);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " HoldParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
};

class RequestKeyframeFromPeerCmd : public resip::DumCommandAdapter
{
   public:
      RequestKeyframeFromPeerCmd(ConversationManager* conversationManager,
                          ParticipantHandle partHandle)
         : mConversationManager(conversationManager),
           mPartHandle(partHandle) {}
      virtual Message* clone() const { return new RequestKeyframeFromPeerCmd(*this); }
      virtual void executeCommand()
      {
         RemoteParticipant* remoteParticipant = dynamic_cast<RemoteParticipant*>(mConversationManager->getParticipant(mPartHandle));
         if(remoteParticipant)
         {
            remoteParticipant->requestKeyframeFromPeer();
         }
         else
         {
            WarningLog(<< "RequestKeyframeFromPeerCmd: invalid remote participant handle: " << mPartHandle);
         }
      }
      EncodeStream& encodeBrief(EncodeStream& strm) const { strm << " HoldParticipantCmd: "; return strm; }
   private:
      ConversationManager* mConversationManager;
      ParticipantHandle mPartHandle;
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

#undef RESIPROCATE_SUBSYSTEM

#endif


/* ====================================================================

 Copyright (c) 2021-2023, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021-2022, Daniel Pocock https://danielpocock.com
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
