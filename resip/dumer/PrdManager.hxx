#if !defined(DumPrdManager_hxx)
#define DumPrdManager_hxx

//#include <vector>
//#include <set>
//#include <map>

// #include "resip/stack/Headers.hxx"
// #include "resip/dum/EventDispatcher.hxx"
// #include "resip/dum/DialogSet.hxx"
// #include "resip/dum/DumTimeout.hxx"
// #include "resip/dum/HandleManager.hxx"
// #include "resip/dum/Handles.hxx"
// #include "resip/dum/MergedRequestKey.hxx"
// #include "resip/dum/RegistrationPersistenceManager.hxx"
// #include "resip/dum/EncryptionManager.hxx"
// #include "resip/dum/ServerSubscription.hxx"
// #include "resip/stack/SipStack.hxx"
// #include "resip/dum/DumFeature.hxx"
// #include "resip/dum/DumFeatureChain.hxx"
// #include "resip/dum/DumFeatureMessage.hxx"
// #include "resip/dum/TargetCommand.hxx"
// #include "resip/dum/ClientSubscriptionFunctor.hxx"
// #include "resip/dum/ServerSubscriptionFunctor.hxx"

#include <memory>
#include "rutil/BaseException.hxx"
#include "rutil/SharedPtr.hxx"
#include "resip/stack/TransactionUser.hxx"

namespace resip 
{

class Data;
class SipStack;

class PrdManager : public TransactionUser
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      int line)
               : BaseException(msg, file, line)
            {}
            
            virtual const char* name() const {return "DialogUsageManager::Exception";}
      };


      PrdManager(SipStack& stack, bool createDefaultFeatures=false);
      virtual ~PrdManager();

      void shutdown(DumShutdownHandler*, unsigned long giveUpSeconds=0);
      virtual void onDumCanBeDeleted()=0;

      // ?jf? Is this really changeable or should it be passed into the constructor
      void setMasterProfile(const SharedPtr<MasterProfile>& masterProfile);
      SharedPtr<MasterProfile> getMasterProfile();
      SharedPtr<UserProfile> getMasterUserProfile();

      SipStack& getSipStack();

      /** Give a prd implementation to the manager. The returned value
          is the input wrapped in a shared pointer. */
      SharedPtr<T> manage(std::auto_ptr<T> prd);

      /// Note:  Implementations of Postable must delete the message passed via post
      // !jf! this should always be done in the constructor/destructor of PrdManager
      //void registerForConnectionTermination(Postable*);
      //void unRegisterForConnectionTermination(Postable*);

      // !jf! removed the MessageInterceptor
      // ?jf? keepalive manager 
      // ?jf? redirect manager as feature
      // ?jf? client auth manager as feature
      // ?jf? server auth manager as feature

      // !jf! RegistrationPersistenceManager  will be created by the app and
      // passed to any Prd that needs it. e.g. ServerRegistration

      // applyToServerSubscriptions needs to be generalized or not included in
      // the new api

      // give dum an opportunity to handle its events. If process() returns true
      // there are more events to process. 
      bool process();

      //exposed so DumThread variants can be written
      Message* getNext(int ms) { return mFifo.getNext(ms); }
      void internalProcess(std::auto_ptr<Message> msg);
      bool messageAvailable(void) { return mFifo.messageAvailable(); }

      // makes a proto response to a request
      static void makeResponse(SipMessage& response, 
                               const SipMessage& request, 
                               int responseCode, 
                               const Data& reason = Data::Empty) const;


      void addTimer(DumTimeout::Type type,
                    unsigned long durationSeconds,
                    BaseUsageHandle target, 
                    int seq, 
                    int altseq=-1);
      
      void addTimerMs(DumTimeout::Type type,
                      unsigned long duration,
                      BaseUsageHandle target, 
                      int seq, 
                      int altseq=-1);

      // None of these methods should be used by anybody except
      // PrdCommands. Required to avoid being repeatedly pummelled in the
      // kidneys. 
      void internalManage(SharedPtr<Prd> prd);
      void internalUnmanage(SharedPtr<Prd> prd);
      void internalSend(std::auto_ptr<SipMessage> msg);
      
   protected:
      virtual void onAllHandlesDestroyed();      

      //TransactionUser virtuals
      virtual const Data& name() const;
      friend class DumThread;


   private:
      
      // ?jf? do I need this? May call a callback to let the app adorn
      void sendResponse(const SipMessage& response);

      // !jf! sending to outbound proxy becomes a feature

   private:
      SipStack& mStack;
      typedef enum 
      {
         Running,
         ShutdownRequested, // while ending usages
         RemovingTransactionUser, // while removing TU from stack
         Shutdown,  // after TU has been removed from stack
         Destroying // while calling destructor
      } ShutdownState;
      ShutdownState mShutdownState;

      SharedPtr<MasterProfile> mMasterProfile;
      //bool mIsDefaultServerReferHandler;

      typedef std::set<MergedRequestKey> MergedRequests;
      MergedRequests mMergedRequests;
            
      typedef std::map<Data, DialogSet*> CancelMap;
      CancelMap mCancelMap;

      typedef std::map<PrdId, Prd*> PrdMap;
      PrdMap mPrdMap;
      
      //std::auto_ptr<RedirectManager>   mRedirectManager;
      //std::auto_ptr<ClientAuthManager> mClientAuthManager;
      //std::auto_ptr<ServerAuthManager> mServerAuthManager;  

      ServerPublicationManager mServerPublicationManager;
      ServerRegistrationManager mServerRegistrationManager;
      ServerSubscriptionManager mServerSubscriptionManager;

      EventDispatcher<ConnectionTerminated> mConnectionTerminatedEventDispatcher;
};

}
   

class DialogUsageManager : public HandleManager, public TransactionUser
{
   private:     
      void processRequest(const SipMessage& request);
      void processResponse(const SipMessage& response);
      bool validateRequestURI(const SipMessage& request);
      bool validateRequiredOptions(const SipMessage& request);
      bool validateContent(const SipMessage& request);
      bool validateAccept(const SipMessage& request);
      bool validateTo(const SipMessage& request);
      bool mergeRequest(const SipMessage& request);
      void processPublish(const SipMessage& publish);
      void removeDialogSet(const DialogSetId& );      
      bool checkEventPackage(const SipMessage& request);
      bool queueForIdentityCheck(SipMessage* msg);
      void processIdentityCheckResponse(const HttpGetMessage& msg);
      void incomingProcess(std::auto_ptr<Message> msg);
      void outgoingProcess(std::auto_ptr<Message> msg);
      void requestMergedRequestRemoval(const MergedRequestKey&);
      void removeMergedRequest(const MergedRequestKey&);
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
