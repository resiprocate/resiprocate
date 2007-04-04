#if !defined(RESIP_RESPONSE_CONTEXT_HXX)
#define RESIP_RESPONSE_CONTEXT_HXX 

#include <iosfwd>
#include <map>
#include <list>

#include "rutil/HashMap.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Via.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/MessageDecorator.hxx"

#include "repro/Target.hxx"

namespace resip
{
class SipMessage;
}

namespace repro
{

class RequestContext;

class ResponseContext : public resip::MessageDecorator
{
   public:
      class CompareStatus  : public std::binary_function<const resip::SipMessage&, const resip::SipMessage&, bool>  
      {
         public:
            bool operator()(const resip::SipMessage& lhs, const resip::SipMessage& rhs) const;
      };      
      
      class CompareQ  : public std::binary_function<const resip::NameAddr&, const resip::NameAddr&, bool>
      {
         public:
            bool operator()(const resip::NameAddr& lhs, const resip::NameAddr& rhs) const;
      };      

      ~ResponseContext();

      /**
         Adds this Target to the collection of Targets.
         
         @param target The Target to add.

         @param beginImmediately Whether to immediately start a transaction for this target.

         @returns If beginImmediately=false, true iff the Target was
         successfully added (could happen if a final response has already
         been forwarded). If beginImmediately=true, true iff a transaction
         was successfully started for the Target (could fail due to the 
         presence of a duplicate contact, or when a final response has
         already been forwarded.)
         
         @note Targets are not checked for duplicate uris until an attempt is made to begin them.
      */
      bool addTarget( repro::Target& target, bool beginImmediately=true);
      
      /**
         Adds a batch of Targets. 
         
         @note RESPONSECONTEXT ASSUMES OWNERSHIP OF THE TARGETS POINTED
             TO IN THIS LIST!
         
         @param targets A list of (sorted) Target*. This list is consumed.
            
         @param highPriority Whether or not the Target ProccessorChain should 
            prioritize this batch above other batches of the same type.
            (This is primarily intended to let multiple recursive redirection
            work properly, but can be used for other purposes.)
         
         @returns true iff any Targets were added.
         
         @note It is assumed that all of these Targets are 
         the same subclass of Target, and that they are already sorted in the
         order of their priority. If these assumptions do not hold, things
         will not break per se, but oddball target processing behavior might
         result.
      */
      bool addTargetBatch(std::list<Target*>& targets,
                           bool highPriority=false);
      
      bool addOutboundBatch(std::map<resip::Data, std::list<Target*> > batch);
      
      /**
         Begins all Candidate client transactions.
         
         @returns true iff any transactions were started
      */
      bool beginClientTransactions();
      
      /** 
         Begins a client transaction.
      
         @param serial The transaction id to start.
         
         @returns true iff the transaction was started (could fail if there was
         a duplicate contact)
      
      */
      bool beginClientTransaction(const resip::Data& serial);
      
      /**
         Cancels all active client transactions. Does not clear Candidate
         transactions.
         
         @returns true iff any transaction was placed in either the
         WaitingToCancel or Cancelled state.
      */ 
      bool cancelActiveClientTransactions();
      
      /**
         Cancels all active client transactions. Also clears Candidate
         transactions (they are transitioned directly to Terminated)
         
         @returns true iff any transaction was placed in either the
         WaitingToCancel, Cancelled, or Terminated state.
      */ 
      bool cancelAllClientTransactions();
      
      /**
         Removes all Candidate transactions.
         
         @returns true iff at least one Candidate transaction was removed.
      */
      bool clearCandidateTransactions();
      
      /**
         Cancels this client transaction if active, or Terminates it if
         Candidate.
         
         @returns true iff this transaction was placed in either the 
         WaitingToCancel, Cancelled, or Terminated state.
      */
      bool cancelClientTransaction(const resip::Data& serial);
            
      /**
         Self-explanatory
      */
      Target* getTarget(const resip::Data& serial) const;

      //Keyed by transaction id
      typedef std::map<resip::Data,repro::Target*> TransactionMap;

      /**
         Self-explanatory.

         @note Can be used to decide which targets should be processed next,
         although this assumes a great deal of interdependency btw
         the various processor chains and homogeneity in how the Targets are
         prioritized, and can be inefficient if there
         are large numbers of Candidate Targets. A more structured approach
         exists in the functions dealing with TransactionBatch.
      */
      const TransactionMap& getCandidateTransactionMap() const;
      
      /**
         @returns true iff there are Candidate targets
      */
      bool hasCandidateTransactions() const;
      
      /**
         @returns true iff there are active targets (in state Trying,
         Proceeding, WaitingForCancel or Cancelled)
      */
      bool hasActiveTransactions() const;
      
      /**
         @returns true iff there are Terminated targets.
      */
      bool hasTerminatedTransactions() const;

      bool hasTargets() const;
      
      int targetCount() const;
      
      /**
         @returns true iff this target is in state Candidate
      */
      bool isCandidate(const resip::Data& tid) const;
      
      /**
         @returns true iff this target is active (Trying, Proceeding,
         WaitingForCancel, or Cancelled)
      */
      bool isActive(const resip::Data& tid) const;
      
      /**
         @returns true iff this target is in state Terminated
      */
      bool isTerminated(const resip::Data& tid) const;
      
      /**
         @returns true iff all targets are in state Terminated
      */
      bool areAllTransactionsTerminated() const;

      
      int getPriority(const resip::SipMessage& msg);

      //!bwc! This should probably not be private, since these two classes are
      //tightly coupled.
      RequestContext& mRequestContext;
      
//      typedef std::list<resip::Data> TransactionQueue;

      std::list<std::list<resip::Data> > mTransactionQueueCollection;

      // !bwc! Map from instance-id to lists of tid
      // (Each elem in list has identical instance id)
      typedef std::map<resip::Data, std::list<resip::Data> > OutboundMap;

      OutboundMap mOutboundMap;
      resip::Data mCurrentResponseTid;      
   private:
      // only constructed by RequestContext
      ResponseContext(RequestContext& parent);

      // These methods are really private. These are not intended to be used
      // by anything other than RequestContext or ResponseContext.

      // call this from RequestContext when a CANCEL comes in 
      void processCancel(const resip::SipMessage& request);

      // call this from RequestContext after the lemur chain for any response 
      void processResponse(resip::SipMessage& response);

      void processTimerC();

      void beginClientTransaction(repro::Target* target);
      void cancelClientTransaction(repro::Target* target);


      void terminateClientTransaction(const resip::Data& tid);
      void removeClientTransaction(const resip::Data& transactionId); 
      
      //There is no terminateClientTransaction(Target target) since terminating
      //a branch is very simple. The guts can be found in the API functions.
      
      void sendRequest(const resip::SipMessage& request);
      
      TransactionMap mCandidateTransactionMap; //Targets with status Candidate.
      TransactionMap mActiveTransactionMap; //Targets with status Trying, Proceeding, or WaitingToCancel.
      TransactionMap mTerminatedTransactionMap; //Targets with status Terminated.

      
      //Maybe someday canonicalized Uris will go here, and checking for duplicates
      //will be much faster
      resip::ContactList mTargetList;
      
      bool isDuplicate(const repro::Target* target) const;
      
      resip::SipMessage mBestResponse;
      int mBestPriority;
      bool mSecure;

      void forwardBestResponse();

      virtual void decorateMessage(resip::SipMessage &msg,
                                   const resip::Tuple &source, 
                                   const resip::Tuple &destination);

      void massageRoute(resip::NameAddr& rt);
      
      friend class RequestContext;
      friend std::ostream& operator<<(std::ostream& strm, const repro::ResponseContext& rc);
};

std::ostream&
operator<<(std::ostream& strm, const repro::ResponseContext& rc);

std::ostream& 
operator<<(std::ostream& strm, const repro::Target* t);

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
