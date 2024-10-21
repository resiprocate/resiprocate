
#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
#include "CommandThread.hxx"

#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/message.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/connection.hpp>
#include <proton/connection_options.hpp>
#include <proton/container.hpp>
#include <proton/tracker.hpp>
#include <proton/source_options.hpp>
#include <proton/work_queue.hpp>

// Cajun JSON
#include "cajun/json/reader.h"
#include "cajun/json/writer.h"
#include "cajun/json/elements.h"

using proton::receiver_options;
using proton::source_options;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

using namespace registrationagent;
using namespace resip;
using namespace std;

CommandThread::CommandThread(const std::string &u)
   : ProtonThreadBase::ProtonReceiverBase(u,
      std::chrono::seconds(60), // FIXME configurable
      std::chrono::seconds(2))
{
}

CommandThread::~CommandThread()
{
}

void
CommandThread::processQueue(UserRegistrationClient& userRegistrationClient)
{
   resip::TimeLimitFifo<proton::message>& fifo = getFifo();
   while(fifo.messageAvailable())
   {
      try
      {
         std::unique_ptr<proton::message> m(fifo.getNext());
         std::unique_ptr<json::Object> _jObj(getBodyAsJSON(*m));
         if(!_jObj)
         {
            ErrLog(<<"failed to get message as JSON");
            break;
         }
         json::Object& jObj = *_jObj;
         std::string command = json::String(jObj["command"]).Value();
         json::Object args = jObj["arguments"];
         StackLog(<<"received command " << command);
         if(command == "set_contact")
         {
            string _aor = json::String(args["aor"]).Value();
            Data aor(_aor);
            string _newContact = json::String(args["contact"]).Value();
            Data newContact(_newContact);
            json::Number expires = json::Number(args["expires"]);
            StackLog(<<"aor = " << aor << " contact = " << newContact << " expires = " << expires.Value());
            vector<Data> route;
            json::Array _route = json::Array(args["route"]);
            for(json::Array::const_iterator it = _route.Begin(); it != _route.End(); it++)
            {
               const json::String& element = json::String(*it);
               StackLog(<<"adding route element " << element.Value());
               route.push_back(Data(element.Value()));
            }
            uint64_t now = ResipClock::getTimeSecs();
            if(expires.Value() < now)
            {
               DebugLog(<<"dropping a command because expiry has already passed " << now - expires << " seconds ago");
            }
            else
            {
               userRegistrationClient.setContact(Uri(aor), newContact, expires.Value(), route);
            }
         }
         else if(command == "unset_contact")
         {
            string _aor = json::String(args["aor"]).Value();
            Data aor(_aor);
            userRegistrationClient.unSetContact(Uri(aor));
         }
      }
      catch(...)
      {
         ErrLog(<<"exception while parsing the JSON");
      }
   }
}

/* ====================================================================
 *
 * Copyright (C) 2022 Daniel Pocock https://danielpocock.com
 * Copyright (C) 2022 Software Freedom Institute LLC https://softwarefreedom.institute
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
