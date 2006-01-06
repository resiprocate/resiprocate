#if !defined(RESIP_RESPONSE_CONTEXT_HXX)
#define RESIP_RESPONSE_CONTEXT_HXX 

#include <iosfwd>
#include <map>
#include "rutil/HashMap.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Via.hxx"
#include "resip/stack/Uri.hxx"

#include "repro/Target.hxx"

namespace resip
{
class SipMessage;
}

namespace repro
{

class RequestContext;

class ResponseContext
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


      //Returns true iff this target was successfully added.
      bool addTarget( repro::Target& target, bool beginImmediately=true);

      //Begins all Candidate client transactions. Returns true iff any transactions
      //were started as a result of this call
      bool beginClientTransactions();
      
      // Returns true iff the transaction was in the Candidate state before this call
      bool beginClientTransaction(const resip::Data& serial);
      
      //Returns true iff any transaction was placed in the WaitingToCancel state
      bool cancelActiveClientTransactions();
      
      bool cancelAllClientTransactions();
      
      //Returns true iff this transaction was placed in the WaitingToCancel state
      bool cancelClientTransaction(const resip::Data& serial);
            
      //Self-explanatory, except that NonExistent will be returned if the
      //transaction does not exist.
      Target::Status getStatus(const resip::Data& serial);

      //Keyed by transaction id
      typedef std::map<resip::Data,repro::Target> TransactionMap;


      //Used to decide which targets should be processed next,
      //usually by a response Processor.
      const TransactionMap& getPendingTransactionMap() const;
      
      
      bool hasPendingTransactions();
      bool hasActiveTransactions();
      bool hasTerminatedTransactions();
      
      const std::list<resip::Uri>& getTargetList() const;
      
      bool areAllTransactionsTerminated();
      // return true if the transaction was found
      int getPriority(const resip::SipMessage& msg);

      //!bwc! This should probably not be private, since these two classes are
      //tightly coupled.
      RequestContext& mRequestContext;
      

   private:
      // only constructed by RequestContext
      ResponseContext(RequestContext& parent);

      // These methods are really private. These are not intended to be used
      // by anything other than member functions of ResponseContext.

      // call this from RequestContext when a CANCEL comes in 
      void processCancel(const resip::SipMessage& request);

      // call this from RequestContext after the lemur chain for any response 
      void processResponse(resip::SipMessage& response);

      void processTimerC();

      void beginClientTransaction(repro::Target& target);
      void cancelClientTransaction(repro::Target& target);


      void terminateClientTransaction(const resip::Data& tid);
      void removeClientTransaction(const resip::Data& transactionId); 
      
      //There is no terminateClientTransaction(Target target) since terminating
      //a branch is very simple. The guts can be found in the API functions.
      
      void sendRequest(const resip::SipMessage& request);
      
      TransactionMap mPendingTransactionMap; //Targets with status Pending.
      TransactionMap mActiveTransactionMap; //Targets with status Trying, Proceeding, or WaitingToCancel.
      TransactionMap mTerminatedTransactionMap; //Targets with status Terminated.

      //Maybe someday canonicalized Uris will go here, and checking for duplicates
      //will be much faster
      std::list<resip::Uri> mTargetList;
      
      resip::SipMessage mBestResponse;
      bool mForwardedFinalResponse;
      int mBestPriority;
      bool mSecure;

      friend class RequestContext;
      friend std::ostream& operator<<(std::ostream& strm, const repro::ResponseContext& rc);
};

std::ostream&
operator<<(std::ostream& strm, const repro::ResponseContext& rc);

std::ostream& 
operator<<(std::ostream& strm, const repro::Target& t);

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
