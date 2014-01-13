#if !defined(RESIP_TLSPEERAUTHMANAGER_HXX)
#define RESIP_TLSPEERAUTHMANAGER_HXX

#include <map>
#include <set>

#include "resip/stack/SipMessage.hxx"
#include "DumFeature.hxx"

namespace resip
{
class DialogUsageManager;

typedef std::set<Data> PermittedFromAddresses;
typedef std::map<Data, PermittedFromAddresses> CommonNameMappings;

class TlsPeerAuthManager : public DumFeature
{
   public:
      enum Result
      {
         Authorized,
         Skipped,
         Rejected
      };

      TlsPeerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, const std::set<Data>& trustedPeers, bool thirdPartyRequiresCertificate = true);
      TlsPeerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, const std::set<Data>& trustedPeers, bool thirdPartyRequiresCertificate, CommonNameMappings& commonNameMappings);
      virtual ~TlsPeerAuthManager();

      virtual ProcessingResult process(Message* msg);      
      
   protected:

      // can return Authorized, Rejected, Skipped
      virtual Result handle(SipMessage* sipMsg);

      /// should return true if the passed in user is authorized for the provided uri
      virtual bool authorizedForThisIdentity(const std::list<resip::Data> &peerNames, 
                                             resip::Uri &fromUri);

      /// should return true if the request must be challenged
      /// The default is to challenge all requests - override this class to change this beviour
      virtual bool requiresAuthorization(const SipMessage& msg);
      /// should return true if the request should be trusted based on
      /// the source/transport
      /// default implementation uses mTrustedPeers
      virtual bool isTrustedSource(const SipMessage& msg);

   private:
      std::set<Data> mTrustedPeers;
      bool mThirdPartyRequiresCertificate;
      CommonNameMappings mCommonNameMappings;
};

 
}

#endif

/* ====================================================================
 * BSD License
 * 
 * Copyright (c) 2012 Daniel Pocock  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * ====================================================================
 * 
 */
