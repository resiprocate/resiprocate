#include "repro/stateAgents/PresenceServer.hxx"
#include <resip/stack/GenericPidfContents.hxx>
#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/MasterProfile.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace resip;
using namespace repro;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

PresenceServer::PresenceServer(DialogUsageManager& dum, Dispatcher* userDispatcher, bool presenceUsesRegistrationState, 
                               bool PresenceNotifyClosedStateForNonPublishedUsers) :
   mDum(dum), 
   mPresenceSubscriptionHandler(dum, userDispatcher, presenceUsesRegistrationState, PresenceNotifyClosedStateForNonPublishedUsers),
   mPresencePublicationHandler(dum)
{         
    MasterProfile& profile = *mDum.getMasterProfile();
    profile.addSupportedMethod(PUBLISH);
    profile.addSupportedMethod(SUBSCRIBE);
    //profile.validateAcceptEnabled() = true;  // !slg! this causes Accept validation for registration requests as well, which is not really desired
    profile.validateContentEnabled() = true;
    profile.addSupportedMimeType(PUBLISH, GenericPidfContents::getStaticType());
    profile.addSupportedMimeType(SUBSCRIBE, GenericPidfContents::getStaticType());

    mDum.addServerSubscriptionHandler(Symbols::Presence, &mPresenceSubscriptionHandler);
    mDum.addServerPublicationHandler(Symbols::Presence, &mPresencePublicationHandler);
}

PresenceServer::~PresenceServer()
{
}

/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
/*
* vi: set shiftwidth=3 expandtab:
*/
