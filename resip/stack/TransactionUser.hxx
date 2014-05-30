#if !defined(RESIP_TU_HXX)
#define RESIP_TU_HXX 

#include <iosfwd>
#include <set>
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Data.hxx"
#include "rutil/CongestionManager.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/MessageFilterRule.hxx"


namespace resip
{
class SipMessage;

/**
   @brief The base-class for an RFC 3261 Transaction User. This is the 
      "app-layer".

      Subclasses of TransactionUser are expected to do the following things:
         - Register itself by using SipStack::registerTransactionUser().
         - Regularly pull messages out of mFifo, and process them.
            - Particularly, every received SIP request MUST be responded to, or
               the stack will leak transactions.
         - Before the TransactionUser is destroyed, it should ensure that it has 
            been unregistered from the stack using 
            SipStack::unregisterTransactionUser(), and gotten a confirmation in 
            the form of a TransactionUserMessage.

      There is also a collection of things you can do to customize how the stack 
      interacts with your TransactionUser:
         - If you wish to restrict the types of SIP traffic your TransactionUser
            will receive from the stack, you can do so by setting the 
            MessageFilterRuleList, either in the constructor, or with 
            setMessageFilterRuleList() (see MessageFilterRule for more)
            - If you need more fine-grained control over what SIP messages your 
               TransactionUser is willing/able to handle, you can override 
               TransactionUser::isForMe().
         - If you wish your TransactionUser to be notified whenever a 
            transaction ends, just pass RegisterForTransactionTermination in the 
            constructor.
         - If you wish your TransactionUser to be notified when Connections are 
            closed, pass RegisterForConnectionTermination in the constructor.

   @ingroup resip_crit
*/
class TransactionUser
{
   public:
      /**
         @brief Posts a Message to this TransactionUser's fifo. Ownership of msg 
            is taken.
         @param msg The Message to add to mFifo. (This takes ownership of msg)
      */
      void post(Message* msg);

      /**
         @brief Returns true iff domain matches one of the domains that this 
            TransactionUser is responsible for. (added with addDomain).
         @param domain The domain name to check.
         @return True iff this TransactionUser is responsible for domain.
         @note The comparison performed is case-sensitive; make sure you 
            lower-case everything you put in here.
      */
      bool isMyDomain(const Data& domain) const;

      /**
         @brief Adds a domain to the set of domains that this TransactionUser is 
            responsible for.
         @note The comparison performed is case-sensitive; make sure you 
            lower-case everything you put in here.
         @todo Make this case-insensitive.
      */
      void addDomain(const Data& domain);

      /**
         @brief Return the name of this TransactionUser. Used in encode().
         @return The name of this TransactionUser, as a Data.
      */
      virtual const Data& name() const=0;

      /**
         @brief Encodes the name of this TransactionUser (as specified by 
            name()), and the current depth of its fifo.
         @param strm The ostream to encode to.
         @return strm
      */
      virtual EncodeStream& encode(EncodeStream& strm) const;

      /**
         @brief Sets this TransactionUser's MessageFilterRuleList.
         
         This tells the stack which SIP messages this TransactionUser is 
         interested in, and which ones it is not. This allows multiple 
         TransactionUsers to run on top of the same SipStack.
         
         @param rules The MessageFilterRuleList to use.
         @see MessageFilterRule
      */
      void setMessageFilterRuleList(MessageFilterRuleList &rules);

      /**
         @internal
         @brief Returns true iff this TransactionUser should be notified when 
            transactions end.
      */
      bool isRegisteredForTransactionTermination() const;

      /**
         @internal
         @brief Returns true iff this TransactionUser should be notified when 
            connections close.
      */
      bool isRegisteredForConnectionTermination() const;
      bool isRegisteredForKeepAlivePongs() const;

      inline CongestionManager::RejectionBehavior getRejectionBehavior() const
      {
         if(mCongestionManager)
         {
            return mCongestionManager->getRejectionBehavior(&mFifo);
         }
         return CongestionManager::NORMAL;
      }
      
      virtual void setCongestionManager(CongestionManager* manager)
      {
         if(mCongestionManager)
         {
            mCongestionManager->unregisterFifo(&mFifo);
         }
         mCongestionManager=manager;
         if(mCongestionManager)
         {
            mCongestionManager->registerFifo(&mFifo);
         }
      }

      const TimeLimitFifo<Message>* getFifo() { return(&mFifo); } const
      
      virtual UInt16 getExpectedWait() const
      {
         return (UInt16)mFifo.expectedWaitTimeMilliSec();
      }
      
      // .bwc. This specifies whether the TU can cope with dropped responses
      // (due to congestion). Some TUs may need responses to clean up state,
      // while others may rely on TransactionTerminated messages. Those that
      // rely on TransactionTerminated messages will be able to return false
      // here, meaning that in dire congestion situations, the stack will drop
      // responses bound for the TU.
      virtual bool responsesMandatory() const {return true;}
      
   protected:
      enum TransactionTermination 
      {
         RegisterForTransactionTermination,
         DoNotRegisterForTransactionTermination
      };

      enum ConnectionTermination 
      {
         RegisterForConnectionTermination,
         DoNotRegisterForConnectionTermination
      };

      enum KeepAlivePongs 
      {
         RegisterForKeepAlivePongs,
         DoNotRegisterForKeepAlivePongs
      };

      /**
         @brief Constructor that specifies whether this TransactionUser needs to 
            hear about the completion of transactions, and the closing of 
            connections.
         @param t Whether or not the TransactionUser should be informed when 
            transactions end (disabled by default).
         @param c Whether or not the TransactionUser should be informed when 
            connections close (disabled by default).
         @note The default MessageFilterRuleList used will accept all SIP 
            requests with either sip: or sips: in the Request-Uri.
         @note This is protected to ensure than no-one constructs the 
            base-class. (Subclasses call this in their constructor)
      */
      TransactionUser(TransactionTermination t=DoNotRegisterForTransactionTermination,
                      ConnectionTermination c=DoNotRegisterForConnectionTermination,
                      KeepAlivePongs k=DoNotRegisterForKeepAlivePongs);

      /**
         @brief Constructor that specifies the MessageFilterRuleList, whether 
            this TransactionUser needs to hear about the completion of 
            transactions, and the closing of connections.
         @param rules The MessageFilterRuleList to use. (A copy is made)
         @param t Whether or not the TransactionUser should be informed when 
            transactions end (disabled by default).
         @param c Whether or not the TransactionUser should be informed when 
            connections close (disabled by default).
         @note This is protected to ensure than no-one constructs the 
            base-class. (Subclasses call this in their constructor)
      */
      TransactionUser(MessageFilterRuleList &rules, 
                      TransactionTermination t=DoNotRegisterForTransactionTermination,
                      ConnectionTermination c=DoNotRegisterForConnectionTermination,
                      KeepAlivePongs k=DoNotRegisterForKeepAlivePongs);

      virtual ~TransactionUser()=0;

      /**
         @brief Returns true iff this TransactionUser should process msg.
         @param msg The SipMessage we received.
         @return True iff this TransactionUser should process msg.
         @note By default, this uses the MessageFilterRuleList. It can be 
            overridden for more flexibility.
      */
      virtual bool isForMe(const SipMessage& msg) const;

      /**
         @brief This TransactionUser's fifo. All communication with the 
            TransactionUser goes through here.
      */
      TimeLimitFifo<Message> mFifo;
      CongestionManager* mCongestionManager;

   private:      
      void postToTransactionUser(Message* msg, TimeLimitFifo<Message>::DepthUsage usage);
      unsigned int size() const;
      bool wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const;

   private:
      MessageFilterRuleList mRuleList;
      typedef std::set<Data> DomainList;
      DomainList mDomainList;
      bool mRegisteredForTransactionTermination;
      bool mRegisteredForConnectionTermination;
      bool mRegisteredForKeepAlivePongs;
      friend class TuSelector;      
};

EncodeStream& 
operator<<(EncodeStream& strm, const TransactionUser& tu);

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
