#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/dum/InviteSessionCreator.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/DumHelper.hxx"

using namespace resip;

InviteSessionCreator::InviteSessionCreator(DialogUsageManager& dum,
                                           const NameAddr& target,
                                           SharedPtr<UserProfile> userProfile,
                                           const Contents* initial,
                                           DialogUsageManager::EncryptionLevel level,
                                           const Contents* alternative,
                                           ServerSubscriptionHandle serverSub)
   : BaseCreator(dum, userProfile),
     mState(Initialized),
     mServerSub(serverSub),
     mEncryptionLevel(level)
{
   makeInitialRequest(target, INVITE);
   if (userProfile->isAnonymous())
   {
      mLastRequest->header(h_Privacys).push_back(PrivacyCategory(Symbols::id));
   }
   
   DumHelper::setOutgoingEncryptionLevel(*mLastRequest, level);
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
      resip_assert(userProfile.get());
      if(userProfile->getDefaultSessionTime() >= 90)
      {
         getLastRequest()->header(h_SessionExpires).value() = userProfile->getDefaultSessionTime();
         getLastRequest()->header(h_MinSE).value() = 90;  // Absolute minimum specified by RFC4028
      }
   }

   std::auto_ptr<Contents> initialOffer;
   if (initial)
   {
      if (alternative)
      {
         MultipartAlternativeContents* mac = new MultipartAlternativeContents;
         mac->parts().push_back(alternative->clone());
         mac->parts().push_back(initial->clone());
         initialOffer.reset(mac);
      }
      else
      {
         initialOffer.reset(initial->clone());
      }
      getLastRequest()->setContents(initialOffer);
   }
   //100rel 
   switch(mDum.getMasterProfile()->getUacReliableProvisionalMode())
   {
      case MasterProfile::Never:
         //no support, do nothing
         break;
      case MasterProfile::Supported:
      case MasterProfile::SupportedEssential:
         getLastRequest()->header(h_Supporteds).push_back(Token(Symbols::C100rel));
         break;
      case MasterProfile::Required:
         getLastRequest()->header(h_Requires).push_back(Token(Symbols::C100rel));
         break;
      default:
         resip_assert(0);
   }
}

InviteSessionCreator::~InviteSessionCreator()
{
}

void
InviteSessionCreator::end()
{
   resip_assert(0);
}

void
InviteSessionCreator::dispatch(const SipMessage& msg)
{
   // !jf! does this have to do with CANCELing all of the branches associated
   // with a single invite request
}

const Contents*
InviteSessionCreator::getInitialOffer()
{
   return getLastRequest()->getContents();
}


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
