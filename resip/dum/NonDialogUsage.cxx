#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/NonDialogUsage.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"

#include <utility>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

NonDialogUsage::Exception::Exception(const Data& msg, const Data& file, const int line)
   : BaseException(msg, file, line)
{
}

const char*
NonDialogUsage::Exception::name() const noexcept
{
   return "NonDialogUsage::Exception";
}

NonDialogUsage::NonDialogUsage(DialogUsageManager& dum, DialogSet& dialogSet) :
   BaseUsage(dum),
   mDialogSet(dialogSet)
{
}

NonDialogUsage::~NonDialogUsage()
{
   mDialogSet.possiblyDie();
}

AppDialogSetHandle 
NonDialogUsage::getAppDialogSet() const
{
   return mDialogSet.mAppDialogSet->getHandle();
}

std::shared_ptr<UserProfile> 
NonDialogUsage::getUserProfile() const noexcept
{
   return mDialogSet.mUserProfile;
}

void
NonDialogUsage::applyLocalContact(SipMessage& response, const SipMessage& request) const
{
   // Only touch the single empty Contact created by Helper::makeResponse -
   // anything else was put there by the application.  An empty host is what
   // the TransportSelector fills in with the address of the outgoing transport.
   if (!response.exists(h_Contacts) ||
       response.header(h_Contacts).size() != 1 ||
       !response.header(h_Contacts).front().uri().host().empty())
   {
      return;
   }

   const std::shared_ptr<UserProfile> profile = getUserProfile();
   NameAddr& contact = response.header(h_Contacts).front();
   if (profile->hasUserAgentCapabilities())
   {
      contact = profile->getUserAgentCapabilities();
   }
   if (!profile->isAnonymous() && profile->hasPublicGruu())
   {
      contact.uri() = profile->getPublicGruu();
   }
   else if (profile->isAnonymous() && profile->hasTempGruu())
   {
      contact.uri() = profile->getTempGruu();
   }
   else
   {
      if (profile->hasOverrideHostAndPort())
      {
         contact.uri() = profile->getOverrideHostAndPort();
      }
      // See Dialog.cxx for why the user part is only copied from sip:, sips:
      // and tel: URIs.
      if (request.header(h_RequestLine).uri().user().empty())
      {
         if (request.header(h_To).uri().isUserRelevant())
         {
            contact.uri().user() = request.header(h_To).uri().user();
         }
      }
      else if (request.header(h_RequestLine).uri().isUserRelevant())
      {
         contact.uri().user() = request.header(h_RequestLine).uri().user();
      }
      const Data& instanceId = profile->getInstanceId();
      if (!contact.uri().exists(p_gr) && !instanceId.empty())
      {
         contact.param(p_Instance) = instanceId;
      }
   }
   if (profile->clientOutboundEnabled())
   {
      // Add ;ob parm to non-register requests - RFC5626 pg17
      contact.uri().param(p_ob);
   }
}

void 
NonDialogUsage::send(std::shared_ptr<SipMessage> msg)
{
   const NameAddrs& sRoute = getUserProfile()->getServiceRoute();

   // Clear the routes if there was previously a service_route. Otherwise
   // keep the route in case there exists custom routes

   if (!sRoute.empty())
   {
      if (msg->header(h_RequestLine).method() == REGISTER)
      {
         const NameAddrs emptyRoute;         
         msg->remove(h_Routes);
         getUserProfile()->setServiceRoute(emptyRoute);
      }
      else
      {
         InfoLog(<< "Applying service route: " << Inserter(getUserProfile()->getServiceRoute()) << " to " << msg->brief());   
         msg->header(h_Routes) = sRoute;
      }
   }
   
   mDum.send(msg);
}

class NonDialogUsageSendCommand : public DumCommandAdapter
{
public:
   NonDialogUsageSendCommand(NonDialogUsage& usage, std::shared_ptr<SipMessage> msg)
      : mNonDialogUsage(usage),
        mMessage(msg)
   {
   }

   void executeCommand() override
   {
      mNonDialogUsage.send(mMessage);
   }

   EncodeStream& encodeBrief(EncodeStream& strm) const override
   {
      return strm << "NonDialogUsageSendCommand";
   }
private:
   NonDialogUsage& mNonDialogUsage;
   std::shared_ptr<SipMessage> mMessage;
};

void
NonDialogUsage::sendCommand(std::shared_ptr<SipMessage> message)
{   
   mDum.post(new NonDialogUsageSendCommand(*this, message));
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. http://www.sipspectrum.com
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
