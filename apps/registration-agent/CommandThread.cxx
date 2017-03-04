
#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
#include "CommandThread.hxx"

#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/connection.hpp>
#include <proton/tracker.hpp>
#include <proton/source_options.hpp>

// Cajun JSON
#include "cajun/json/reader.h"
#include "cajun/json/writer.h"
#include "cajun/json/elements.h"

using proton::receiver_options;
using proton::source_options;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONCLIENT

using namespace registrationclient;
using namespace resip;
using namespace std;

CommandThread::CommandThread(const std::string &u)
   : mUrl(u),
     mFifo(0, 0)
{
}

CommandThread::~CommandThread()
{
}

void
CommandThread::on_container_start(proton::container &c)
{
   mReceiver = c.open_receiver(mUrl);
}

void
CommandThread::on_receiver_open(proton::receiver &)
{
   InfoLog(<<"receiver ready for queue " << mUrl);
}

void
CommandThread::on_receiver_close(proton::receiver &r)
{
   InfoLog(<<"receiver closed");
}

void
CommandThread::on_transport_error(proton::transport &t)
{
   WarningLog(<<"transport closed unexpectedly, trying to re-establish connection");
   t.connection().container().open_receiver(mUrl);
}

void
CommandThread::on_message(proton::delivery &d, proton::message &m)
{
   // get body as std::stringstream
   std::string _json;
   try
   {
      _json = proton::get<std::string>(m.body());
   }
   catch(proton::conversion_error& ex)
   {
      ErrLog(<<"failed to extract message body as string: " << ex.what());
      return;
   }
   StackLog(<<"on_message received: " << _json);
   std::stringstream stream;
   stream << _json;

   // extract elements from JSON
   json::Object *elemRootFile = new json::Object();
   if(!elemRootFile)
   {
      ErrLog(<<"failed to allocate new json::Object()"); // FIXME
      return;
   }
   try
   {
      json::Reader::Read(*elemRootFile, stream);
   }
   catch(json::Reader::ScanException& ex)
   {
      ErrLog(<<"failed to scan JSON message: " << ex.what() << " message body: " << _json);
      return;
   }
   mFifo.add(elemRootFile, TimeLimitFifo<json::Object>::InternalElement);
}

void
CommandThread::processQueue(UserRegistrationClient& userRegistrationClient)
{
   while(mFifo.messageAvailable())
   {
      SharedPtr<json::Object> _jObj(mFifo.getNext());
      json::Object& jObj = *_jObj.get();
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
         userRegistrationClient.setContact(Uri(aor), newContact, expires.Value(), route);
      }
      else if(command == "unset_contact")
      {
         string _aor = json::String(args["aor"]).Value();
         Data aor(_aor);
         userRegistrationClient.unSetContact(Uri(aor));
      }
   }
}

void
CommandThread::thread()
{
   while(!mShutdown)
   {
      try
      {
         proton::default_container(*this).run();
      }
      catch(exception& e)
      {
         WarningLog(<<"CommandThread::thread container threw " << e.what());
      }
   }
   InfoLog(<<"CommandThread::thread container stopped");
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
