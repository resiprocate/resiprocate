#include "ReconSubsystem.hxx"
#include "InstantMessage.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include <resip/dum/ClientPagerMessage.hxx>
#include <resip/dum/ServerPagerMessage.hxx>
#include <resip/stack/PlainContents.hxx>

#include "ReconSubsystem.hxx"
#include "UserAgent.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

InstantMessage::InstantMessage()
{

}

InstantMessage::~InstantMessage()
{

}

void
InstantMessage::onMessageArrived(resip::ServerPagerMessageHandle handle, const resip::SipMessage& message)
{
   // Default implementation is to do nothing - application should override this
}

void
InstantMessage::onSuccess(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status)
{
   // Default implementation is to do nothing - application should override this
}

void
InstantMessage::onFailure(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents)
{
   // Default implementation is to do nothing - application should override this
}

/* ====================================================================
 *
 * Copyright 2016 Mateus Bellomo https://mateusbellomo.wordpress.com/  All rights reserved.
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
 *
 */

