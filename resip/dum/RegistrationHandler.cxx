#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

void 
ClientRegistrationHandler::onFlowTerminated(ClientRegistrationHandle h)
{
   InfoLog (<< "ClientRegistrationHandler::onFlowTerminated, refreshing registration to open new flow");
   h->requestRefresh();
}

void
ServerRegistrationHandler::getGlobalExpires(const SipMessage& msg, SharedPtr<MasterProfile> masterProfile,
      UInt32 &expires, UInt32 &returnCode)
{
   if (!masterProfile)
   {
      returnCode = 500;
      resip_assert(0);
      return;
   }

   expires=3600;
   returnCode=0;

   if (!msg.empty(h_Expires) && msg.header(h_Expires).isWellFormed())
   {
      //only client specified Expires value is subject to the min/max constraints, default is used if none specified.
      expires = msg.header(h_Expires).value();

      if (expires != 0)
      {
         //check min expires first since max expires will not return an error and will just change the expires value.
         UInt32 minExpires = masterProfile->serverRegistrationMinExpiresTime();

         if (expires < minExpires)
         {
            returnCode = 423;
            expires = minExpires;
         }
         else
         {
            UInt32 maxExpires = masterProfile->serverRegistrationMaxExpiresTime();

            if (expires > maxExpires)
            {
               expires = maxExpires;
            }
         }
      }
   }
   else
   {
      expires = masterProfile->serverRegistrationDefaultExpiresTime();
   }
}

void
ServerRegistrationHandler::getContactExpires(const NameAddr &contact, SharedPtr<MasterProfile> masterProfile,
      UInt32 &expires, UInt32 &returnCode)
{
   if (!masterProfile)
   {
      returnCode = 500;
      resip_assert(0);
      return;
   }
   
   returnCode=0;

   if (contact.exists(p_expires))
   {
      expires = contact.param(p_expires);

      if (expires != 0)
      {
         //check min expires first since max expires will not return an error and will just change the expires value.
         UInt32 minExpires = masterProfile->serverRegistrationMinExpiresTime();

         if (expires < minExpires)
         {
            returnCode = 423;
            expires = minExpires;
         }
         else
         {
            UInt32 maxExpires = masterProfile->serverRegistrationMaxExpiresTime();

            if (expires > maxExpires)
            {
               expires = maxExpires;
            }
         }
      }
   }
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
