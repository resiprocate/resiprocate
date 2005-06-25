#if !defined(RESIP_RESPONSE_CONTEXT_HXX)
#define RESIP_RESPONSE_CONTEXT_HXX 

#include <iosfwd>
#include "rutil/HashMap.hxx"
#include "resiprocate/NameAddr.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Via.hxx"
#include "resiprocate/Uri.hxx"

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
      typedef enum
      {
         Trying,
         Proceeding,
         WaitingToCancel,
         Terminated
      } Status;

      struct Branch
      {
            Status status;
            resip::Uri uri;
            resip::Via via; // top via
      };


   private:
      // only constructed by RequestContext
      ResponseContext(RequestContext& parent);

      // call this after the monkey chain runs on an event
      void processCandidates();

      // call this from RequestContext when a CANCEL comes in 
      void processCancel(const resip::SipMessage& request);

      // call this from RequestContext after the lemur chain for any response 
      void processResponse(resip::SipMessage& response);

   private:
      // These methods are really private
      void processPendingTargets();
      void sendRequest(const resip::SipMessage& request);
      void cancelClientTransaction(const Branch& branch);
      void terminateClientTransaction(const resip::Data& transactionId);
      void cancelProceedingClientTransactions();
      bool areAllTransactionsTerminated();
      // return true if the transaction was found
      bool removeClientTransaction(const resip::Data& transactionId); 
      int getPriority(const resip::SipMessage& msg);

      RequestContext& mRequestContext;
      
      typedef std::multiset<resip::NameAddr, CompareQ> PendingTargetSet;
      PendingTargetSet mPendingTargetSet;

      HashSet<resip::Uri> mTargetSet;
      
      typedef HashMap<resip::Data, Branch> TransactionMap;
      TransactionMap mClientTransactions;

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
operator<<(std::ostream& strm, const repro::ResponseContext::Branch& b);

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
