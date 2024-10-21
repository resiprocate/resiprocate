
#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
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

#include "ProtonCommandThread.hxx"

using proton::receiver_options;
using proton::source_options;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace reconserver;
using namespace recon;
using namespace resip;
using namespace std;

ProtonCommandThread::ProtonCommandThread(const Data& u)
   : ProtonThreadBase::ProtonReceiverBase(u.c_str(),
      std::chrono::seconds(60), // FIXME configurable
      std::chrono::seconds(2))
{
}

ProtonCommandThread::~ProtonCommandThread()
{
}

void
ProtonCommandThread::processQueue(MyConversationManager& conversationManager)
{
   resip::TimeLimitFifo<proton::message>& fifo = getFifo();
   while(fifo.messageAvailable())
   {
      try
      {
         std::unique_ptr<proton::message> m(fifo.getNext());
         std::unique_ptr<json::Object> _jObj(std::move(getBodyAsJSON(*m)));
         if(!_jObj)
         {
            ErrLog(<<"failed to get message as JSON");
            break;
         }
         json::Object& jObj = *_jObj;
         std::string command = json::String(jObj["command"]).Value();
         json::Object args = jObj["arguments"];
         StackLog(<<"received command " << command);
         if(command == "inviteToRoom")
         {
            string _destination = json::String(args["destination"]).Value();
            Data __destination(_destination);
            NameAddr destination(__destination);
            string _room = json::String(args["room"]).Value();
            Data room(_room);
            StackLog(<<"destination = " << destination << " room = " << room);
            conversationManager.inviteToRoom(room, destination);
         }
         else
         {
            ErrLog(<<"unrecognized command: " << command);
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
