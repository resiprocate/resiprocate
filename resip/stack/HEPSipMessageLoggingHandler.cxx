#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdexcept>

#include "resip/stack/HEPSipMessageLoggingHandler.hxx"
#include "rutil/hep/ResipHep.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

HEPSipMessageLoggingHandler::HEPSipMessageLoggingHandler(SharedPtr<HepAgent> agent)
   : mHepAgent(agent)
{
   if(!agent.get())
   {
      ErrLog(<<"agent must not be NULL");
      throw std::runtime_error("agent must not be NULL");
   }
}

HEPSipMessageLoggingHandler::~HEPSipMessageLoggingHandler()
{
}

void
HEPSipMessageLoggingHandler::outboundMessage(const Tuple &source, const Tuple &destination, const SipMessage &msg)
{
   sendToHOMER(source, destination, msg);
}

void
HEPSipMessageLoggingHandler::outboundRetransmit(const Tuple &source, const Tuple &destination, const SendData &data)
{
   // FIXME - implement for SendData
}

void
HEPSipMessageLoggingHandler::inboundMessage(const Tuple& source, const Tuple& destination, const SipMessage &msg)
{
   sendToHOMER(source, destination, msg);
}

void
HEPSipMessageLoggingHandler::sendToHOMER(const Tuple& source, const Tuple& destination, const SipMessage &msg)
{
   mHepAgent->sendToHOMER<SipMessage>(source.getType(),
      source.toGenericIPAddress(), destination.toGenericIPAddress(),
      HepAgent::SIP, msg,
      msg.exists(h_CallId) ? msg.header(h_CallId).value() : Data::Empty);
}

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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
